#include <chrono>
#include <iostream>
#include "FrameCounter.hh"

std::chrono::time_point<std::chrono::high_resolution_clock> start;

FrameCounter::FrameCounter() {
    start = std::chrono::high_resolution_clock::now();
}

FrameCounter& FrameCounter::operator++(int) {
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    static auto oldsecs = secs;
    static uint32_t frameCount = 0;
    double fps = static_cast<double>(frameCount++) / secs;
    if (secs != oldsecs) {
        std::cout << "FPS: " << fps  << std::endl;
        oldsecs = secs;
    }

    return *this;
}
