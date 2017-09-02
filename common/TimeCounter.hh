#pragma once

#include <chrono>

class TimeCounter
{
  public:
    TimeCounter(size_t interval = 1000) :
            interval_(interval),
            count_(0), total_(0), average_(0),
            interval_start_(std::chrono::system_clock::now())
    {
    }

    void reset()
    {
        average_ = static_cast<float>(total_) /
                static_cast<float>(count_);
        count_ = 0;
        total_ = 0;
        interval_start_ = std::chrono::system_clock::now();
    }

    void start()
    {
        start_ = std::chrono::system_clock::now();
    }

    bool end()
    {
        count_++;
        auto now = std::chrono::system_clock::now();
        total_ += std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_).count();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                now - interval_start_).count() >= interval_)
        {
            reset();
            return true;
        }
        return false;
    }

    float average()
    {
        return average_;
    }

  private:
    int64_t interval_;
    int64_t count_;
    int64_t total_;
    float average_;
    std::chrono::time_point<std::chrono::system_clock> start_;
    std::chrono::time_point<std::chrono::system_clock> interval_start_;
};
