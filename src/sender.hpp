#pragma once
#include "rtc/rtc.hpp"

#include "message.hpp"

class Sender {
public:
    Sender();
    ~Sender();

    void start();
    void stop();

private:
    void reg_callbacks();
    void send_message();

    std::shared_ptr<rtc::PeerConnection> pc_;
    std::shared_ptr<rtc::DataChannel> dc_;
    const std::string answer_path_ = "/tmp/answer.sdp";
    const std::string offer_path_ = "/tmp/offer.sdp";
};
