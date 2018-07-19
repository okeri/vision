#pragma once

#include <linux/videodev2.h>
#include <ISourceProvider.hh>

namespace capture {

class SourceProvider : public ISourceProvider {
    int fd_;
    FrameInfo info_;
    void *buffer_;
    struct v4l2_buffer bufInfo_;

    Frame frameFromYUYV(const unsigned char *buffer, size_t size);
  public:
    SourceProvider(const std::string &id, const FrameInfo& info);
    virtual FrameInfo info() override;
    virtual Frame nextFrame() override;
    virtual ~SourceProvider() override;
};

}  // namespace
