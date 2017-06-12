#pragma once

#include <linux/videodev2.h>
#include <Jpeg.hh>
#include <ISourceProvider.hh>

namespace capture {

class SourceProvider : public ISourceProvider {
    int fd_;
    FrameInfo info_;
    void *buffer_;
    struct v4l2_buffer bufInfo_;
    Jpeg jpeg_;
    bool startDumped_;

  public:
    SourceProvider(const std::string &id, const FrameInfo& info);
    virtual FrameInfo info() override;
    virtual Frame nextFrame() override;
    virtual void dumpFrame(int stream) override;
    virtual ~SourceProvider() override;
};

}  // namespace
