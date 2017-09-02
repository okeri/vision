#pragma once

#include <memory>
#include <Frame.hh>

class Computer {
    class Impl;
    std::unique_ptr<Impl> pImpl_;

  public:
    Computer(int device, const FrameInfo &info);
    ~Computer();
    Frame compute(const Frame &input);
};
