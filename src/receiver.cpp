#include "rtc/rtc.hpp"
#include <nlohmann/json.hpp>

#include "receiver.hpp"

Receiver::Receiver() {

    rtc::Configuration config;
    pc_ = std::make_shared<rtc::PeerConnection>(config);

    reg_callbacks();
}

void Receiver::reg_callbacks() {}

void Receiver::start() {}

void Receiver::stop() {}

Receiver::~Receiver() {}

int main() {
    rtc::InitLogger(rtc::LogLevel::Warning);
    Receiver receiver;
    return 0;
}
