#ifndef RUDP_CLIENT_CLIENTSEGMENT_H
#define RUDP_CLIENT_CLIENTSEGMENT_H

#include "config.h"

using namespace std;

class ClientSegment {
public:
    byte4 sequenceNumber;
    byte4 ackNumber;
    byte ackFlag;
    byte finFlag;
    byte2 length;
    byte2 checksum; //校验和
    byte2 reserved; //保留字段
    byte *data;

    ClientSegment(byte4 ack_number, byte ack_flag, byte *data, byte4 seq_number = 0, byte fin_flag = FALSE,
                  byte2 checksum = 0, byte2 reserved = 0);

    byte *prepareSegment();
};

#endif