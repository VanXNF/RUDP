#ifndef RUDP_SERVER_SERVERSEGMENT_H
#define RUDP_SERVER_SERVERSEGMENT_H
#include "config.h"

class ServerSegment {
public:
    byte4 sequenceNumber;
    byte4 ackNumber;
    byte ackFlag;
    byte finFlag;
    byte2 length;
    byte2 checksum;
    byte2 reserved;
    byte *data;

    ServerSegment();

    ServerSegment(byte4 seqNumber, byte finFlag, byte *data, byte4 ackNumber = 0, byte ackFlag = FALSE, byte2 checksum = 0, byte2 reserved = 0);

    byte *prepareSegment();
};
#endif