#include "Congestion.h"

using namespace std;

Congestion::Congestion(int advertisedSize) {
    this->advertisedSize = advertisedSize;
    this->cwnd = 1;
    this->ssthresh = 64;
    this->dupAck = 0;
    this->isSlowStart = true;
    this->RTTs = INIT_SEC;
    this->RTTd = 0;
    this->RTO = 0;
}

void Congestion::updateRTT(double start, double end) {
    float a = 0.125; //计算新 RTT 样本值时加权 RFC 6298 推荐值
    float b = 0.25; //计算 RTT 的偏差的加权平均值时 RFC 6298 推荐值
    double newSampleRTT = end - start;
    RTTs = (1.0 - a) * RTTs + (a * newSampleRTT);
    RTTd = (1.0 - b) * RTTd + (b * abs(RTTs - newSampleRTT));
    RTO = RTTs + 4 * RTTd;
}

void Congestion::slowStart() {
    this->ssthresh = this->cwnd / 2;
    this->cwnd = 1; //重置拥塞窗口
    this->isSlowStart = true;
    this->dupAck = 0;
}


/**
 * 慢启动算法
 */
void Congestion::updateSlowStart() {
    this->cwnd *= 2;
    this->dupAck = 0;
    this->isSlowStart = true;
}

/**
 * 拥塞避免算法
 */
void Congestion::updateCongestionAvoidance() {
    this->cwnd += 1; //加法增大
    this->dupAck = 0;
    this->isSlowStart = false;
}