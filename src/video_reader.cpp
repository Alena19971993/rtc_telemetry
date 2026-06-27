#include "video_reader.hpp"

#include <iostream>

VideoReader::~VideoReader() { close(); }

bool VideoReader::open(const std::string &path) {
    if (avformat_open_input(&fmt_, path.c_str(), nullptr, nullptr) < 0) {
        std::cout << "Failed to open video: " << path << std::endl;
        return false;
    }
    if (avformat_find_stream_info(fmt_, nullptr) < 0) {
        std::cout << "Failed to find stream info" << std::endl;
        close();
        return false;
    }

    video_stream_ = av_find_best_stream(fmt_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_ < 0) {
        std::cout << "No video stream found" << std::endl;
        close();
        return false;
    }

    pkt_ = av_packet_alloc();
    if (!pkt_) {
        std::cout << "Failed to allocate packet" << std::endl;
        close();
        return false;
    }

    std::cout << "Video stream found: " << path << std::endl;
    stream_metadata();
    return true;
}

void VideoReader::stream_metadata() const {
    AVStream *st = fmt_->streams[video_stream_];
    AVCodecParameters *par = st->codecpar;
    std::cout << "codec: " << avcodec_get_name(par->codec_id) << std::endl;
    std::cout << "resolution: " << par->width << "x" << par->height << std::endl;

    AVRational fr = st->avg_frame_rate;
    if (fr.den != 0) {
        std::cout << "fps: " << av_q2d(fr) << std::endl;
    }

    std::cout << "duration: " << static_cast<double>(fmt_->duration) / AV_TIME_BASE << " s"
              << std::endl;
}

bool VideoReader::read_frame() {
    if (!fmt_ || !pkt_) {
        return false;
    }
    av_packet_unref(pkt_);

    while (av_read_frame(fmt_, pkt_) >= 0) {
        if (pkt_->stream_index == video_stream_) {
            return true;
        }
        av_packet_unref(pkt_);
    }
    return false;
}

void VideoReader::close() {
    if (pkt_) {
        av_packet_free(&pkt_);
    }
    if (fmt_) {
        avformat_close_input(&fmt_);
    }
    video_stream_ = -1;
}

const std::byte *VideoReader::data() const {
    return pkt_ ? reinterpret_cast<const std::byte *>(pkt_->data) : nullptr;
}

std::size_t VideoReader::size() const { return pkt_ ? static_cast<std::size_t>(pkt_->size) : 0; }

std::int64_t VideoReader::pts() const { return pkt_ ? pkt_->pts : AV_NOPTS_VALUE; }
