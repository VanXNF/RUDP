#ifndef RUDP_SERVER_RUDPSERVICE_H
#define RUDP_SERVER_RUDPSERVICE_H

#include "ServerSocket.h"
#include "SendUnit.h"
#include "Sliding.h"
#include "Congestion.h"

class ServerService {
public:
    int broadcastFD;
    int serverPort;
    int receiverWindowSize;
    ServerSocketUDP *serverSocketUDP;

    ~ServerService();

    ServerService(int port, int windowSize);

    bool startService();// 可靠 UDP 用于传输文件或消息

    ServerSegment *startControlService();//非可靠 UDP，用于控制转发鼠标键盘事件

    void startControlServiceWithResult(int fd, byte* (* pFunction)(int, byte* ));

    ServerSegment *parseRequest(byte *buffer);

    bool sendResponse(int socketFD, byte *buffer, struct sockaddr_storage clientDetails);

    void retransmit(byte4 seqNo, int socketFD, struct sockaddr_storage clientDetails, SendUnit *sendUnit);

    bool sendSegments(byte *data, int socketFD, struct sockaddr_storage clientDetails, int recvWindowSize);

    double getTime();

    struct sockaddr_in initBroadcast(int port);
    
    void receiveBroadcast(int port, char* message);

    void startBroadcast(struct sockaddr_in* peerAddr, char* msg, int sleepSecond = 1);

    void closeBroadcastService();

    byte4 readAck(int socketFD, struct sockaddr_storage clientDetails);

    bool closeService();
};

#endif