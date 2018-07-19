#pragma once

class FrameCounter {
    uint32_t frames_;

  public:
    FrameCounter();
    FrameCounter& operator++(int);
    uint32_t frames();
};
