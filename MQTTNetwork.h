#ifndef _MQTTNETWORK_H_
#define _MQTTNETWORK_H_
 
#include "TCPSocketConnection.h"
#include "EthernetInterface.h"
 
class MQTTNetwork {
public:
    MQTTNetwork(EthernetInterface* aNetwork) : network(aNetwork) {
        socket = new TCPSocketConnection();
    }
 
    ~MQTTNetwork() {
        delete socket;
    }
 
    int read(unsigned char* buffer, int len, int timeout) {
        return socket->receive((char*)buffer, len);
    }
 
    int write(unsigned char* buffer, int len, int timeout) {
        return socket->send((char*)buffer, len);
    }
 
    int connect(const char* hostname, int port) {
        return socket->connect(hostname, port);
    }

    char* getIPAddress() {
        return network->getIPAddress();
    }
 
    int disconnect() {
        return socket->close();
    }
 
private:
    EthernetInterface* network;
    TCPSocketConnection* socket;
};
 
#endif // _MQTTNETWORK_H_
 