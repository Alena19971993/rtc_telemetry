#include "rtc/rtc.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

#include "sdp_file.hpp"
#include "sender.hpp"

using namespace std::chrono;

Sender::Sender() {
    std::remove(answer_path_.c_str());
    std::remove(offer_path_.c_str());

    rtc::Configuration config;
    pc_ = std::make_shared<rtc::PeerConnection>(config);
    reg_callbacks();
}

void Sender::reg_callbacks() {
    pc_->onStateChange(
        [](rtc::PeerConnection::State state) { std::cout << "ICE state: " << state << std::endl; });

    pc_->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
        std::cout << "Gathering state: " << state << std::endl;
        if (state == rtc::PeerConnection::GatheringState::Complete) {
            write_file(offer_path_, std::string(pc_->localDescription().value()));
            std::cout << "Offer written to: " << offer_path_ << std::endl;
        }
    });

    try {
        dc_ = pc_->createDataChannel("telemetry");
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
        pc_->close();
        return;
    }

    dc_->onOpen([this]() { std::cout << "DataChannel opened: " << dc_->label() << std::endl; });

    dc_->onClosed([this]() { std::cout << "DataChannel closed: " << dc_->label() << std::endl; });
}

void Sender::send_msg() {
    auto deadline = steady_clock::now() + minutes(1);
    while (!dc_->isOpen()) {
        if (steady_clock::now() > deadline) {
            std::cout << "Timeout waiting for DataChannel" << std::endl;
            pc_->close();
            return;
        }
        std::this_thread::sleep_for(milliseconds(100));
    }

    static uint64_t seq = 0;
    while (dc_->isOpen()) {
        auto now = system_clock::now().time_since_epoch();
        uint64_t secs = duration_cast<seconds>(now).count();
        uint32_t ns =
            static_cast<uint32_t>(duration_cast<nanoseconds>(now).count() % 1'000'000'000);

        Message msg{++seq, {secs, ns}, "hello"};
        dc_->send(nlohmann::json(msg).dump());
        std::cout << "Sent seq= " << seq << std::endl;
        std::this_thread::sleep_for(milliseconds(100));
    }
}

void Sender::start() {
    if (!dc_) {
        pc_->close();
        return;
    };
    std::cout << "Waiting for " << answer_path_ << std::endl;
    auto answer_sdp = read_file(answer_path_);
    auto deadline = steady_clock::now() + minutes(1);

    while (!answer_sdp) {
        if (steady_clock::now() > deadline) {
            std::cout << "Timeout waiting for answer" << std::endl;
            pc_->close();
            return;
        }
        std::this_thread::sleep_for(seconds(1));
        answer_sdp = read_file(answer_path_);
    }

    try {
        pc_->setRemoteDescription(
            rtc::Description(answer_sdp.value(), rtc::Description::Type::Answer));
    } catch (const std::exception &e) {
        std::cout << "Failed to get SDP: " << e.what() << std::endl;
        pc_->close();
        return;
    }

    send_msg();
}

void Sender::stop() {
    while (pc_->state() != rtc::PeerConnection::State::Closed &&
           pc_->state() != rtc::PeerConnection::State::Failed &&
           pc_->state() != rtc::PeerConnection::State::Disconnected) {
        std::this_thread::sleep_for(seconds(1));
    }
}

Sender::~Sender() { stop(); }

int main() {
    rtc::InitLogger(rtc::LogLevel::Warning);
    Sender sender;
    sender.start();
    return 0;
}
