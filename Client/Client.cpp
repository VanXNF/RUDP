#include <stdio.h>
#include <iostream>
#include <string.h>

#include "ClientService.h"

using namespace std;

int main(int argc, char const *argv[]) {
    string ip = "192.168.4.222";
    int port = 7896;
    int size = 20;
    byte *raw;
//    cout << "Please input <ip> <port> <window size>:" << endl;
//    cin >> ip;
//    cin>> port >> size;

//  首先建立Service
    char message[16] = {0};
    ClientService service = ClientService(port, size);
//    cout << endl;
//  开启广播
    service.receiveBroadcast(9001, message);
//    获取到服务器IP地址后，使用 set 方法
    cout << message;
    service.setServerIP(message);
    cout << "Please input data:" << endl;
    cin >> raw;
    service.setData(raw);
    service.startService();
    cin >> raw;
}