#include <libv4l2.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <TimeCounter.hh>
#include "SourceProvider.hh"

#include <iostream>
namespace capture {

SourceProvider::SourceProvider(const std::string &id,
                               const FrameInfo& info) :
        info_(info), buffer_(MAP_FAILED)
{
    fd_ = open(id.c_str(), O_RDWR);
    if (fd_ == -1) {
        throw std::runtime_error("Error: cannot open camera " + id);
    }

    struct v4l2_capability caps = {0};
    if (ioctl(fd_, VIDIOC_QUERYCAP, &caps) == -1 ||
        !(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        throw std::runtime_error("Error: cannot get camera capabilities" + id);
    }

    struct v4l2_format format;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.width = info.width;
    format.fmt.pix.height = info.height;
    if (ioctl(fd_, VIDIOC_S_FMT, &format) == -1) {
        throw std::runtime_error("Error: cannot set format" + id);
    }

#ifndef FORCE_FRAMERATE
    struct v4l2_streamparm streamparm;
    memset(&streamparm, 0, sizeof(streamparm));
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    streamparm.parm.capture.timeperframe.numerator = 1;
    streamparm.parm.capture.timeperframe.denominator = 30;
    if(ioctl(fd_, VIDIOC_S_PARM, &streamparm) != 0)
    {
        throw std::runtime_error("Error: cannot set fps)" + id);
    }
#endif

    struct v4l2_requestbuffers bufrequest;
    bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count = 1;

    if(ioctl(fd_, VIDIOC_REQBUFS, &bufrequest) == -1) {
        throw std::runtime_error("Error: cannot request buffer" + id);
    }

    memset(&bufInfo_, 0, sizeof(bufInfo_));
    bufInfo_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufInfo_.memory = V4L2_MEMORY_MMAP;
    if(ioctl(fd_, VIDIOC_QBUF, &bufInfo_) == -1) {
        throw std::runtime_error("Error: cannot query buffer" + id);
    }

    buffer_ = mmap(NULL, bufInfo_.length, PROT_READ, MAP_SHARED,
                               fd_, bufInfo_.m.offset);
    if (buffer_ == MAP_FAILED) {
        throw std::runtime_error("Error: failed access data" + id);
    }

    if (ioctl(fd_, VIDIOC_STREAMON, &bufInfo_.type) == -1) {
        throw std::runtime_error("Error: failed to start capture" + id);
    }
}

SourceProvider::~SourceProvider() {
    ioctl(fd_, VIDIOC_STREAMOFF, &bufInfo_.type);
    if (buffer_ != MAP_FAILED) {
        munmap(buffer_, bufInfo_.length);
    }
    close(fd_);
}


namespace {

void YUV2RGB(int y, int u, int v, unsigned char *result)
{
    auto clamp  = [] (int byte) -> unsigned char {
        return std::clamp(byte, 0, 255);
    };
    result[0] = clamp((int)(y + (1.402 * (v - 128))));
    result[1] = clamp((int)(y - 0.344 * (u - 128) -  0.714 * (v - 128)));
    result[2] = clamp((int)(y + (1.772 * (u - 128))));
}

}  // namespace

TimeCounter counter;
// this function also mirrors lines
Frame SourceProvider::frameFromYUYV(const unsigned char *buffer, size_t size) {
    static Frame frame(info_.size());
    counter.start();
    size_t rowSize = info_.rowSize();
    for (int i = rowSize - 3, j = 0, istop = -3; j < size; i -= 6, j += 4) {
        int Y1 = buffer[j + 0];
        int U = buffer[j + 1];
        int Y2 = buffer[j + 2];
        int V = buffer[j + 3];

        YUV2RGB(Y1, U, V, frame.data() + i);
        YUV2RGB(Y2, U, V, frame.data() + i - 3);

        if (i == istop) {
            istop += rowSize;
            i = istop + rowSize;
        }
    }
    if (counter.end()) {
            std::cout << "cpu convert:" << counter.average() << std::endl;
    }
    return frame;
}

Frame SourceProvider::nextFrame() {
    ioctl(fd_, VIDIOC_DQBUF, &bufInfo_);
    bufInfo_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufInfo_.memory = V4L2_MEMORY_MMAP;
    ioctl(fd_, VIDIOC_QBUF, &bufInfo_);

    switch (info_.format) {
        case FrameFormat::YUYV:
            return Frame(static_cast<uint8_t*>(buffer_),
                         static_cast<uint8_t*>(buffer_) + bufInfo_.length);

        case FrameFormat::RGB:
            return frameFromYUYV(static_cast<unsigned char*>(buffer_),
                                 bufInfo_.length);

        default:
            throw std::logic_error("Getting data in this Frame format is unimplemented");
    }
}

FrameInfo SourceProvider::info() {
    return info_;
}

}  // namespace
