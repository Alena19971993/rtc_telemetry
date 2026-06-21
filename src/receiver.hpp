#pragma once
#include "rtc/rtc.hpp"

#include "message.hpp"

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
    std::atomic<uint64_t> received_{0};
    std::atomic<uint64_t> sum_latency_us_{0};
    std::atomic<uint64_t> start_time_us_{0};
};
