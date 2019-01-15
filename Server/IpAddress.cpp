#include "IpAddress.h"
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>
#include <net/if.h>
#include <sys/ioctl.h>
#include <system_error>
using namespace std;

void getLocalIP(char* addressBuffer) {
    struct ifaddrs * ifAddrStruct=NULL;
    void * tmpAddrPtr=NULL;
    getifaddrs(&ifAddrStruct);
    while (ifAddrStruct!=NULL) {
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET) { // check it is IP4
            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
        }
        ifAddrStruct=ifAddrStruct->ifa_next;
    }
//    cout << addressBuffer << endl;
}

void getBoardcastIP(char* addressBuffer) {
    struct ifaddrs * ifAddrStruct=NULL;
    void * tmpAddrPtr=NULL;
    getifaddrs(&ifAddrStruct);
    while (ifAddrStruct!=NULL) {
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET) { // check it is IP4
            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_ifu.ifu_broadaddr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
        }
        ifAddrStruct=ifAddrStruct->ifa_next;
    }
//    cout << addressBuffer << endl;
}