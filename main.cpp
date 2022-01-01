#include "DigitalIn.h"
#include "InterfaceDigitalOut.h"
#include "TCPSocketConnection.h"
#include "mbed.h"
#include "WIZnetInterface.h"
#include "MQTTClient.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "mbed_thread.h"

#define VERSION "v01 bluepill"
#define CONTROLLER_NUM "00"
#define CONTROLLER_NUM_DEC 0
#define WATCHDOG_TIMEOUT_MS 9999
#define LOOP_SLEEP_MS 99
#define MQTT_KEEPALIVE 15

Ticker tick_30sec;
Ticker tick_15sec;
Ticker tick_1sec;
Ticker tick_500ms;

bool flag_publish_info;
bool flag_publish_inputs;
bool flag_publish_outputs;
bool flag_read_dht;
bool flag_read_ds1820;

enum IO_state {IO_ON, IO_OFF};

Watchdog &wd = Watchdog::get_instance();

uint8_t mac_addr[6]={0x00, 0x00, 0x00, 0xBE, 0xEF, CONTROLLER_NUM_DEC};
const char* mqtt_broker = "192.168.1.1";
const int mqtt_port = 1883;
char* topic_sub = "cmnd/controller" CONTROLLER_NUM "/+";
char* topic_pub = "stat/controller" CONTROLLER_NUM "/";
char* topic_lwt = "stat/controller" CONTROLLER_NUM "/online";
unsigned long uptime_sec = 0;
bool connected_net = false;
bool connected_mqtt = false;
uint8_t conn_failures = 0;

DigitalOut led(PC_13);
#define NUM_INPUTS 13
DigitalIn inputs[] = {PB_9, PB_8, PB_7, PB_6, PB_5, PB_4, PB_3, PA_15, PA_12, PA_11, PA_10, PA_9, PA_8};
bool input_state[NUM_INPUTS];
#define NUM_OUTPUTS 12
DigitalOut outputs[] = {PC_14, PC_15, PA_0, PA_1, PA_2, PA_3, PA_4, PA_5, PA_6, PA_7, PB_0, PB_1};


void message_handler(MQTT::MessageData& md)
{
    // MQTT callback function
    MQTT::Message &message = md.message;
    // printf("%ld: DEBUG: Message arrived: qos %d, retained %d, dup %d, packetid %d\n", uptime_sec, message.qos, message.retained, message.dup, message.id);
    printf("%ld: Message arrived: %.*s\n", uptime_sec, message.payloadlen, (char*)message.payload);
}

bool publish(MQTT::Client<MQTTNetwork, Countdown> &client, char* topic, char* msg_payload, bool retained = false) {
    // main function to publish MQTT messages
    MQTT::Message msg;
    msg.qos = MQTT::QOS1;
    msg.retained = retained;
    msg.payloadlen = strlen(msg_payload)+1;
    msg.payload = (char*)msg_payload;
    char topic_full[30];
    strcat(topic_full, topic_pub);
    strcat(topic_full, topic);
    // printf("%ld: DEBUG: Publishing: %s to: %s\n", uptime_sec, msg.payload, topic_full);
    if (client.publish(topic_full, msg) != MQTT::SUCCESS) {
        printf("%ld: Publish Error! (topic:%s msg:%s)\n", uptime_sec, topic, msg_payload);
    }
    return true;
}

bool publish_num(MQTT::Client<MQTTNetwork, Countdown> &client, char* topic, int num, bool retained = false) {
    char message[10];
    sprintf(message, "%d", num);
    return publish(client, topic, message, retained);
}

bool publish_info(MQTT::Client<MQTTNetwork, Countdown> &client) {
    // periodic mqtt info message
    char topic[] = "uptime";
    char message[10];
    sprintf(message, "%ld", uptime_sec);
    return publish(client, topic, message);
}

void publish_inputs(MQTT::Client<MQTTNetwork, Countdown> &client) {
    for (int i=0; i<NUM_INPUTS; i++) {
        char topic_str[8]; // long enough string for inputxx
        sprintf(topic_str, "input%d", i);
        publish_num(client, topic_str, input_state[i]);
    }
}

void publish_outputs(MQTT::Client<MQTTNetwork, Countdown> &client) {
    for (int i=0; i<NUM_OUTPUTS; i++) {
        char topic_str[9]; // long enough string for outputxx
        sprintf(topic_str, "output%d", i);
        publish_num(client, topic_str, !outputs[i]);
    }
}

bool networking_init(WIZnetInterface &wiz) {
    printf("%ld: Start networking...\n", uptime_sec);
    // reset the w5500
    wiz.init(mac_addr);
    if (wiz.connect() != 0) {
        printf("%ld: DHCP failed :-(\n", uptime_sec);
        return false;
    }
    printf("%ld: IP: %s\n", uptime_sec, wiz.getIPAddress());
    return true;
}

