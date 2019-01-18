#include <stdio.h>
#include <iostream>
#include <bitset>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <string>
#include <ctime>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <errno.h>
#include <deque>
#include <time.h>

#include "ServerService.h"

#define APP 'A'
#define TIMER 'B'
#define ACK 'C'
#define NANO_SEC 1000000000
#define MICR_SEC 1000000
#define INIT_SEC 50000

ServerService::ServerService(int port, int windowSize) {
    this->serverPort = port;
    this->receiverWindowSize = windowSize;
    this->serverSocketUDP = new ServerSocketUDP(AF_INET, SOCK_DGRAM, 0, serverPort);
}

bool ServerService::startService() {

    socklen_t clientLength;
    struct sockaddr_storage clientDetails;
    byte buffer[MTU];
    bzero(buffer, MTU);
    bool flag = false;
    clientLength = sizeof(clientDetails);
    while (true) {
        recvfrom(serverSocketUDP->getSocket(), buffer, MTU, 0, (struct sockaddr *) &clientDetails, &clientLength);
        ServerSegment *segment = parseRequest(buffer);
        if (segment->data && segment->ackFlag == 'N') {
            cout << "Received: ";
            cout << (char *) segment->data << endl;
            string key = (char *) segment->data;
            key = "I received " + key;
            for (int i = 0; i < 200; ++i) {
                key += "123 hjg ghjfhdjg hj cjchjfghjcghjc   yjccgyjvghjvg jyy yjcgyj vyj";
            }
//            todo 对收到信息做处理，并响应请求
            if (sendSegments((byte *) key.c_str(), serverSocketUDP->getSocket(), clientDetails, receiverWindowSize)) {
                flag = true;
            }
        }
        if (flag) {
            cout << "Sent all segments. Closing server connection." << endl;
//            break;
        }
    }
}

ServerSegment* ServerService::startControlService() {
    socklen_t clientLength;
    struct sockaddr_storage clientDetails;
    byte buffer[MTU];
    bzero(buffer, MTU);
    clientLength = sizeof(clientDetails);
    recvfrom(serverSocketUDP->getSocket(), buffer, MTU, 0, (struct sockaddr *) &clientDetails, &clientLength);
    ServerSegment *segment = parseRequest(buffer);

    if (segment->data && segment->ackFlag == 'N') {
        cout << "received: ";
        cout << (char *) segment->data << endl;
        return segment;
    } else {
        return nullptr;
    }
}

void ServerService::startControlServiceWithResult(int fd, byte* (* pFunction)(int, byte* )) {
    socklen_t clientLength;
    struct sockaddr_storage clientDetails;
    byte buffer[MTU];
    bzero(buffer, MTU);
    clientLength = sizeof(clientDetails);
    recvfrom(serverSocketUDP->getSocket(), buffer, MTU, 0, (struct sockaddr *) &clientDetails, &clientLength);
    ServerSegment *segment = parseRequest(buffer);

    if (segment->data && segment->ackFlag == 'N') {
        cout << "received: ";
        cout << (char *) segment->data << endl;
        sendSegments(pFunction(fd, segment->data), serverSocketUDP->getSocket(), clientDetails, receiverWindowSize);
    }
}

ServerSegment *ServerService::parseRequest(byte *buffer) {
    byte4 seqNo = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | (buffer[0]);
    byte4 ackNo = (buffer[7] << 24) | (buffer[6] << 16) | (buffer[5] << 8) | (buffer[4]);
    byte ackFlag = (byte) buffer[8];
    byte finFlag = (byte) buffer[9];
    byte2 length = (buffer[11] << 8) | (buffer[10]);
    byte2 checksum = (buffer[13] << 8) | (buffer[12]);
    byte2 reserved = (buffer[15] << 8) | (buffer[14]);
    byte *data = &buffer[16];
    return new ServerSegment(seqNo, finFlag, data, ackNo, ackFlag, checksum, reserved);
}

bool ServerService::sendResponse(int socketFD, byte *buffer, struct sockaddr_storage clientDetails) {
    socklen_t clientLength = sizeof(clientDetails);
    return sendto(socketFD, buffer, MTU, 0, (struct sockaddr *) &clientDetails, clientLength) != -1;
}

byte4 ServerService::readAck(int socketFD, struct sockaddr_storage clientDetails) {
    byte *buffer = (byte *) calloc(MTU, sizeof(byte));
    socklen_t clientLength = sizeof(clientDetails);
    memset(buffer, 0, MTU);

    if (recvfrom(socketFD, buffer, MTU, 0, (struct sockaddr *) &clientDetails, &clientLength)) {
        ServerSegment *temp = parseRequest(buffer);
        if (temp->ackFlag == TRUE) {
            free(buffer);
            return temp->ackNumber;
        }
        delete temp;
    } else {
        //若 ack 标志位为 FALSE 则直接丢弃
        free(buffer);
    }
    return 0;
}

