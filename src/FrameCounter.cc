#include <chrono>
#include <iostream>
#include "FrameCounter.hh"

std::chrono::time_point<std::chrono::high_resolution_clock> start;

FrameCounter::FrameCounter() : frames_(0) {
    start = std::chrono::high_resolution_clock::now();
}

uint32_t FrameCounter::frames() {
    return frames_;
}

FrameCounter& FrameCounter::operator++(int) {
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    static auto oldsecs = secs;
    double fps = static_cast<double>(frames_++) / secs;
    if (secs != oldsecs) {
        std::cout << "FPS: " << fps  << std::endl;
        oldsecs = secs;
    }

    return *this;
}
