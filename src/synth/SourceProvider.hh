#pragma once

#include <ISourceProvider.hh>

namespace synth {

class SourceProvider final: public ISourceProvider {
    enum class Direction {
        Forward,
        Backward
    };

    int fd_;
    FrameInfo info_;
    Direction dir_;
    int64_t min_;
    int64_t max_;

  public:
    SourceProvider(const std::string &id, FrameFormat format);
    FrameInfo info() override;
    Frame nextFrame() override;
    ~SourceProvider() override;
};

}  // namespace
