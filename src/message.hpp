#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

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
