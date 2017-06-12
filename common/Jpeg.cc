#include <cstdio>
#include <jpeglib.h>
#include "Jpeg.hh"

class Jpeg::Impl {
    struct jpeg_decompress_struct info_;
    struct jpeg_error_mgr jerr;
    J_COLOR_SPACE fmt_;

    J_COLOR_SPACE convertFormat(FrameFormat fmt) {
        switch (fmt) {
            case FrameFormat::Grayscale:
                return JCS_GRAYSCALE;

            case FrameFormat::RGB:
                return JCS_RGB;

            case FrameFormat::RGBX:
                return JCS_EXT_RGBX;

            case FrameFormat::RGBA:
                return JCS_EXT_RGBA;

            default:
                return JCS_RGB;
        }
    }

  public:
    Impl(FrameFormat fmt) {
        fmt_ = convertFormat(fmt);
        info_.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&info_);
    }

    ~Impl() {
        jpeg_destroy_decompress(&info_);
    }

    Frame unpack(const void *buffer, uint32_t len) {
        Frame frame;
        jpeg_mem_src(&info_, static_cast<const uint8_t*>(buffer), len);
        if (jpeg_read_header(&info_, TRUE)) {
            info_.out_color_space = fmt_;
            jpeg_start_decompress(&info_);
            JDIMENSION stride = info_.output_width * info_.output_components;
            frame.resize(stride * info_.output_height);
            while (info_.output_scanline < info_.output_height) {
                uint8_t *buffer_array[1];
                buffer_array[0] = frame.data() + (info_.output_scanline *
                                                  stride);
                jpeg_read_scanlines(&info_, buffer_array, 1);
            }
            jpeg_finish_decompress(&info_);

        }
        return frame;
    }
};

Jpeg::Jpeg(FrameFormat fmt) : pImpl_(std::make_unique<Jpeg::Impl>(fmt)) {
}

Jpeg::~Jpeg() {
}

Frame Jpeg::unpack(const void *buffer, uint32_t len) {
    return pImpl_->unpack(buffer, len);
}
