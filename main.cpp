#include "OLEDDisplayFonts.h"
#include "SSD1306I2C.h"
#include "DS1820.h"
#include "SI7021.h"
#include "TCPSocketConnection.h"
#include "ThisThread.h"
#include "mbed.h"
#include "EthernetInterface.h"
#include "MQTTClient.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "mbed_thread.h"
#include <cstdio>

#define VERSION "v01 MVHR multi Si7021 bluepill"
#define CONTROLLER_NUM "99"
#define CONTROLLER_NUM_HEX 0x99
#define WATCHDOG_TIMEOUT_MS 9999
#define LOOP_SLEEP_MS 99
#define MQTT_KEEPALIVE 20
#define NET_TIMEOUT_MS 2000
#define MAX_DS1820 9

Ticker tick_30sec;
Ticker tick_15sec;
Ticker tick_1sec;
Ticker tick_500ms;

bool flag_publish_info;
bool flag_publish_inputs;
bool flag_publish_outputs;
bool flag_read_ds1820;
bool flag_read_si7021;
bool flag_update_oled;

enum IO_state {IO_ON, IO_OFF};

Watchdog &wd = Watchdog::get_instance();

uint8_t mac_addr[6]={0x00, 0x00, 0x00, 0xBE, 0xEF, CONTROLLER_NUM_HEX};
const char* mqtt_broker = "192.168.1.1";
const int mqtt_port = 1883;
char* topic_sub = "cmnd/controller" CONTROLLER_NUM "/+";
char* topic_cmnd = "cmnd/controller" CONTROLLER_NUM "/";
char* topic_pub = "stat/controller" CONTROLLER_NUM "/";
char* topic_lwt = "stat/controller" CONTROLLER_NUM "/online";
unsigned long uptime_sec = 0;
bool connected_net = false;
bool connected_mqtt = false;
uint8_t conn_failures = 0;

#define NUM_INPUTS 9
DigitalIn inputs[] = {PA_0, PA_1, PA_2, PA_3, PA_4, PA_5, PA_6, PA_7, PB_0};
bool input_state[NUM_INPUTS];
#define NUM_OUTPUTS 7
DigitalOut outputs[] = {PB_3, PA_15, PA_12, PA_11, PA_10, PA_9, PA_8};
DigitalOut led(PC_13);

#define NUM_SENSORS 4
int sensor_to_read = 0;
DigitalOut sensors[] = {PB_7, PB_6, PB_5, PB_4}; // switched Vcc on each
SI7021 sensor(PB_9, PB_8, SI7021::SI7021_ADDRESS, 400000); // common to all sensors
SI7021::SI7021_vector_data_t siData[NUM_SENSORS];
SI7021::SI7021_status_t siStatus[NUM_SENSORS];

DS1820* temp_probe[MAX_DS1820];
#define DS1820_DATA_PIN PB_1
int num_ds1820 = 0;

#define OLED_ADR   0x3c
SSD1306I2C oled_i2c(OLED_ADR, PB_9, PB_8);
char oled_msg_line1[25];
char oled_msg_line2[25];
char oled_msg_line3[25];


void message_handler(MQTT::MessageData& md)
{
    // MQTT callback function
    MQTT::Message &message = md.message;
    char topic[md.topicName.lenstring.len + 1];
    sprintf(topic, "%.*s", md.topicName.lenstring.len, md.topicName.lenstring.data);
    char* payload = new char[message.payloadlen + 1];
    sprintf(payload, "%.*s", message.payloadlen, (char*)message.payload);
    // printf("%ld: DEBUG: Received: %s Msg: %s qos %d, retained %d, dup %d, packetid %d\n", uptime_sec, topic, payload, message.qos, message.retained, message.dup, message.id);
    char* sub_topic = topic + strlen(topic_cmnd);  // find the last word of the topic (eg: cmnd/controller00/output2)
    if (!strncmp(sub_topic, "output", 6)) {
        // output# command received
        char* output_num_str = sub_topic + 6;
        int8_t output_num = atoi(output_num_str);
        if (output_num < 0 || output_num >= NUM_OUTPUTS) {
            printf("%ld: Error: unknown output number: %d\n", uptime_sec, output_num);
            return;
        }
        if (!strncmp(payload, "1", 1)) {
            printf("%ld: Turning output %d ON\n", uptime_sec, output_num);
            sprintf(oled_msg_line2, "Output %d ON", output_num);
            outputs[output_num] = 1;
            flag_publish_outputs = true;
        }
        else if (!strncmp(payload, "0", 1)) {
            printf("%ld: Turning output %d OFF\n", uptime_sec, output_num);
            sprintf(oled_msg_line2, "Output %d OFF", output_num);
            outputs[output_num] = 0;
            flag_publish_outputs = true;
        }
        else {
            printf("%ld: Error: unknown output command: %s\n", uptime_sec, payload);
            return;
        }
    }
}

