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
    std::atomic_bool gathering_done_{false};
};
