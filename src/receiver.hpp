#pragma once
#include "rtc/rtc.hpp"

class Receiver {
public:
    Receiver();
    ~Receiver();

    void start();
    void stop();

private:
    void reg_callbacks();
    void receive_mesg(const std::string &data);

    std::shared_ptr<rtc::PeerConnection> pc_;
    uint64_t received_{0};
    double sum_latency_{0.0};
    std::chrono::steady_clock::time_point start_time_;
};