bool publish(MQTT::Client<MQTTNetwork, Countdown> &client, char* topic, char* msg_payload, bool retained = false) {
    // main function to publish MQTT messages
    MQTT::Message msg;
    msg.qos = MQTT::QOS1;
    msg.retained = retained;
    msg.payloadlen = strlen(msg_payload);
    msg.payload = (char*)msg_payload;
    char topic_full[30];
    strcat(topic_full, topic_pub);
    strcat(topic_full, topic);
    // printf("%ld: DEBUG: Publishing: %s to: %s\n", uptime_sec, msg.payload, topic_full);
    if (client.publish(topic_full, msg) != MQTT::SUCCESS) {
        printf("%ld: Publish Error! (topic:%s msg:%s)\n", uptime_sec, topic, msg_payload);
        sprintf(oled_msg_line1, "%s", "MQTT Publish error! :-(");
        return false;
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
        publish_num(client, topic_str, outputs[i]);
    }
}

void update_oled() {
    char uptime_line[25];
    oled_i2c.clear();
    oled_i2c.setFont(ArialMT_Plain_10);
    // Boilerplate stuff
    oled_i2c.drawString(0, 0, "Controller" CONTROLLER_NUM);
    oled_i2c.drawHorizontalLine(0, 12, 128);
    oled_i2c.drawHorizontalLine(0, 54, 128);
    if (uptime_sec < 10) {
        oled_i2c.drawString(0, 54, VERSION);
    }
    else {
        sprintf(uptime_line, "Uptime: %ld", uptime_sec);
        oled_i2c.drawString(0, 54, uptime_line);
    }
    // Dynamic middle bit
    oled_i2c.drawString(2, 15, oled_msg_line1);
    oled_i2c.drawString(2, 28, oled_msg_line2);
    oled_i2c.drawString(2, 40, oled_msg_line3);
    // online status
    if (connected_net) {
        oled_i2c.drawString(75, 0, "NET");
    }
    else {
        oled_i2c.drawString(75, 0, "net");
    }
    if (connected_mqtt) {
        oled_i2c.drawString(100, 0, "MQTT");
    }
    else {
        oled_i2c.drawString(100, 0, "mqtt");
    }
    // send to display
    oled_i2c.setBrightness(64);
    oled_i2c.display();
}



void read_inputs(MQTT::Client<MQTTNetwork, Countdown> &client) {
    for (int i=0; i<NUM_INPUTS; i++) {
        bool old_state = input_state[i];    // save old state
        input_state[i] = inputs[i];         // read new value
        if(input_state[i] != old_state) {
            // input has changed state
            printf("%ld: Input %d changed to %d\n", uptime_sec, i, input_state[i]);
            sprintf(oled_msg_line1, "Input %d changed to %d", i, input_state[i]);
            char topic_str[8]; // long enough string for inputxx
            sprintf(topic_str, "input%d", i);
            publish_num(client, topic_str, input_state[i]);
        }
    }
}

void read_ds1820(MQTT::Client<MQTTNetwork, Countdown> &client) {
    char temp_str[6];
    char topic_str[12];
    // Start temperature conversion of all probes, wait until ready
    temp_probe[0]->convertTemperature(true, DS1820::all_devices);
    // loop through all devices and publish temp
    for (int i = 0; i<num_ds1820; i++) {
        float temp_ds = temp_probe[i]->temperature();
        if (temp_ds == DS1820::invalid_conversion) {
            printf("%ld: DS1820 %d failed temperature conversion :-(\n", uptime_sec, i);
            return;
        }
        else if (temp_ds == -0.25) {
            // reject bad temp readings (0 counts converted to -0.25degC)
            printf("%ld: DS1820 %d bad temp (likely not connected) :-(\n", uptime_sec, i);
            return;
        }
        // convert to string and publish
        sprintf(temp_str, "%3.2f", temp_ds);
        sprintf(topic_str, "probetemp%d", i);
        printf("%ld: DS1820 %d measures %3.2foC\n", uptime_sec, i, temp_ds);
        sprintf(oled_msg_line3, "DS1820 %d = %3.2foC", i, temp_ds);
        // printf("%ld: DS1820 %d measures %doC (int)\n", uptime_sec, i, (int)temp_ds);
        publish(client, topic_str, temp_str, false);
    }
}

