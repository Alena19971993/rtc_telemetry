#pragma once
#include "rtc/rtc.hpp"

class Sender {
public:
    Sender();
    ~Sender();

    void reg_callbacks();
    void start();
    void stop();

private:
    std::shared_ptr<rtc::PeerConnection> pc_;
    std::shared_ptr<rtc::DataChannel> dc_;
    const std::string answer_path_ = "/tmp/answer.sdp";
    const std::string offer_path_ = "/tmp/offer.sdp";
};
