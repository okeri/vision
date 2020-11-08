#pragma once

#include <memory>
#include <functional>
#include <Single.hh>

class EGLWindow : public Single {
    using RenderFn = std::function<void(uint32_t, uint32_t)>;
    class Impl;
    std::unique_ptr<Impl> pImpl_;

  public:
    EGLWindow(int width, int height, RenderFn render);
    ~EGLWindow();
    int loop();
    void stop();
};
