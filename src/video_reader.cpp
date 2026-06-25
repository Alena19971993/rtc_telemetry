#include "video_reader.hpp"

#include <iostream>

VideoReader::~VideoReader() { close(); }

bool VideoReader::open(const std::string &path) {
    std::cout << path;
    return true;
}

bool VideoReader::read_frame() { return true; }

void VideoReader::close() {
    if (pkt_) {
        av_packet_free(&pkt_);
    }
    if (fmt_) {
        avformat_close_input(&fmt_);
    }
}
