#ifndef RUDP_SERVER_SENDUNIT_H
#define RUDP_SERVER_SENDUNIT_H
#include "ServerSegment.h"

using namespace std;

class SendUnit {
public:
    size_t dataSize;     //数据长度
    byte* data;
    byte4 initSeqNo;    //初始序号
    ServerSegment *serverSegment;

    SendUnit(byte4 SeqNo);

    SendUnit(byte4 seqNo, byte* data);

    ServerSegment *getRUDP(byte4 seqNo);

    byte *getSendUnit(byte4 seqNo);

    void setData(byte* data);

    void openFile(byte *filename);
};
#endif