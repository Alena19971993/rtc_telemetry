#pragma once
#include "rtc/rtc.hpp"

class Receiver {
public:
     Receiver();
    ~Receiver();

    void reg_callbacks();
    void start();
    void stop();

private:
    std::shared_ptr<rtc::PeerConnection> pc_;
};