void read_si7021(MQTT::Client<MQTTNetwork, Countdown> &client, int num) {
    char temp_str[6];
    char topic_str[12];
    // turn off other sensors
    for (int j = 0; j<NUM_SENSORS; j++) {
        if (j != num) {
            sensors[j] = 0;
        }
    }
    if (siStatus[num] == SI7021::SI7021_FAILURE) {
        printf("%ld: SI7021 %d resetting\n", uptime_sec, num);
        siStatus[num] = sensor.SI7021_SoftReset();
        ThisThread::sleep_for(25);
    }
    if (siStatus[num] == SI7021::SI7021_SUCCESS) {
        // Start humidity conversion (temp conversion triggered by default)
        siStatus[num] = sensor.SI7021_TriggerHumidity(SI7021::SI7021_NO_HOLD_MASTER_MODE);
        thread_sleep_for(50);
        siStatus[num] = sensor.SI7021_ReadHumidity(&siData[num]);
        siStatus[num] = sensor.SI7021_ReadTemperatureFromRH(&siData[num]);
        if (siStatus[num] == SI7021::SI7021_FAILURE) {
            printf("%ld: SI7021 %d failed humidity/temp conversion :-(\n", uptime_sec, num);
        }
    }
    // turn sensors back on
    for (int j = 0; j<NUM_SENSORS; j++) {
        sensors[j] = 1;
    }
    if (siStatus[num] == SI7021::SI7021_SUCCESS) {
        // convert to string and publish
        printf("%ld: SI7021 %d measures %3.2foC / %3.1f%%RH\n", uptime_sec, num, siData[num].Temperature, siData[num].RelativeHumidity);
        sprintf(oled_msg_line2, "SI7021 %d = %3.1f%%RH", num, siData[num].RelativeHumidity);
        sprintf(oled_msg_line3, "SI7021 %d = %3.2foC", num, siData[num].Temperature);
        sprintf(topic_str, "temp%d", num);
        sprintf(temp_str, "%3.2f", siData[num].Temperature);
        publish(client, topic_str, temp_str, false);
        sprintf(topic_str, "humdity%d", num);
        sprintf(temp_str, "%3.2f", siData[num].RelativeHumidity);
        publish(client, topic_str, temp_str, false);
    }
}

bool networking_init(EthernetInterface &wiz) {
    printf("%ld: Start networking...\n", uptime_sec);
    // reset the w5500
    wiz.init(mac_addr);
    if (wiz.connect(NET_TIMEOUT_MS) != 0) {
        printf("%ld: DHCP failed :-(\n", uptime_sec);
        sprintf(oled_msg_line1, "%s", "DHCP failed :-(");
        sprintf(oled_msg_line2, "IP: --");
        return false;
    }
    printf("%ld: IP: %s\n", uptime_sec, wiz.getIPAddress());
    sprintf(oled_msg_line2, "IP: %s", wiz.getIPAddress());
    return true;
}

