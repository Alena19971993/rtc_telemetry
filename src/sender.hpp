#pragma once
#include "rtc/rtc.hpp"

struct Timestamp {
    uint64_t secs;
    uint32_t nanosecs;
};

struct Message {
    uint64_t    seq;
    Timestamp   timestamp;
    std::string payload;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Timestamp, secs, nanosecs)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Message, seq, timestamp, payload)

class Sender {
public:
    Sender();
    ~Sender();

    void reg_callbacks();
    void start();
    void stop();

private:
    void reg_callbacks();
    void send_message();

    std::optional<Message> message_;
    std::shared_ptr<rtc::PeerConnection> pc_;
    std::shared_ptr<rtc::DataChannel> dc_;
    const std::string answer_path_ = "/tmp/answer.sdp";
    const std::string offer_path_ = "/tmp/offer.sdp";
};
