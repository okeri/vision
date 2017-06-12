#include <libv4l2.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <cstring>
#include <stdexcept>
#include <algorithm>
#include "SourceProvider.hh"

namespace capture {

SourceProvider::SourceProvider(const std::string &id,
                               const FrameInfo& info) :
        info_(info), buffer_(MAP_FAILED), jpeg_(info.format),
        startDumped_(false)
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
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.width = info.width;
    format.fmt.pix.height = info.height;
    if (ioctl(fd_, VIDIOC_S_FMT, &format) == -1) {
        throw std::runtime_error("Error: cannot set format" + id);
    }

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

Frame SourceProvider::nextFrame() {
    ioctl(fd_, VIDIOC_DQBUF, &bufInfo_);
    bufInfo_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufInfo_.memory = V4L2_MEMORY_MMAP;
    ioctl(fd_, VIDIOC_QBUF, &bufInfo_);
    return jpeg_.unpack(buffer_, bufInfo_.length);
}

void SourceProvider::dumpFrame(int stream) {
    if (!startDumped_) {
        write(stream, &info_, sizeof(info_));
        write(stream, &bufInfo_.length, sizeof(bufInfo_.length));
        startDumped_ = true;
    }
    ioctl(fd_, VIDIOC_DQBUF, &bufInfo_);
    bufInfo_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufInfo_.memory = V4L2_MEMORY_MMAP;
    ioctl(fd_, VIDIOC_QBUF, &bufInfo_);
    write(stream, buffer_, bufInfo_.length);
}

FrameInfo SourceProvider::info() {
    return info_;
}

}  // namespace
