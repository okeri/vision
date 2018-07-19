#pragma once

#include <linux/videodev2.h>
#include <ISourceProvider.hh>

namespace capture {

class SourceProvider final: public ISourceProvider {
    int fd_;
    FrameInfo info_;
    void *buffer_;
    struct v4l2_buffer bufInfo_;

  public:
    SourceProvider(const std::string &id, const FrameInfo& info);
    FrameInfo info() override;
    Frame nextFrame() override;
    ~SourceProvider() override;
};

}  // namespace
