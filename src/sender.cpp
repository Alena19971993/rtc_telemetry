#include <iostream>
#include <thread>
#include "rtc/rtc.hpp"
#include <nlohmann/json.hpp>

#include "sender.hpp"
#include "sdp_file.hpp"

Sender::Sender()
{
    std::remove(answer_path_.c_str());
    std::remove(offer_path_.c_str());
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
            write_file(offer_path_, std::string(pc_->localDescription().value()));
            std::cout << "Offer written to: " << offer_path_ << std::endl;
        }
    });

    try {
        dc_ = pc_->createDataChannel("telemetry");
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        pc_->close();
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
    std::cout << "Waiting for " << answer_path_ << std::endl;
    auto answer_sdp = read_file(answer_path_);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(1);

    while (!answer_sdp){
        if (std::chrono::steady_clock::now() > deadline) {
            std::cout << "Timeout waiting for answer" << std::endl;
            pc_->close();
            return;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        answer_sdp = read_file(answer_path_);
    }

    try {
        pc_->setRemoteDescription(rtc::Description(answer_sdp.value(),
                                                   rtc::Description::Type::Answer));
    } catch (const std::exception& e) {
        std::cout << "Faild to get SDP: " << e.what() << std::endl;
        pc_->close();
        return;
    }
}

void Sender::stop()
{
    while (pc_->state() != rtc::PeerConnection::State::Closed &&
           pc_->state() != rtc::PeerConnection::State::Failed &&
           pc_->state() != rtc::PeerConnection::State::Disconnected) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

Sender::~Sender()
{
    stop();
}

int main()
{
    rtc::InitLogger(rtc::LogLevel::Warning);
    Sender sender;
    sender.start();
    return 0;
}
