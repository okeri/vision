#pragma once

#include <memory>
#include <Frame.hh>

class GPU {
    class Impl;
    std::unique_ptr<Impl> pImpl_;

  public:
    enum class DisplayType {
        Features,
        FeaturesGray,
    };

    struct Params {
        DisplayType displayType;
        unsigned fastPointNumber;
        unsigned fastThreshold;
    };

    GPU(int device, const FrameInfo& info, const Params& params);
    ~GPU();
    Frame compute(const Frame& input);
};
