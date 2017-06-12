#pragma once

#include <memory>
#include <ISourceProvider.hh>

class SourceFactory {
    FrameInfo info_;

  public:
    SourceFactory(const FrameInfo &desired);
    std::vector<SourceInfo> availableSources();
    std::unique_ptr<ISourceProvider> createProvider(const SourceInfo &info);
};
