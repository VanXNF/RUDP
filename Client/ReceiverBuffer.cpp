#include "ReceiverBuffer.h"
#include <cstring>

ReceiverBuffer::ReceiverBuffer(int sequenceNumber, byte *data) {
    this->sequenceNumber = sequenceNumber;
    this->data = data;
}

byte4 ReceiverBuffer::getAck() {
    return this->sequenceNumber + strlen((char *) data);
}