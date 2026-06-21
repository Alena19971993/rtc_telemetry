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
    void receive_msg(const std::string &data);

    const std::string offer_path_ = "/tmp/offer.sdp";
    const std::string answer_path_ = "/tmp/answer.sdp";
    std::shared_ptr<rtc::PeerConnection> pc_;
    std::atomic<uint64_t> received_{0};
    std::atomic<uint64_t> sum_latency_us_{0};
    std::atomic<uint64_t> start_time_us_{0};
};
