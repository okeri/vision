#pragma once

#include <ISourceProvider.hh>

namespace synth {

class SourceProvider : public ISourceProvider {
    enum class Direction {
        Forward,
        Backward
    };

    int fd_;
    FrameInfo info_;
    Direction dir_;
    uint64_t min_;
    uint64_t max_;

  public:
    SourceProvider(const std::string &id, FrameFormat format);
    virtual FrameInfo info() override;
    virtual Frame nextFrame() override;
    virtual ~SourceProvider() override;
};

}  // namespace
