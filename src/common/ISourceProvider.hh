#pragma once

#include <string>

#include <Frame.hh>

enum class SourceType {
    Capture,
    Synthetic
};

struct SourceInfo {
    SourceType type;
    std::string id;
    SourceInfo(SourceType t, const std::string& i) :
            type(t), id(i) {
    }
};

class ISourceProvider {
  public:
    virtual FrameInfo info() = 0;
    virtual Frame nextFrame() = 0;
    virtual ~ISourceProvider() {}
};
