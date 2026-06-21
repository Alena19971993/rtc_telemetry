#include "rtc/rtc.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

#include "message.hpp"
#include "receiver.hpp"
#include "sdp_file.hpp"

using namespace std::chrono;

Receiver::Receiver() {

    rtc::Configuration config;
    pc_ = std::make_shared<rtc::PeerConnection>(config);

    reg_callbacks();
}

void Receiver::reg_callbacks() {
    pc_->onStateChange(
        [](rtc::PeerConnection::State state) { std::cout << "ICE state: " << state << std::endl; });

    pc_->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
        std::cout << "Gathering state: " << state << std::endl;
        if (state == rtc::PeerConnection::GatheringState::Complete) {
            const std::string answer_path = "/tmp/answer.sdp";
            write_file(answer_path, std::string(pc_->localDescription().value()));
            std::cout << "Answer written to: " << answer_path << std::endl;
        }
    });

    pc_->onDataChannel([this](std::shared_ptr<rtc::DataChannel> dc) {
        std::cout << "DataChannel opened: " << dc->label() << std::endl;

        dc->onClosed([dc]() { std::cout << "DataChannel closed: " << dc->label() << std::endl; });

        dc->onMessage([this](auto data) {
            if (std::holds_alternative<std::string>(data))
                receive_mesg(std::get<std::string>(data));
        });
    });
}

void Receiver::receive_mesg(const std::string &data) {
    Message msg;
    try {
        msg = nlohmann::json::parse(data).get<Message>();
    } catch (const std::exception &e) {
        std::cout << "Failed to parse message: " << e.what() << std::endl;
        return;
    }

    uint64_t count = ++received_;
    if (count == 1)
        start_time_us_ = static_cast<uint64_t>(
            duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());

    auto now = system_clock::now().time_since_epoch();
    uint64_t now_secs = duration_cast<seconds>(now).count();
    uint32_t now_ns =
        static_cast<uint32_t>(duration_cast<nanoseconds>(now).count() % 1'000'000'000);

    double latency_ms =
        (static_cast<double>(now_secs - msg.timestamp.secs) * 1e9 + static_cast<double>(now_ns) -
         static_cast<double>(msg.timestamp.nanosecs)) /
        1e6;

    uint64_t latency_us = static_cast<uint64_t>(latency_ms * 1000.0);
    sum_latency_us_ += latency_us;

    std::cout << "received seq=" << msg.seq << " latency_ms=" << latency_ms << std::endl;

    if (count % 100 == 0) {
        auto start = steady_clock::time_point(microseconds(start_time_us_.load()));
        double elapsed = duration<double>(steady_clock::now() - start).count();

        std::cout << "stats: received=" << count
                  << " avg_latency_ms=" << static_cast<double>(sum_latency_us_.load()) / static_cast<double>(count) / 1000.0
                  << " rate_hz=" << static_cast<double>(count) / elapsed << std::endl;
    }
}

void Receiver::start() {
    const std::string offer_path = "/tmp/offer.sdp";
    std::cout << "Waiting for: " << offer_path << std::endl;
    auto offer_sdp = read_file(offer_path);
    auto deadline = steady_clock::now() + minutes(1);

    while (!offer_sdp) {
        if (steady_clock::now() > deadline) {
            std::cout << "Timeout waiting for offer" << std::endl;
            pc_->close();
            return;
        }
        std::this_thread::sleep_for(seconds(1));
        offer_sdp = read_file(offer_path);
    }

    try {
        pc_->setRemoteDescription(
            rtc::Description(offer_sdp.value(), rtc::Description::Type::Offer));
    } catch (const std::exception &e) {
        std::cout << "Failed to get SDP: " << e.what() << std::endl;
        return;
    }
}

void Receiver::stop() {
    while (pc_->state() != rtc::PeerConnection::State::Closed &&
           pc_->state() != rtc::PeerConnection::State::Failed &&
           pc_->state() != rtc::PeerConnection::State::Disconnected) {
        std::this_thread::sleep_for(seconds(1));
    }
}

Receiver::~Receiver() { stop(); }

int main() {
    rtc::InitLogger(rtc::LogLevel::Warning);
    Receiver receiver;
    receiver.start();
    return 0;
}
