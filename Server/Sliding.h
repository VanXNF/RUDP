#ifndef RUDP_SERVER_SLIDING_H
#define RUDP_SERVER_SLIDING_H

#include "ServerSegment.h"

class Sliding {
public:
    ServerSegment *rudp;
    double sentTime;
    byte4 seqNo;
    bool mark;

    Sliding(ServerSegment *rudp, double sentTime);
};

#endif