double ServerService::getTime() {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    return start.tv_sec + start.tv_nsec / NANO_SEC;
}

void ServerService::retransmit(byte4 seqNo, int socketFD, struct sockaddr_storage clientDetails, SendUnit *sendUnit) {
    byte *buffer = (byte *) calloc(MTU, sizeof(byte));
    memset(buffer, 0, MTU);
    buffer = sendUnit->getSendUnit(seqNo);
    sendResponse(socketFD, buffer, clientDetails);
    memset(buffer, 0, MTU);
    free(buffer);
}

bool
ServerService::sendSegments(byte *data, int socketFD, struct sockaddr_storage clientDetails, int recvWindowSize) {

    byte4 initSeqNo = rand() % 100;
    byte4 seqNo = initSeqNo;
    byte4 sendBase = initSeqNo;

    byte *buffer = (byte *) calloc(MTU, sizeof(byte));
    memset(buffer, 0, MTU);

    //使用序号获取 udp
    auto *sendUnit = new SendUnit(initSeqNo, data);

    auto *congestion = new Congestion(recvWindowSize);

    // 丢包数
    int packetDropped = 0;
    //数据分段数量
    int packetNumber = (int) (sendUnit->dataSize / (MTU - HEADER_LEN)) + 1;

    fd_set readFDs; // fd_set 来存储文件描述符, 用于使用多个套接字时不阻塞线程
    FD_ZERO(&readFDs); //FD_ZERO宏将一个 fd_set 类型变量的所有位都设为 0
    FD_SET(socketFD, &readFDs);
    int nextFD = socketFD + 1;

    byte4 ackNo;

    struct timeval timeValue;
    timeValue.tv_sec = 0; //秒
    timeValue.tv_usec = INIT_SEC; //微秒

    //双端队列实现滑动窗口
    deque<Sliding *> window;
    window.clear();
    int readyDescriptors; // -1 -> Error
    int sendDataNum; // 单次需要发送数据段数量
    int SlideWindowDataNum = 0;
    int left = 0;

    while (true) {
        sendDataNum = (int) min(congestion->cwnd, (double) recvWindowSize);
        if (congestion->isSlowStart) {
            cout << "Sending Packets in mode: Slow Start" << endl;
            cout << "Maximum Packets that will be sent:" << sendDataNum << endl;
        } else {
            cout << "Sending Packets in mode: Congestion Avoidance" << endl;
            cout << "Maximum Packets that will be sent:" << sendDataNum << endl;
        }

        cout << "RetransmissionTime-Out: " << congestion->RTO << endl;
        window.clear();

        //向滑动窗格填充数据
        while (sendDataNum != 0 && (seqNo - initSeqNo) < sendUnit->dataSize) {
            buffer = sendUnit->getSendUnit(seqNo);
            sendResponse(socketFD, buffer, clientDetails);
            window.push_back(new Sliding(sendUnit->getRUDP(seqNo), getTime()));
            seqNo += (MTU - HEADER_LEN);
            sendDataNum--;
        }
        //获取滑动窗格分组数
        SlideWindowDataNum = (int) window.size();

        FD_ZERO(&readFDs);
        FD_SET(socketFD, &readFDs);
        if (timeValue.tv_usec != 0 && timeValue.tv_sec != 0) {
            timeValue.tv_sec = (long) congestion->RTO / NANO_SEC;
            timeValue.tv_usec = (congestion->RTO) * 1000 - timeValue.tv_sec;
        }

        while (true) {
            readyDescriptors = select(nextFD, &readFDs, NULL, NULL, &timeValue);
            if (readyDescriptors == -1) {
                cout << "Error in Select" << endl;
                cerr << strerror(errno) << endl;
                exit(5);
            } else if (readyDescriptors > 0) {
                // ACK 判断
                ackNo = readAck(socketFD, clientDetails);
                cout << "Ack Event " << endl;
                if (sendBase < ackNo) {
                    // 创建新 ACK
                    cout << "New ACK: " << ackNo << endl;
                    sendBase = ackNo;
                    if (window.size() > 0 && sendBase - (MTU - HEADER_LEN) >= window.front()->seqNo) {
                        while (!window.empty()) {
                            if (window.front()->seqNo < sendBase && window.front()->mark == false) {
                                congestion->updateRTT(window.front()->sentTime, getTime());
                                window.pop_front();
                            } else {
                                break;
                            }
                        }
                    }
                    if (window.size() == 0) {
                        left = 0;
                        if (congestion->cwnd >= congestion->ssthresh && congestion->ssthresh != -1) {
                            congestion->isSlowStart = false; // congestion avoidance
                            cout << "Entering Congestion Avoidance mode" << endl;
                        }
                        if (congestion->isSlowStart) {
                            congestion->updateSlowStart();
                        } else {
                            congestion->updateCongestionAvoidance();
                        }
                        break;
                    }
                } else {
                    // 重复的 ACK 确认
                    congestion->dupAck++;
                    cout << "Dup ACK: " << ackNo << " ACK Count:" << congestion->dupAck << endl;
                    if (congestion->dupAck >= 3) {
                        // 三次收到重复 ACK 后立即重传
                        retransmit(ackNo, socketFD, clientDetails, sendUnit);
                        congestion->updateRTO();
                        packetDropped++;
                        cout << "Retransmitting : " << ackNo << endl;
                        if (window.size() != 0) {
                            if (!window.front()->mark) {
                                for (int i = 0; i < window.size() - 1; i++) {
                                    window[i]->mark = true;
                                }
                            }
                        }
                        congestion->slowStart();
                        congestion->dupAck = 0;
                        left = window.size();
                    }
                }
            } else if (readyDescriptors == 0) {
                // 超时
                cout << "Timeout Event " << endl;
                if (timeValue.tv_sec == 0 && timeValue.tv_usec == 0) {
                    // Loss occurred go back to slow start
                    packetDropped += window.size();
                    if (window.size() != 0) {
                        left = window.size();
                        retransmit(ackNo, socketFD, clientDetails, sendUnit);
                        cout << "Retransmitting : " << window.front()->seqNo << endl;
                        window.clear();
                    }
                    congestion->slowStart();
                }
                break;
            }
        }

        cout << "Percentage of Packets Sent: " << SlideWindowDataNum - left << "/" << SlideWindowDataNum << " = "
             << ((float) (SlideWindowDataNum - left) / SlideWindowDataNum) * 100 << endl;
        SlideWindowDataNum = 0, left = 0;
        if ((seqNo - initSeqNo) > sendUnit->dataSize || (ackNo - initSeqNo) > sendUnit->dataSize) {
            cout << endl << "Last Packet has been delivered & All ACKs have been received" << endl;
            break;
        }
    }

    memset(buffer, 0, MTU);
    delete sendUnit;
    return true;
}

