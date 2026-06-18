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
};
