#include "rtc/rtc.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "sdp_file.hpp"
#include "sender.hpp"
#include "video_reader.hpp"

using namespace std::chrono;

Sender::Sender(std::string video_path) : video_path_(std::move(video_path)) {
    std::remove(answer_path_.c_str());
    std::remove(offer_path_.c_str());

    init();
    reg_callbacks();
}

void Sender::init() {
    rtc::Configuration config;
    pc_ = std::make_shared<rtc::PeerConnection>(config);

    rtpConfig_ = std::make_shared<rtc::RtpPacketizationConfig>(1, "video", 96,
                                                               rtc::H264RtpPacketizer::ClockRate);
    packetizer_ = std::make_shared<rtc::H264RtpPacketizer>(
        rtc::NalUnit::Separator::LongStartSequence, rtpConfig_);
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
}

bool Sender::setup_session(bool has_video) {
    if (has_video) {
        auto video = rtc::Description::Video("video");
        video.addH264Codec(96);

        try {
            track_ = pc_->addTrack(video);
            track_->setMediaHandler(packetizer_);
        } catch (const std::exception &e) {
            std::cout << "addTrack failed: " << e.what() << std::endl;
            pc_->close();
            return false;
        }

        track_->onOpen([this]() { std::cout << "Track opened" << std::endl; });
        track_->onClosed([this]() { std::cout << "Track closed" << std::endl; });
    } else {
        std::cout << "Video unavailable, continuing with data channel only" << std::endl;
    }

    try {
        dc_ = pc_->createDataChannel("telemetry");
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
        pc_->close();
        return false;
    }
    dc_->onOpen([this]() { std::cout << "DataChannel opened: " << dc_->label() << std::endl; });
    dc_->onClosed([this]() { std::cout << "DataChannel closed: " << dc_->label() << std::endl; });

    return true;
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
    bool has_video = reader_.open(video_path_);

    if (!setup_session(has_video)) {
        return;
    }

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

int main(int argc, char *argv[]) {
    rtc::InitLogger(rtc::LogLevel::Warning);
    std::cout << "If you want to start a video stream, specify the path to the mp4 file"
              << std::endl;
    std::string video_path = (argc > 1) ? argv[1] : "";
    Sender sender(video_path);
    sender.start();
    return 0;
}
