#pragma once

#include <Frame.hh>
#include "vec.hh"

using Features = std::vector<Vec2i>;

Features getFeatures(const FrameInfo &info, const Frame &frame);
