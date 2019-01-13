#include "Sliding.h"

Sliding::Sliding(ServerSegment *rudp, double sentTime) {
    this->rudp = rudp;
    this->mark = false;
    this->sentTime = sentTime;
    this->seqNo = rudp->sequenceNumber;
}