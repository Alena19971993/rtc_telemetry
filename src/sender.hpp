#pragma once
#include "rtc/rtc.hpp"

#include "message.hpp"
#include "video_reader.hpp"

class Sender {
public:
    explicit Sender(std::string video_path);
    ~Sender();

    void start();
    void stop();

private:
    void reg_callbacks();
    void send_msg();
    void init();
    bool setup_session(bool has_video);
    void send_video();

    VideoReader reader_;
    std::shared_ptr<rtc::PeerConnection> pc_;
    std::shared_ptr<rtc::DataChannel> dc_;
    std::shared_ptr<rtc::RtpPacketizationConfig> rtpConfig_;
    std::shared_ptr<rtc::H264RtpPacketizer> packetizer_;
    std::shared_ptr<rtc::Track> track_;
    const std::string video_path_;
    const std::string answer_path_ = "/tmp/answer.sdp";
    const std::string offer_path_ = "/tmp/offer.sdp";
};
