#ifndef RUDP_CLIENT_CLIENTSOCKET_H
#define RUDP_CLIENT_CLIENTSOCKET_H

#include <stdio.h>
#include <string>

using namespace std;

class ClientSocketUDP {
public:
    int clientSocketFD;
    int portNumber;
    string serverHost;

    ~ClientSocketUDP();

    ClientSocketUDP(int domain, int type, int protocol, int port, string server);

    int socketClient(int domain, int type, int protocol);

    struct sockaddr_in setupServer();

    int getSocket();

    void closeSocket();
};

#endif