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
    pc_->onStateChange([this](rtc::PeerConnection::State state) {
        std::cout << "PC state: " << state << std::endl;
        switch (state) {
        case rtc::PeerConnection::State::Disconnected:
            std::cout << "Connection temporarily lost, waiting for recovery." << std::endl;
            break;
        case rtc::PeerConnection::State::Failed:
            std::cout
                << "Connection permanently lost, if the video stream is running, stopping video"
                << std::endl;
            break;
        case rtc::PeerConnection::State::Closed:
            std::cout << "Connection closed, if the video stream is running, stopping video"
                      << std::endl;
            break;
        default:
            break;
        }
    });

    pc_->onIceStateChange([](rtc::PeerConnection::IceState state) {
        std::cout << "ICE state: " << state << std::endl;
        switch (state) {
        case rtc::PeerConnection::IceState::New:
            std::cout
                << "ICE agent created; no remote candidates yet, connectivity checks not started."
                << std::endl;
            break;
        case rtc::PeerConnection::IceState::Checking:
            std::cout << "Pairing local/remote candidates and running STUN connectivity checks."
                      << std::endl;
            break;
        case rtc::PeerConnection::IceState::Connected:
            std::cout << "A working candidate pair was found; media can flow (gathering/checks may "
                         "still continue)."
                      << std::endl;
            break;
        case rtc::PeerConnection::IceState::Completed:
            std::cout << "ICE fully finished: checks done, nominated pair selected, no more "
                         "candidates expected."
                      << std::endl;
            break;
        case rtc::PeerConnection::IceState::Failed:
            std::cout << "No candidate pair succeeded; ICE could not establish a path."
                      << std::endl;
            break;
        case rtc::PeerConnection::IceState::Disconnected:
            std::cout
                << "Connectivity temporarily lost (checks stopped passing); may recover on its own."
                << std::endl;
            break;
        case rtc::PeerConnection::IceState::Closed:
            std::cout << "ICE agent closed; terminal state." << std::endl;
            break;
        }
    });

    pc_->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
        std::cout << "Gathering state: " << state << std::endl;
        if (state == rtc::PeerConnection::GatheringState::Complete) {
            write_file(answer_path_, std::string(pc_->localDescription().value()));
            std::cout << "Answer written to: " << answer_path_ << std::endl;
        }
    });

    pc_->onDataChannel([this](std::shared_ptr<rtc::DataChannel> dc) {
        std::cout << "DataChannel opened: " << dc->label() << std::endl;

        dc->onClosed([dc]() { std::cout << "DataChannel closed: " << dc->label() << std::endl; });

        dc->onMessage([this](auto data) {
            if (std::holds_alternative<std::string>(data))
                receive_msg(std::get<std::string>(data));
        });
    });

    pc_->onTrack([this](std::shared_ptr<rtc::Track> track) {
        std::cout << "Track received: " << track->mid() << std::endl;
        track->setMediaHandler(std::make_shared<rtc::H264RtpDepacketizer>());

        track->onClosed([]() { std::cout << "Track closed" << std::endl; });

        track->onFrame([this](rtc::binary data, rtc::FrameInfo) { receive_video(data); });
        track_ = track;
    });
}

void Receiver::receive_msg(const std::string &data) {
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

        std::cout << "stats: received=" << count << " avg_latency_ms="
                  << static_cast<double>(sum_latency_us_.load()) / static_cast<double>(count) /
                         1000.0
                  << " rate_hz=" << static_cast<double>(count) / elapsed << std::endl;
    }
}

void Receiver::receive_video(const rtc::binary &data) {
    if (!video_file_.is_open()) {
        video_file_.open(video_path_, std::ios::binary);
    }
    video_file_.write(reinterpret_cast<const char *>(data.data()),
                      static_cast<std::streamsize>(data.size()));
    std::cout << "Video frame received, size: " << data.size() << std::endl;
}

void Receiver::start() {
    std::cout << "Waiting for: " << offer_path_ << std::endl;
    auto offer_sdp = read_file(offer_path_);
    auto deadline = steady_clock::now() + minutes(1);

    while (!offer_sdp) {
        if (steady_clock::now() > deadline) {
            std::cout << "Timeout waiting for offer" << std::endl;
            pc_->close();
            return;
        }
        std::this_thread::sleep_for(seconds(1));
        offer_sdp = read_file(offer_path_);
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
