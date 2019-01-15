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
#include "ClientService.h"

#define ACK "ACK Sample Message"
#define NONE 0
#define DROP 1
#define DELAY 2
#define BOTH 3

using namespace std;

ClientService::~ClientService() {
    closeBroadcastService();
    closeService();
}

ClientService::ClientService(int port, int windowSize) {
    this->serverPort = port;
    this->windowSize = windowSize;
}

ClientService::ClientService(string ip, int port, int windowSize) {
    this->serverPort = port;
    this->windowSize = windowSize;
    setServerIP(ip);
}

void ClientService::setServerIP(string ip) {
    this->serverIp = ip;
    clientSocketUDP = new ClientSocketUDP(AF_INET, SOCK_DGRAM, 0, serverPort, serverIp);
}

void ClientService::setData(byte *data) {
    this->data = data;
}

bool ClientService::startService() {
    struct sockaddr_in severSettings;
    severSettings = clientSocketUDP->setupServer();
    byte *header = initRequest(data);
    sendPacket(header, clientSocketUDP->getSocket(), severSettings);
    receiveResponse(clientSocketUDP->getSocket(), severSettings, windowSize);
    free(header);
}

bool ClientService::startControlService(byte *data, byte2 reserved) {
    setData(data);
    struct sockaddr_in severSettings;
    severSettings = clientSocketUDP->setupServer();
    byte *header = initRequest(data, reserved);
    sendPacket(header, clientSocketUDP->getSocket(), severSettings);
    free(header);
}

byte *ClientService::initRequest(byte *data) {
    byte4 sequenceNumber = rand() % 100;
    byte4 ackNumber = 10;
    byte ackFlag = FALSE;
    byte finFlag = FALSE;
    auto *segment = new ClientSegment(ackNumber, ackFlag, data, sequenceNumber, finFlag);
    return segment->prepareSegment();
}

byte *ClientService::initRequest(byte *data, byte2 reserved) {
    byte4 sequenceNumber = rand() % 100;
    byte4 ackNumber = 10;
    byte ackFlag = FALSE;
    byte finFlag = FALSE;
    byte2 checksum = 0;
    auto *segment = new ClientSegment(ackNumber, ackFlag, data, sequenceNumber, finFlag, checksum, reserved);
    return segment->prepareSegment();
}

byte *ClientService::prepareAck(byte4 ackNumber, byte ackFlag) {
    auto *segment = new ClientSegment(ackNumber, ackFlag, (byte *) ACK);
    return segment->prepareSegment();
}

ClientSegment *ClientService::parseRequest(byte *buffer) {
    byte4 seqNo = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | (buffer[0]);
    byte4 ackNo = (buffer[7] << 24) | (buffer[6] << 16) | (buffer[5] << 8) | (buffer[4]);
    byte ackFlag = (byte) buffer[8];
    byte finFlag = (byte) buffer[9];
    byte2 length = (buffer[11] << 8) | (buffer[10]);
    byte2 checksum = (buffer[13] << 8) | (buffer[12]);
    byte2 reserved = (buffer[15] << 8) | (buffer[14]);
    byte *data = &buffer[16];
    return new ClientSegment(ackNo, ackFlag, data, seqNo, finFlag, checksum, reserved);
}

int ClientService::sendPacket(byte *buffer, int clientSocketFD, struct sockaddr_in serverDetails) {
    int sentNum = sendto(clientSocketFD, buffer, MTU, 0, (struct sockaddr *) &serverDetails, sizeof(serverDetails));
    if (sentNum < 0) {
        cerr << "Error in sending to socket: " << strerror(errno) << endl;
        exit(1);
    }
    return sentNum;
}

