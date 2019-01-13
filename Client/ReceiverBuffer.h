#ifndef RUDP_CLIENT_RECEIVERBUFFER_H
#define RUDP_CLIENT_RECEIVERBUFFER_H

#include "config.h"

class ReceiverBuffer {
public:
    int sequenceNumber;
    byte *data;

    ReceiverBuffer(int sequenceNumber, byte *data);

    byte4 getAck();
};

#endif
