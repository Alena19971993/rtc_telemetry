#pragma once

#include <cstddef>
#include <cstdint>
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

    const std::byte *data() const;
    std::size_t size() const;
    std::int64_t pts() const;

private:
    void stream_metadata() const;

    AVFormatContext *fmt_ = nullptr;
    AVPacket *pkt_ = nullptr;
    int video_stream_ = -1;
};