struct sockaddr_in ServerService::initBroadcast(int port) {
    struct sockaddr_in peerAddr;
    const int opt = 1;
    int ret = 0;

    bzero(&peerAddr, sizeof(struct sockaddr_in));
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons(port);
    peerAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    broadcastFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (broadcastFD == -1) {
        cout << "Create Broadcast Socket Fail" << endl;
        exit(1);
    }

    ret = setsockopt(broadcastFD, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));
    if (ret == -1) {
        cout << "Set Socket To Broadcast Format Fail" << endl;
        exit(2);
    }
    return peerAddr;
}

void ServerService::startBroadcast(struct sockaddr_in *peerAddr, char* msg, int sleepSecond) {
    //    char msg[100] = "Msg from udp broadcast client!";
    if (-1 != sendto(broadcastFD, msg, sizeof(msg), 0, (struct sockaddr *) peerAddr, sizeof(struct sockaddr_in))) {
        cout << "Send Broadcast Success" << endl;
    } else {
        cout << "Send Broadcast Error" << endl;
    }
    sleep(sleepSecond);
}

void ServerService::receiveBroadcast(int port, char* message) {
    struct sockaddr_in ownAddr, peerAddr;
    char recvMsg[200] = {0};
    socklen_t peerAddrLen = 0;
    char peerName[30] = {0};
    int ret = 0;

    bzero(&ownAddr, sizeof(struct sockaddr_in));
    bzero(&peerAddr, sizeof(struct sockaddr_in));
    ownAddr.sin_family = AF_INET;
    ownAddr.sin_port = htons(port);
    ownAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    broadcastFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (broadcastFD == -1) {
        cout << "Create Broadcast Sock Fail" << endl;
        return;
    }

    ret = bind(broadcastFD, (struct sockaddr *) &ownAddr, sizeof(struct sockaddr_in));
    if (ret == -1) {
        cout << "Bind Broadcast Address Fail" << endl;
        return;
    }

    while (true) {
        ret = recvfrom(broadcastFD, recvMsg, sizeof(recvMsg), 0, (struct sockaddr *) &peerAddr, &peerAddrLen);
        if (ret > 0) {
            inet_ntop(AF_INET, &peerAddr.sin_addr.s_addr, peerName, sizeof(peerName));
            if (!strcmp("0.0.0.0", peerName)) {
                continue;
            } else {
                printf("Receive from %s, msg:%s\n", peerName, recvMsg);
                memcpy(message, peerName, strlen(peerName));
                break;
            }
        } else {
            cout << "Receive Broadcast Message Error" << endl;
        }
    }
    bzero(recvMsg, sizeof(recvMsg));
}

void ServerService::closeBroadcastService() {
    close(this->broadcastFD);
}

bool ServerService::closeService() {
    close(serverSocketUDP->getSocket());
}

ServerService::~ServerService() {
    closeBroadcastService();
    closeService();
}
