#include <iostream>
#include "rtc/rtc.hpp"
#include <nlohmann/json.hpp>
#include <thread>

#include "receiver.hpp"
#include "sdp_file.hpp"

Receiver::Receiver() {

    rtc::Configuration config;
    pc_ = std::make_shared<rtc::PeerConnection>(config);

    reg_callbacks();
}

void Receiver::reg_callbacks()
{
    pc_->onStateChange([](rtc::PeerConnection::State state) {
        std::cout << "ICE state: " << state  << std::endl;
    });

    pc_->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
        std::cout << "Gathering state: " << state  << std::endl;
        if (state == rtc::PeerConnection::GatheringState::Complete) {
            const std::string answer_path = "/tmp/answer.sdp";
            write_file(answer_path, std::string(pc_->localDescription().value()));
            std::cout << "Answer written to: " << answer_path << std::endl;
        }
    });

    pc_->onDataChannel([this](std::shared_ptr<rtc::DataChannel> dc) {
        std::cout << "DataChannel opened: " << dc->label() << std::endl;

        dc->onClosed([dc]() {
            std::cout << "DataChannel closed: " << dc->label() << std::endl;
        });

        dc->onMessage([this](auto data) {
        });
    });
}

void Receiver::start()
{
    const std::string offer_path = "/tmp/offer.sdp";
    std::cout << "Waiting for " << offer_path << std::endl;
    auto offer_sdp = read_file(offer_path);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(1);

    while (!offer_sdp){
        if (std::chrono::steady_clock::now() > deadline) {
            std::cout << "Timeout waiting for offer" << std::endl;
            pc_->close();
            return;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        offer_sdp = read_file(offer_path);
    }

    try {
        pc_->setRemoteDescription(rtc::Description(offer_sdp.value(),
                                                   rtc::Description::Type::Offer));
    } catch (const std::exception& e) {
        std::cout << "Faild to get SDP: " << e.what() << std::endl;
        return;
    }
}

void Receiver::stop()
{
    while (pc_->state() != rtc::PeerConnection::State::Closed &&
           pc_->state() != rtc::PeerConnection::State::Failed &&
           pc_->state() != rtc::PeerConnection::State::Disconnected) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

Receiver::~Receiver(){
    stop();
}

int main() {
    rtc::InitLogger(rtc::LogLevel::Warning);
    Receiver receiver;
    receiver.start();
    return 0;
}
