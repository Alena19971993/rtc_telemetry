#include "rtc/rtc.hpp"
#include <nlohmann/json.hpp>

#include "sender.hpp"

Sender::Sender() {

    rtc::Configuration config;
    pc_ = std::make_shared<rtc::PeerConnection>(config);
    
    reg_callbacks();
}

void Sender::reg_callbacks() {}

void Sender::start() {}

void Sender::stop() {}

Sender::~Sender() {}

int main() {
    rtc::InitLogger(rtc::LogLevel::Warning);
    Sender sender;
    return 0;
}