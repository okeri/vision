#pragma once

#include <memory>
#include <Frame.hh>

class Jpeg {
    class Impl;
    std::unique_ptr<Impl> pImpl_;

  public:
    explicit Jpeg(FrameFormat fmt);
    ~Jpeg();
    Frame unpack(const void *buffer, uint32_t len);
};
