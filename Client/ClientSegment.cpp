#include <stdio.h>
#include <iostream>
#include <cstring>

#include "ClientSegment.h"

using namespace std;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"

ClientSegment::ClientSegment(byte4 ackNumber, byte ackFlag, byte *data, byte4 seqNumber, byte finFlag, byte2 checksum,
                             byte2 reserved) {
    this->sequenceNumber = seqNumber;
    this->ackNumber = ackNumber;
    this->ackFlag = ackFlag;
    this->finFlag = finFlag;
    this->length = strlen((char *) data);
    this->checksum = checksum;
    this->reserved = reserved;
    this->data = data;
}

#pragma clang diagnostic pop

byte *ClientSegment::prepareSegment() {

    int counter = 0;
    byte *buffer = (byte *) calloc(MTU, sizeof(byte));

    memset(buffer, 0, MTU);

    memcpy(buffer + counter, &sequenceNumber, sizeof(sequenceNumber));
    counter += sizeof(sequenceNumber);

    memcpy(buffer + counter, &ackNumber, sizeof(ackNumber));
    counter += sizeof(ackNumber);

    memcpy(buffer + counter, &ackFlag, 1);
    counter += sizeof(ackFlag);

    memcpy(buffer + counter, &finFlag, 1);
    counter += sizeof(finFlag);

    memcpy(buffer + counter, &length, sizeof(length));
    counter += sizeof(length);

    memcpy(buffer + counter, &checksum, sizeof(checksum));
    counter += sizeof(checksum);

    memcpy(buffer + counter, &reserved, sizeof(reserved));
    counter += sizeof(reserved);

    memcpy(buffer + counter, data, length);

    return buffer;
}

