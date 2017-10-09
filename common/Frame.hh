#pragma once

#include <cstdint>
#include <vector>

using Frame = std::vector<uint8_t>;

enum class FrameFormat {
    Grayscale,
    RGB,
    RGBX,
    RGBA,
    YUYV,
    ARGB,
    BGR
};

struct FrameInfo {
    FrameFormat format;
    uint32_t width;
    uint32_t height;

    FrameInfo() = default;
    FrameInfo(FrameFormat f, uint32_t w, uint32_t h) :
            format(f), width(w), height(h) {
    }

    FrameInfo& operator=(const FrameInfo &rhs) {
        format = rhs.format;
        width = rhs.width;
        height = rhs.height;
        return *this;
    }

    int bpp() const {
        switch (format) {
            case FrameFormat::Grayscale:
                return 1;

            case FrameFormat::YUYV:
                return 2;

            case FrameFormat::RGB:
                return 3;

            default:
                return 4;
        }
    }

    size_t rowSize() const {
        return bpp() * width;
    }

    size_t size() const {
        return rowSize() * height;
    }
};
