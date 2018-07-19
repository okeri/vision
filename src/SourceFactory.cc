#include "SourceFactory.hh"
#include <experimental/filesystem>
#include "capture/SourceProvider.hh"
#include "synth/SourceProvider.hh"

namespace fs = std::experimental::filesystem;

SourceFactory::SourceFactory(const FrameInfo &desired) : info_(desired) {
}

std::vector<SourceInfo> SourceFactory::availableSources() {
    std::vector<SourceInfo> result;
    auto appendIfExists = [&result] (const std::string& path, SourceType type) {
        if (fs::exists(path)) {
            for(auto& source: fs::directory_iterator(path)) {
                result.emplace_back(type, source.path());
            }
        }
    };
    appendIfExists("/dev/v4l/by-id", SourceType::Capture);
    appendIfExists("data", SourceType::Synthetic);
    fs::create_directory("data");
    return result;
}

std::unique_ptr<ISourceProvider> SourceFactory::createProvider(
    const SourceInfo &info) {
    switch (info.type) {
        case SourceType::Capture:
            return std::make_unique<capture::SourceProvider>(
                info.id, info_);

        case SourceType::Synthetic:
            return std::make_unique<synth::SourceProvider>(
                info.id, info_.format);
    }
    throw std::runtime_error("Unknown source type");
}