bool ClientService::receiveResponse(int socketFD, struct sockaddr_in serverDetails, size_t windowSize, int mode,
                                    int percentage) {
    byte *receivedBuffer = (byte *) calloc(MTU, sizeof(byte));
    memset(receivedBuffer, 0, MTU);
    byte *sendBuffer = (byte *) calloc(MTU, sizeof(byte));
    memset(sendBuffer, 0, MTU);
    ostringstream receivedStream;

    bool flag = true; // 结束接收标志
    byte4 finFlag = FALSE; // 数据包结束标志
    int expectedSeqNo = -1; // 期望的 Seq 值
    int lastByteReceived = -1; // 期望 seq 的值 -1，即数据尾部序号

    deque<ReceiverBuffer *> receiverWindow;
    receiverWindow.clear();

    auto *empty = new ReceiverBuffer(-1, (byte *) " ");

    struct timeval tv;
    while (true) {
        tv.tv_sec = 3; // 设置超时时间为3秒
        tv.tv_usec = 0;
        setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        if (recvfrom(socketFD, receivedBuffer, MTU, 0, nullptr, nullptr)) {
            ClientSegment *temp = parseRequest(receivedBuffer);

            //模拟丢失与延迟
            if (mode != NONE) {
                if (rand() % 100 + 1 <= percentage) {
                    if (mode == DROP) continue;
                    else if (mode == DELAY) usleep((rand() % 10) * 1000000);
                    else if (mode == BOTH) {
                        if (rand() % 2 == 0) continue;
                        else usleep((rand() % 10) * 1000000);
                    }
                }
            }

            //如果头部有结束标记，则标记数据发送完毕
            if (temp->finFlag == TRUE) {
                finFlag = TRUE;
            }

            // 获取上一个数据包的 SeqNo 加上头部数据字段设计长度，计算期望 Seq 值
            if (expectedSeqNo == -1 && lastByteReceived == -1) {
                //第一次收到数据包, 填入接收窗口
                receiverWindow.push_back(new ReceiverBuffer(temp->sequenceNumber, temp->data));
                lastByteReceived = temp->sequenceNumber + (MTU - HEADER_LEN) - 1;
            } else if (expectedSeqNo == temp->sequenceNumber) {
                // 收到期望的数据包
                if (receiverWindow.empty()) {
                    receiverWindow.push_back(new ReceiverBuffer(temp->sequenceNumber, temp->data));
                } else {
                    receiverWindow.pop_front();
                    receiverWindow.push_front(new ReceiverBuffer(temp->sequenceNumber, temp->data));
                }
                lastByteReceived = temp->sequenceNumber + (MTU - HEADER_LEN) - 1;
            } else if (expectedSeqNo != temp->sequenceNumber) {
                // 收到乱序数据包时 （Seq - 期望 Seq 值） / (单个数据包最大尺寸) + 1
                int diff = (int) (((temp->sequenceNumber) - expectedSeqNo) / (MTU - HEADER_LEN)) + 1;
                if (diff > windowSize - 1) {
                    // 如果收到的数据包的 SeqNo 与期望 Seq 值相差之间数据包数量差距超过窗口尺寸，则直接丢弃
                    continue;
                }

                if (diff >= receiverWindow.size()) {
                    // 期望： 1, 3, 5 乱序： 1, 5, 3
                    // diff = (5 - 3) / 2 = 1 与期望的数据包差 1，则实际差距 2
                    // [1] -> [1, -1, -1] -> [1, -1, 5]
                    receiverWindow.resize(diff + 1, empty);
                    receiverWindow.at(diff) = new ReceiverBuffer(temp->sequenceNumber, temp->data);
                } else {
                    // [1, -1, -1, 7] -> [1, -1, 5, 7]
                    receiverWindow.at(diff) = new ReceiverBuffer(temp->sequenceNumber, temp->data);
                }
                lastByteReceived = temp->sequenceNumber + (MTU - HEADER_LEN) - 1;
            } else if (expectedSeqNo > temp->sequenceNumber) {
                continue;
            }

            // 检查数据包是否失序
            if (receiverWindow.front()->sequenceNumber != -1) {
                while (!receiverWindow.empty()) {
                    if (receiverWindow.front()->sequenceNumber == -1) break;
                    receivedStream << receiverWindow.front()->data;
                    expectedSeqNo = receiverWindow.front()->sequenceNumber + (MTU - HEADER_LEN);
                    receiverWindow.pop_front();
                }
            }

            // 发送确认 ACK, 设置 ACK 标志位为 TRUE
            sendBuffer = prepareAck(expectedSeqNo + 1, TRUE);
            sendPacket(sendBuffer, socketFD, serverDetails);
            memset(receivedBuffer, 0, MTU);
            // 如果收到含有结束标记的数据包且接收窗口已处理完毕并清空
            if (finFlag == TRUE && receiverWindow.empty()) {
                flag = false;
            }
            delete temp;
        }

        if (!flag) {
            break;
        }
    }

    //打印输出收到的字符
    cout << receivedStream.str() << endl;
    return true;
}

void ClientService::receiveBroadcast(int port, char *message) {
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

void ClientService::startBroadcast(struct sockaddr_in *peerAddr, char *msg, int sleepSecond) {
    if (-1 != sendto(broadcastFD, msg, sizeof(msg), 0, (struct sockaddr *) peerAddr, sizeof(struct sockaddr_in))) {
        cout << "Send Broadcast Success" << endl;
    } else {
        cout << "Send Broadcast Error" << endl;
    }
    sleep(sleepSecond);
}

struct sockaddr_in ClientService::initBroadcast(int port) {
    struct sockaddr_in peerAddr;
    const int opt = 1; //广播标记
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

    ret = setsockopt(broadcastFD, SOL_SOCKET, SO_BROADCAST, (char *) &opt, sizeof(opt));
    if (ret == -1) {
        cout << "Set Socket To Broadcast Format Fail" << endl;
        exit(2);
    }
    return peerAddr;
}

void ClientService::closeBroadcastService() {
    close(this->broadcastFD);
}

bool ClientService::closeService() {
    this->clientSocketUDP->closeSocket();
}

ReceiverBuffer *ClientService::getReceiverBuffer(byte *buffer) {
    ClientSegment *temp = parseRequest(buffer);
    return new ReceiverBuffer(temp->sequenceNumber, temp->data);
}