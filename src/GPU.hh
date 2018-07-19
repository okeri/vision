#pragma once

#include <memory>
#include <Frame.hh>

class GPU {
    class Impl;
    std::unique_ptr<Impl> pImpl_;

  public:
    GPU(int device, const FrameInfo &info);
    ~GPU();
    Frame compute(const Frame &input);
};
