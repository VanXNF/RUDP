#ifndef RUDP_SERVER_CONGESTION_H
#define RUDP_SERVER_CONGESTION_H
#include <cmath>

#define INIT_SEC 50000

class Congestion {
public:
    double cwnd; //拥塞窗口报文段个数
    double ssthresh; //慢开始门限值
    int advertisedSize; //通知窗口报文段个数（接收方窗口）
    int dupAck; // ACK 重复确认次数
    bool isSlowStart; // true -> 慢开始算法, false -> 拥塞避免算法
    double RTTs; //RTT 样本值
    double RTTd; //RTT 的偏差的加权平均值
    double RTO; //超时重传时间 RetransmissionTime-Out

    Congestion(int advertisedSize);

    void updateRTT(double start, double end);

    void updateRTO();

    void slowStart();

    void updateSlowStart();

    void updateCongestionAvoidance();
};
#endif