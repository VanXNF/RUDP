#ifndef RUDP_SERVER_SERVERSOCKET_H
#define RUDP_SERVER_SERVERSOCKET_H

#include <string.h>

using namespace std;

class ServerSocketUDP {
    int serverSocketFD; //-1 标识错误
    int portNumber;
public:
    ~ServerSocketUDP();

    ServerSocketUDP(int domain, int type, int protocol, int port);

    int getSocket();

    void closeSocket();

    int socketServer(int domain, int type, int protocol);

    int bindServer();
};

#endif