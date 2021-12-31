#include "PinNames.h"
#include "TCPSocketConnection.h"
#include "Watchdog.h"
#include "mbed.h"
#include "WIZnetInterface.h"
#include "MQTTClient.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "stm32f1xx.h"
#include <cstdint>
#include <cstring>

#define VERSION "v01 bluepill"
#define NODE_NUM "00" // TODO just define node number
#define WATCHDOG_TIMEOUT_MS 9999
#define LOOP_SLEEP_MS 99
#define MQTT_DEBUG 1

Ticker tick_30sec;
Ticker tick_15sec;
Ticker tick_1sec;
Ticker tick_500ms;

bool flag_publish_info;
bool flag_error_message;
bool flag_publish_inputs;
bool flag_publish_outputs;
bool flag_read_dht;
bool flag_read_inputs;
bool flag_read_ds1820;

enum led_modes {LED_ON, LED_OFF};

Watchdog &wd = Watchdog::get_instance();

uint8_t mac_addr[6]={0x00, 0x00, 0x00, 0xBE, 0xEF, 0x00}; // TODO make last byte dynamic
const char* mqtt_broker = "192.168.1.1";
char* topic_sub = "cmnd/controller" NODE_NUM "/+";
char* topic_pub = "stat/controller" NODE_NUM "/";
const int mqtt_port = 1883;
unsigned long uptime_sec = 0;
bool connected_net = false;
bool connected_mqtt = false;
uint8_t conn_failures = 0;

DigitalOut led(PC_13);


void message_handler(MQTT::MessageData& md)
{
    // MQTT callback function
    MQTT::Message &message = md.message;
    printf("%ld: Message arrived: qos %d, retained %d, dup %d, packetid %d\n", uptime_sec, message.qos, message.retained, message.dup, message.id);
    printf("%ld: Payload: %.*s\n", uptime_sec, message.payloadlen, (char*)message.payload);
}

bool publish(MQTT::Client<MQTTNetwork, Countdown> &client, char* topic, char* msg_payload, bool retained = false) {
    // main function to publish MQTT messages
    MQTT::Message msg;
    int8_t rc = MQTT::SUCCESS;
    msg.qos = MQTT::QOS1;
    msg.retained = retained;
    msg.payloadlen = strlen(msg_payload)+1;
    msg.payload = (char*)msg_payload;
    char topic_full[30];
    strcat(topic_full, topic_pub);
    strcat(topic_full, topic);
    printf("%ld: DEBUG: Publishing: %s to: %s\n", uptime_sec, msg.payload, topic_full);
    if (client.publish(topic_full, msg) != MQTT::SUCCESS) {
        printf("%ld: Publish Error! client.publish returned: %d\n", uptime_sec, rc);
    }
    return true;
}

bool publish_info(MQTT::Client<MQTTNetwork, Countdown> &client) {
    // periodic mqtt info message
    char topic[] = "uptime";
    char message[10];
    sprintf(message, "%ld", uptime_sec);
    return publish(client, topic, message);
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

int network_test() {
    // check net connection
    TCPSocketConnection testSock;
    testSock.connect("192.168.1.170", 5555);
    return testSock.send((char*)".", 1);
}

int8_t mqtt_init(MQTTNetwork &mqttNet, MQTT::Client<MQTTNetwork, Countdown> &client) {
    //close old connection
    // rc = mqttNet.disconnect();
    // printf("%ld: MQTT Disconnect error: %d", uptime_sec, rc);
    // TCP connect to broker
    printf("%ld: Connecting to broker...\n", uptime_sec);
    if (mqttNet.connect((char*)mqtt_broker, mqtt_port) != MQTT::SUCCESS) {
        printf("%ld: Couldn't connect TCP socket to broker %s :-(\n", uptime_sec, mqtt_broker);
        conn_failures++;  // record this as a connection failure in case we need to reset the Wiznet
        return false;
    }
    // Client connect to broker
    MQTTPacket_connectData conn_data = MQTTPacket_connectData_initializer;
    conn_data.clientID.cstring = (char*)"controller" NODE_NUM;
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
    // publish_value(client, "version", VERSION, true);
    // publish_value(client, "IPAddress", wiz.getIPAddress(), true);
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
    flag_read_inputs = true;
    if(connected_net && !connected_mqtt) {
        led = !led;
    }
}


int main(void)
{
    wd.start(WATCHDOG_TIMEOUT_MS);

    printf("%ld: Welcome! Ver: %s\n", uptime_sec, VERSION);
    WIZnetInterface wiz(PB_15, PB_14, PB_13, PB_12, PB_11); // SPI2 with PB_11 reset

    MQTTNetwork mqttNetwork(&wiz);
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    tick_500ms.attach(&every_500ms, 0.5);
    tick_1sec.attach(&every_second, 1.0);
    tick_15sec.attach(&every_15sec, 15.1);
    tick_30sec.attach(&every_30sec, 29.5);

    wd.kick();

    int testSendResult = 0;

    while(1) {

        if(!connected_net) {
            // network isn't connected
            led = LED_OFF;
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
            // network is connected
            if(flag_publish_info) {
                publish_info(client);
                flag_publish_info = false;
            }
            else {
                // we're connected, do stuff!
            }
        }
        // printf("%ld: Sleeping...\n", uptime_sec);
        // testSendResult = network_test();
        // printf("%ld: Net test result: %d\n", uptime_sec, testSendResult);
        connected_mqtt = client.isConnected();
        // printf("%ld: DEBUG: MQTT connected: %d\n", uptime_sec, connected_mqtt);
        
        client.yield(LOOP_SLEEP_MS);  // pause a while, yawn......
    }
}
