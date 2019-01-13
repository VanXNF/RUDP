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

#include "SendUnit.h"

using namespace std;

SendUnit::SendUnit(byte4 seqNo) {
    this->initSeqNo = seqNo;
}


SendUnit::SendUnit(byte4 seqNo, byte *data) {
    this->initSeqNo = seqNo;
    setData(data);
}

ServerSegment *SendUnit::getRUDP(byte4 seqNo) {
    byte *dataSpace = (byte *) calloc(MTU - HEADER_LEN, sizeof(byte)); //分配数据可用连续空间
    memset(dataSpace, 0, (MTU - HEADER_LEN)); // 初始化数据置临零
    // 如果剩下的数据小于单次最大传输容量，则读取所有剩余数据，将结束位置为 TRUE 后发送，否则读取单次最大传输容量数据，将结束位置 FALSE 后发送。
    if (dataSize - (seqNo - initSeqNo) < MTU - HEADER_LEN) {
        memcpy(dataSpace, &data[(seqNo-initSeqNo)], dataSize - (seqNo - initSeqNo));
        return new ServerSegment(seqNo, TRUE, dataSpace);
    } else {
        memcpy(dataSpace, &data[(seqNo-initSeqNo)], (MTU - HEADER_LEN));
        return new ServerSegment(seqNo, FALSE, dataSpace);
    }
}

byte *SendUnit::getSendUnit(byte4 seqNo) {
    return getRUDP(seqNo)->prepareSegment();
}

void SendUnit::setData(byte *source) {
    this->dataSize = strlen((char *) source);
    this->data = (byte *) calloc(dataSize, sizeof(byte));
    memcpy(this->data, source, dataSize);
}

void SendUnit::openFile(byte *filename) {
    ifstream ifs;
    ifs.open((char *) filename);
    ifs.seekg(0, ios::end);
    this->dataSize = ifs.tellg();
    ifs.seekg(0, ios::beg);
    this->data = (byte *) calloc(this->dataSize, sizeof(byte));
    ifs.read((char *) this->data, this->dataSize);
    ifs.close();
}

byte *getSendBuffer(byte4 seqNo, byte finFlag, byte *data) {
    ServerSegment *sender = new ServerSegment(seqNo, finFlag, data);
    return sender->prepareSegment();
}