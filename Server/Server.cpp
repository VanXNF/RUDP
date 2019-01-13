#include <stdio.h>
#include <iostream>
#include <arpa/inet.h>
#include "ServerService.h"

using namespace std;

int main(int argc, char const *argv[]) {
    int port = 7896;
    int size = 20;
//    cout << "Please input port and window size:";
//    cin >> port >> size;
    ServerService service = ServerService(port, size);

    char msg[20] = "BROADCAST";
    //初始化广播
    struct sockaddr_in peerAddr = service.initBroadcast(9001);
    //发送单条广播
    service.startBroadcast(&peerAddr, msg);
    service.startBroadcast(&peerAddr, msg);
    service.startBroadcast(&peerAddr, msg, 5);
    service.startBroadcast(&peerAddr, msg);
    service.startBroadcast(&peerAddr, msg);
    service.startBroadcast(&peerAddr, msg);
    service.startService();
//    service.startControlService();
//    service.startControlServiceWithResult();
    return 0;
}