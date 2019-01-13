#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include "ServerSocket.h"
#include "IpAddress.h"

using namespace std;

ServerSocketUDP::~ServerSocketUDP() {
    closeSocket();
}

ServerSocketUDP::ServerSocketUDP(int domain, int type, int protocol, int port) {
    this->portNumber = port;
    this->serverSocketFD = -1;  //FD: file descriptor 文件描述符
    this->socketServer(domain, type, protocol);
    if (this->bindServer() > 0) {
        cout << "Binding successfully done...." << endl;
    }
}

int ServerSocketUDP::getSocket() {
    return this->serverSocketFD;
}

void ServerSocketUDP::closeSocket() {
    close(this->serverSocketFD);
}

int ServerSocketUDP::socketServer(int domain, int type, int protocol) {

    this->serverSocketFD = socket(domain, type, protocol);

    if (this->serverSocketFD < 0) {
        cerr << "Error in opening socket: " << strerror(errno) << endl;
        exit(0);
    }

    return 1;
}

int ServerSocketUDP::bindServer() {

    struct sockaddr_in serverAddress;
    char ip[16];
    getLocalIP(ip);
    serverAddress.sin_family = AF_INET; //指定协议地址族
    serverAddress.sin_addr.s_addr = inet_addr(ip);
    serverAddress.sin_port = htons(this->portNumber);

    //绑定对应端口，监听服务
    if (bind(this->serverSocketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Error in binding: " << endl;
        cout << "Error Code: " << strerror(errno) << endl;
        exit(1);
    }
    return 1;
}
