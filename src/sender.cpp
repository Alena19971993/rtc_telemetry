#include <iostream>
#include <thread>
#include "rtc/rtc.hpp"
#include <nlohmann/json.hpp>

#include "sender.hpp"
#include "sdp_file.hpp"

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
            const std::string offer_path = "/tmp/offer.sdp";
            write_file(offer_path, std::string(pc_->localDescription().value()));
            std::cout << "Offer written to: " << offer_path << std::endl;
            gathering_done_ = true;
        }
    });

    try {
        dc_ = pc_->createDataChannel("telemetry");
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return;
    }

    dc_->onOpen([this]() {
        std::cout << "DataChannel opened: " << dc_->label() << std::endl;
    });

    dc_->onClosed([this]() {
        std::cout << "DataChannel closed: " << dc_->label() << std::endl;
    });
}

void Sender::start()
{
    const std::string answer_path = "/tmp/answer.sdp";
    std::cout << "Waiting for " << answer_path << std::endl;
    auto answer_sdp = read_file(answer_path);

    while (!answer_sdp){
        std::this_thread::sleep_for(std::chrono::seconds(1));
        answer_sdp = read_file(answer_path);
    }

    try {
        pc_->setRemoteDescription(rtc::Description(answer_sdp.value(),
                                                   rtc::Description::Type::Answer));
    } catch (const std::exception& e) {
        std::cout << "Faild to get SDP: " << e.what() << std::endl;
        return;
    }
}

void Sender::stop()
{
    while (pc_->state() != rtc::PeerConnection::State::Closed &&
           pc_->state() != rtc::PeerConnection::State::Failed) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

Sender::~Sender() {}

int main()
{
    rtc::InitLogger(rtc::LogLevel::Warning);
    Sender sender;
    sender.start();
    sender.stop();
    return 0;
}
