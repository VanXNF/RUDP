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

bool ClientService::startControlService(byte* data, byte2 reserved) {
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

bool ClientService::receiveResponse(int socketFD, struct sockaddr_in serverDetails, size_t windowSize, int mode, int percentage) {
    byte *receivedBuffer = (byte *) calloc(MTU, sizeof(byte));
    memset(receivedBuffer, 0, MTU);
    byte *sendBuffer = (byte *) calloc(MTU, sizeof(byte));
    memset(sendBuffer, 0, MTU);
    ostringstream receivedStream;
    bool flag = true;
    byte4 finFlag = FALSE;
    int lastByteRead = -1;
    int lastByteReceived = -1;

    deque<ReceiverBuffer *> receiverWindow;
    receiverWindow.clear();

    ReceiverBuffer *empty = new ReceiverBuffer(-1, (byte *) " ");

    while (true) {
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

            if (temp->finFlag == TRUE)
                finFlag = TRUE;

            // Always send ACK for last byte read + 1
            if (lastByteRead == -1 && lastByteReceived == -1) {
                // First packet rcvd. Initial Read.
                receiverWindow.push_back(new ReceiverBuffer(temp->sequenceNumber, temp->data));
                lastByteReceived = temp->sequenceNumber + (MTU - HEADER_LEN) - 1;
            } else if (lastByteRead + 1 == temp->sequenceNumber) {
                // Next expected packet arrival.
                if (receiverWindow.size() == 0) {
                    receiverWindow.push_back(new ReceiverBuffer(temp->sequenceNumber, temp->data));
                } else {
                    receiverWindow.pop_front();
                    receiverWindow.push_front(new ReceiverBuffer(temp->sequenceNumber, temp->data));
                }
                lastByteReceived = temp->sequenceNumber + (MTU - HEADER_LEN) - 1;

            } else if (lastByteRead + 1 != temp->sequenceNumber) {
                // Out of order packet
                int diff = (int) (((temp->sequenceNumber) - (lastByteRead + 1)) / (MTU - HEADER_LEN));

                if (diff > windowSize - 1) {
                    // if out of order packet is greater than rcv window discard packet.
                    continue;
                }

                if (diff >= receiverWindow.size()) {
                    // Out of order cases:
                    // 1. [-1,X] ==> [-1,X,Y] - tested
                    // 2. [-1,X] ==> [-1,X,-1,Y] - tested
                    // 3. [] ==> [-1,-1,X] - tested
                    receiverWindow.resize(diff + 1, empty);
                    receiverWindow.at(diff) = new ReceiverBuffer(temp->sequenceNumber, temp->data);
                } else {
                    // Out of order cases:
                    // 1. [-1,X,-1,Z] ==> [-1,X,Y,Z] - tested
                    receiverWindow.at(diff) = new ReceiverBuffer(temp->sequenceNumber, temp->data);
                }

                lastByteReceived = temp->sequenceNumber + (MTU - HEADER_LEN) - 1;

            } else if (lastByteRead + 1 > temp->sequenceNumber) {
                continue;
            }

            // check for order, update last read
            if (receiverWindow.front()->sequenceNumber != -1) {
                while (!receiverWindow.empty()) {

                    if (receiverWindow.front()->sequenceNumber == -1)
                        break;

                    receivedStream << receiverWindow.front()->data;
                    lastByteRead = receiverWindow.front()->sequenceNumber + (MTU - HEADER_LEN) - 1;
                    receiverWindow.pop_front();

                }
            }


            sendBuffer = prepareAck(lastByteRead + 1, TRUE);
            sendPacket(sendBuffer, socketFD, serverDetails);
            memset(receivedBuffer, 0, MTU);
            // Last packet recieved & nothing to process in the window.
            if (finFlag == TRUE && receiverWindow.size() == 0) {
                flag = false;
            }
            delete temp;
        }

        if (!flag) {
            break;
        }

    }

//    todo 处理返回信息
//    ofstream out_file;
//    out_file.open("out_file.txt");
//    if (out_file.is_open())
//        out_file << receivedStream.str();
//    else
//        cout << " Unable to Open File" << endl;
//    out_file.close();

    cout << receivedStream.str() << endl;
    return true;
}

void ClientService::receiveBroadcast(int port, char* message) {
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
        cout << "Create sock fail" << endl;
        return;
    }

    ret = bind(broadcastFD, (struct sockaddr *) &ownAddr, sizeof(struct sockaddr_in));
    if (ret == -1) {
        cout << "Bind addr fail" << endl;
        return;
    }

    while (true) {
        ret = recvfrom(broadcastFD, recvMsg, sizeof(recvMsg), 0, (struct sockaddr *) &peerAddr, &peerAddrLen);
        if (ret > 0) {
            inet_ntop(AF_INET, &peerAddr.sin_addr.s_addr, peerName, sizeof(peerName));
            printf("Recv from %s, msg[%s]\n", peerName, recvMsg);
            if (!strcmp("0.0.0.0", peerName)) {
                continue;
            } else {
                memcpy(message, peerName, strlen(peerName));
                break;
            }
        } else {
            cout << "Receive message error" << endl;
        }
    }
    bzero(recvMsg, sizeof(recvMsg));
}

void ClientService::startBroadcast(struct sockaddr_in *peerAddr, char *msg, int sleepSecond) {
    if (-1 != sendto(broadcastFD, msg, sizeof(msg), 0, (struct sockaddr *) peerAddr, sizeof(struct sockaddr_in))) {
        cout << "Send msg success" << endl;
    } else {
        cout << "Send msg error" << endl;
    }
    sleep(sleepSecond);
}

struct sockaddr_in ClientService::initBroadcast(int port) {
    struct sockaddr_in peerAddr;
    const int opt = 1;
//    char msg[100] = "Msg from udp broadcast client!";
    int ret = 0;

    bzero(&peerAddr, sizeof(struct sockaddr_in));
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons(port);
    peerAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    broadcastFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (broadcastFD == -1) {
        cout << "Create socket fail" << endl;
        exit(1);
    }

    ret = setsockopt(broadcastFD, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));
    if (ret == -1) {
        cout << "Set socket to broadcast format fail" << endl;
        exit(2);
    }
    return peerAddr;
}

void ClientService::closeBroadcastService() {
    close(this->broadcastFD);
}

bool ClientService::closeService() {
//    free(data);
    this->clientSocketUDP->closeSocket();
}

ReceiverBuffer* ClientService::getReceiverBuffer(byte *buffer) {
    ClientSegment *temp = parseRequest(buffer);
    return new ReceiverBuffer(temp->sequenceNumber, temp->data);
}