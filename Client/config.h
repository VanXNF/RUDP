
#define MTU 1472 //最大传输单元 (byte)
#define HEADER_LEN 16 // seqNo(4 byte) + ackNo(4 byte) + ackFlag(1 byte) + finFlag(1 byte) + length(2 byte) + checkSum(2 byte) + reserved(2 byte)
#define TRUE 'Y'
#define FALSE 'N'

typedef unsigned char byte; //一字节
typedef unsigned short byte2; //二字节
typedef unsigned int byte4; //四字节

