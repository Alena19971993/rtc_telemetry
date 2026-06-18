#include <iostream>
#include "rtc/rtc.hpp"
#include <nlohmann/json.hpp>

#include "sender.hpp"

Sender::Sender()
{
    rtc::Configuration config;
    pc_ = std::make_shared<rtc::PeerConnection>(config);
    
    reg_callbacks();
}

void Sender::reg_callbacks()
{
    pc_->onStateChange([](rtc::PeerConnection::State state) {
        std::cout << "ICE state: " << state  << std::endl;
    });

    pc_->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
        std::cout << "Gathering state: " << state  << std::endl;
        if (state == rtc::PeerConnection::GatheringState::Complete) {
            std::cout << "SDP: " << std::string(pc_->localDescription().value()) << std::endl;
        }
    });
}

void Sender::start()
{
    pc_->setLocalDescription();
}

void Sender::stop() {}

Sender::~Sender() {}

int main()
{
    rtc::InitLogger(rtc::LogLevel::Warning);
    Sender sender;
    return 0;
}