int8_t mqtt_init(MQTTNetwork &mqttNet, MQTT::Client<MQTTNetwork, Countdown> &client) {
    // TCP connect to broker
    printf("%ld: Connecting to MQTT broker...\n", uptime_sec);
    if (mqttNet.connect((char*)mqtt_broker, mqtt_port) != MQTT::SUCCESS) {
        printf("%ld: Couldn't connect TCP socket to broker %s :-(\n", uptime_sec, mqtt_broker);
        conn_failures++;  // record this as a connection failure in case we need to reset the Wiznet
        return false;
    }
    // Client connect to broker
    MQTTPacket_connectData conn_data = MQTTPacket_connectData_initializer;
    MQTTPacket_willOptions lwt = MQTTPacket_willOptions_initializer;
    lwt.topicName.cstring = topic_lwt;
    lwt.message.cstring = (char*)"0";
    lwt.retained = true;
    conn_data.willFlag = 1;
    conn_data.will = lwt;
    conn_data.keepAliveInterval = MQTT_KEEPALIVE;
    conn_data.clientID.cstring = (char*)"controller" CONTROLLER_NUM;
    if (client.connect(conn_data) != MQTT::SUCCESS) {
        printf("%ld: MQTT Client couldn't connect to broker %s :-(\n", uptime_sec, mqtt_broker);
        conn_failures++;  // record this as a connection failure in case we need to reset the Wiznet
        return false;
    }
    printf("%ld: Connected to broker %s :-)\n", uptime_sec, mqtt_broker);
    // Subscribe to topic
    if (client.subscribe(topic_sub, MQTT::QOS1, message_handler) != MQTT::SUCCESS) {
        printf("%ld: MQTT Client couldn't subscribe to topic :-(\n", uptime_sec);
        return false;
    }
    printf("%ld: Subscribed to %s\n", uptime_sec, topic_sub);
    // Node online message
    publish(client, "version", VERSION, true);
    publish(client, "IPAddress", mqttNet.getIPAddress(), true);
    publish_num(client, "online", 1, true);
    conn_failures = 0;   // remember to reset this on success
    return true;
} 


void every_30sec() {
    // no waits or blocking routines here please!
    flag_publish_inputs = true;
    flag_publish_outputs = true;
}

void every_15sec() {
    // no waits or blocking routines here please!
    flag_read_dht = true;
    flag_read_ds1820 = true;
    flag_publish_info = true;
}

void every_second() {
    // no waits or blocking routines here please!
    uptime_sec++;
    if(connected_mqtt) {
        led = !led;
    }
    wd.kick();       // kick the dog before the timeout
}

void every_500ms() {
    // no waits or blocking routines here please!
    if(connected_net && !connected_mqtt) {
        led = !led;
    }
}


int main(void)
{
    wd.start(WATCHDOG_TIMEOUT_MS);

    printf("\n===============\n%ld: Welcome! Ver: %s\n", uptime_sec, VERSION);
    printf("", uptime_sec);
    WIZnetInterface wiz(PB_15, PB_14, PB_13, PB_12, PB_11); // SPI2 with PB_11 reset

    MQTTNetwork mqttNetwork(&wiz);
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    tick_500ms.attach(&every_500ms, 0.5);
    tick_1sec.attach(&every_second, 1.0);
    tick_15sec.attach(&every_15sec, 15.1);
    tick_30sec.attach(&every_30sec, 29.5);

    // pull high all inputs
    for(int i=0; i<NUM_INPUTS; i++) {
        inputs[i].mode(PullUp);
    }
    //pulse all outputs
    for(int i=0; i<NUM_OUTPUTS; i++) {
        outputs[i] = IO_OFF;
        thread_sleep_for(200);
    }
    
    wd.kick();

    while(1) {

        if(!connected_net) {
            // network isn't connected
            led = IO_OFF;
            connected_mqtt = false;   // if we've no net connection, mqtt ain't connected either
            connected_net = networking_init(wiz);
        }
        else {
            if(!connected_mqtt) {
                // not connected to broker
                mqtt_init(mqttNetwork, client);
                if (conn_failures > 3) {      // wiznet could be bad, re-initialise
                    printf("%ld: Too many connection failures! Resetting wiznet...\n", uptime_sec);
                    connected_net = false;
                    conn_failures = 0;
                }
            }
            // we're connected, do stuff!
            else if(flag_publish_info) {
                publish_info(client);
                flag_publish_info = false;
            }
            else if (flag_publish_inputs) {
                publish_inputs(client);
                flag_publish_inputs = false;
            }
            else if (flag_publish_outputs) {
                publish_outputs(client);
                flag_publish_outputs = false;
            }
        }
        connected_mqtt = client.isConnected();
        // printf("%ld: DEBUG: MQTT connected: %d\n", uptime_sec, connected_mqtt);
        
        client.yield(LOOP_SLEEP_MS);  // pause a while, yawn......
    }
}
