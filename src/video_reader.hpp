#pragma once

#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class VideoReader {
public:
    VideoReader() = default;
    ~VideoReader();

    VideoReader(const VideoReader &) = delete;
    VideoReader &operator=(const VideoReader &) = delete;

    bool open(const std::string &path);
    bool read_frame();
    void close();

private:
    AVFormatContext *fmt_ = nullptr;
    AVPacket *pkt_ = nullptr;
};
