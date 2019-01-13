#ifndef RUDP_CLIENT_CLIENTSERVICE_H
#define RUDP_CLIENT_CLIENTSERVICE_H

#include <cstring>
#include "config.h"
#include "ClientSocket.h"
#include "ClientSegment.h"
#include "ReceiverBuffer.h"

class ClientService {
public:
    string serverIp;
    int serverPort;
    byte* data;
    int windowSize;
    int broadcastFD;
    ClientSocketUDP* clientSocketUDP;

    ~ClientService();

    ClientService(int port, int windowSize);

    ClientService(string ip, int port, int windowSize);

    void setServerIP(string ip);

    bool startService();
    
    bool startControlService(byte* data, byte2 reserved);

    void setData( byte* data);

    byte *initRequest(byte *data);

    byte *initRequest(byte *data, byte2 reserved);

    byte *prepareAck(byte4 ackNumber, byte ackFlag);

    ClientSegment *parseRequest(byte *buffer);

    int sendPacket(byte *buffer, int clientSocketFD, struct sockaddr_in serverDetails);

    bool receiveResponse(int socketFD, struct sockaddr_in serverDetails, size_t windowSize, int mode = 0, int percentage = 0);

    void receiveBroadcast(int port, char* message);

    struct sockaddr_in initBroadcast(int port);

    void startBroadcast(struct sockaddr_in* peerAddr, char* msg ,int sleepSecond = 1);

    void closeBroadcastService();

    bool closeService();

    ReceiverBuffer* getReceiverBuffer(byte *buffer);
};

#endif
