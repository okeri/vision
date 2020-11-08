#pragma once

#include <Frame.hh>

class Renderer {
  public:
    Renderer();
    void render(const Frame& frame, const FrameInfo& info, uint32_t offset,
        uint32_t width, uint32_t height);
};
