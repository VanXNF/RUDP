#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>


#include "ClientSocket.h"

using namespace std;

ClientSocketUDP::~ClientSocketUDP() {
    closeSocket();
}

int ClientSocketUDP::socketClient(int domain, int type, int protocol) {

    this->clientSocketFD = socket(domain, type, protocol);

    if (this->clientSocketFD < 0) {
        cerr << "Error in init socket" << strerror(errno) << endl;
        exit(2);
    }
    return 1;
}

ClientSocketUDP::ClientSocketUDP(int domain, int type, int protocol, int port, string server) {
    this->portNumber = port;
    this->clientSocketFD = -1;
    this->serverHost = server;
    this->socketClient(domain, type, protocol);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"

struct sockaddr_in ClientSocketUDP::setupServer() {
    struct sockaddr_in server_address;
    if (inet_addr(this->serverHost.c_str()) == -1) {
        struct hostent *host;

        if ((host = gethostbyname(this->serverHost.c_str())) == nullptr) {
            cerr << "Error resolving hostname " << this->serverHost << endl;
            cout << "Error Code: " << strerror(errno) << endl;
            exit(3);
        }
        struct in_addr **address_list;
        address_list = (struct in_addr **) host->h_addr_list;
        for (int i = 0; address_list[i] != nullptr;) {
            server_address.sin_addr = *address_list[i++];
            break;
        }
    } else {
        server_address.sin_addr.s_addr = inet_addr(this->serverHost.c_str());
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(this->portNumber);
    return server_address;
}

#pragma clang diagnostic pop

int ClientSocketUDP::getSocket() {
    return this->clientSocketFD;
}

void ClientSocketUDP::closeSocket() {
    close(this->clientSocketFD);
}