#pragma once

#include <Frame.hh>

class Renderer {
  public:
    Renderer();
    void render(const Frame &frame, const FrameInfo &info);
};