bool mqtt_init(MQTTNetwork &mqttNet, MQTT::Client<MQTTNetwork, Countdown> &client) {
    // TCP connect to broker
    printf("%ld: Connecting to MQTT broker...\n", uptime_sec);
    if (mqttNet.connect((char*)mqtt_broker, mqtt_port, NET_TIMEOUT_MS) != MQTT::SUCCESS) {
        printf("%ld: Couldn't connect TCP socket to broker %s :-(\n", uptime_sec, mqtt_broker);
        sprintf(oled_msg_line1, "%s", "Couldn't connect MQTT");
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
    conn_data.MQTTVersion = 3;
    conn_data.keepAliveInterval = MQTT_KEEPALIVE;
    conn_data.clientID.cstring = (char*)"controller" CONTROLLER_NUM;
    if (client.connect(conn_data) != MQTT::SUCCESS) {
        printf("%ld: MQTT Client couldn't connect to broker %s :-(\n", uptime_sec, mqtt_broker);
        sprintf(oled_msg_line1, "%s", "Couldn't connect MQTT");
        conn_failures++;  // record this as a connection failure in case we need to reset the Wiznet
        return false;
    }
    printf("%ld: Connected to broker %s :-)\n", uptime_sec, mqtt_broker);
    sprintf(oled_msg_line1, "%s", "Connected to Broker :-)");
    // Subscribe to topic
    if (client.subscribe(topic_sub, MQTT::QOS1, message_handler) != MQTT::SUCCESS) {
        printf("%ld: MQTT Client couldn't subscribe to topic :-(\n", uptime_sec);
        sprintf(oled_msg_line1, "%s", "MQTT subscribe error");
        return false;
    }
    printf("%ld: Subscribed to %s\n", uptime_sec, topic_sub);
    // Node online message
    publish(client, "version", VERSION, true);
    publish(client, "IPAddress", mqttNet.getIPAddress(), true);
    publish_num(client, "online", 1, true);
    publish_num(client, "inputs", NUM_INPUTS, true);
    publish_num(client, "outputs", NUM_OUTPUTS, true);
    publish_num(client, "ds1820", num_ds1820, true);
    conn_failures = 0;   // remember to reset this on success
    return true;
} 


void every_30sec() {
    // no waits or blocking routines here please!
    flag_publish_inputs = true;
    flag_publish_outputs = true;
    flag_read_ds1820 = true;
}

void every_15sec() {
    // no waits or blocking routines here please!
    flag_publish_info = true;
    flag_read_si7021 = true;
}

void every_second() {
    // no waits or blocking routines here please!
    uptime_sec++;
    if(connected_mqtt) {
        led = !led;
    }
    wd.kick();       // kick the dog before the timeout
    flag_update_oled = true;
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

    printf("\n===========\n%ld: Welcome! Name: Controller%s\n", uptime_sec, CONTROLLER_NUM);
    printf("%ld: Version: %s\n===========\n", uptime_sec, VERSION);
    printf("%ld: Inputs: %d Outputs: %d Sensors: %d\n", uptime_sec, NUM_INPUTS, NUM_OUTPUTS, NUM_SENSORS);
    EthernetInterface wiz(PB_15, PB_14, PB_13, PB_12, PB_11); // SPI2 with PB_11 reset

    MQTTNetwork mqttNetwork(&wiz);
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork, NET_TIMEOUT_MS);

    tick_500ms.attach(&every_500ms, 0.5);
    tick_1sec.attach(&every_second, 1.0);
    tick_15sec.attach(&every_15sec, 15.1);
    tick_30sec.attach(&every_30sec, 29.5);

    // Switch on all SI7021
    for(int i=0; i<NUM_SENSORS; i++) {
        sensors[i] = 1;
    }

    // pull high all inputs
    for(int i=0; i<NUM_INPUTS; i++) {
        inputs[i].mode(PullUp);
        input_state[i] = 1;
    }
    //pulse all outputs
    for(int i=0; i<NUM_OUTPUTS; i++) {
        outputs[i] = 1;
        thread_sleep_for(100);
    }
    for(int i=0; i<NUM_OUTPUTS; i++) {
        outputs[i] = 0;
        thread_sleep_for(100);
    }

    // Initialize DS1820 probe array
    while(DS1820::unassignedProbe(DS1820_DATA_PIN)) {
        temp_probe[num_ds1820] = new DS1820(DS1820_DATA_PIN);
        num_ds1820++;
        if (num_ds1820 == MAX_DS1820) {
            break;
        }
    }
    printf("%ld: DS1820: Found %d device(s)\n", uptime_sec, num_ds1820);

    // Initialise SI7021 temp/humidity sensor
    // siStatus[num] = sensor.SI7021_SoftReset();
    // ThisThread::sleep_for(15);
    // siStatus[num] = sensor.SI7021_Conf(SI7021::SI7021_RESOLUTION_RH_12_TEMP_14, SI7021::SI7021_HTRE_DISABLED);
    // siStatus[num] = sensor.SI7021_GetElectronicSerialNumber(&siData);
    // siStatus[num] = sensor.SI7021_GetFirmwareRevision(&siData);
    // printf("%ld: SI7021: FW: %02x ESN: %16x %16x\n", uptime_sec, siData.FirmwareRevision, siData.ElectronicSerialNumber_MSB, siData.ElectronicSerialNumber_LSB);
    
    // Initialise OLED display
    oled_i2c.init();
    sprintf(oled_msg_line1, "%s", "Initialising...");
    update_oled();
    
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
                connected_mqtt = mqtt_init(mqttNetwork, client);
                if (conn_failures > 3) {      // wiznet could be bad, re-initialise
                    printf("%ld: Too many connection failures! Resetting wiznet...\n", uptime_sec);
                    connected_net = false;
                    conn_failures = 0;
                }
            }
            else {
                // we're connected, do stuff!
                read_inputs(client);
                if(flag_publish_info) {
                    publish_info(client);
                    flag_publish_info = false;
                }
                else if (flag_update_oled) {
                    update_oled();
                    flag_update_oled = false;
                }
                else if (flag_publish_inputs) {
                    publish_inputs(client);
                    flag_publish_inputs = false;
                }
                else if (flag_publish_outputs) {
                    publish_outputs(client);
                    flag_publish_outputs = false;
                }
                else if (flag_read_ds1820) {
                    read_ds1820(client);
                    flag_read_ds1820 = false;
                }
                else if (flag_read_si7021) {
                    read_si7021(client, sensor_to_read);
                    sensor_to_read++;
                    if (sensor_to_read == NUM_SENSORS) {
                        sensor_to_read = 0;
                    }
                    flag_read_si7021 = false;
                }
                connected_mqtt = client.isConnected();
            }
            // printf("%ld: DEBUG: MQTT connected: %d\n", uptime_sec, connected_mqtt);        
            client.yield(LOOP_SLEEP_MS);  // pause a while, yawn......
        }
        if (flag_update_oled) {
            update_oled();
            flag_update_oled = false;
        }
        ThisThread::sleep_for(LOOP_SLEEP_MS);
    }
}
