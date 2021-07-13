#include <getopt.h>
#include <fcntl.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <charconv>

#include "SourceFactory.hh"
#include "FrameCounter.hh"
#include "GPU.hh"
#include "Renderer.hh"
#include "EGLWindow.hh"

int main(int argc, char* argv[]) {
    int c;
    unsigned frames = 0;
    unsigned cameraIndex = 0;
    bool showSources = false;
    const char *data = nullptr, *record = nullptr;
    GPU::Params params{GPU::DisplayType::Features, 9, 20};

    while ((c = getopt(argc, argv, "c:d:f:glp:r:t:")) != -1) {
        switch (c) {
            case 'c':
                std::from_chars(optarg, optarg + strlen(optarg), cameraIndex);
                break;

            case 'l':
                showSources = true;
                break;

            case 'g':
                params.displayType = GPU::DisplayType::FeaturesGray;
                break;

            case 'p':
                std::from_chars(
                    optarg, optarg + strlen(optarg), params.fastPointNumber);
                break;

            case 't':
                std::from_chars(
                    optarg, optarg + strlen(optarg), params.fastThreshold);
                break;

            case 'd':
                data = optarg;
                break;

            case 'r':
                record = optarg;
                break;

            case 'f':
                std::from_chars(optarg, optarg + strlen(optarg), frames);
                break;

            default:
                std::cout << "usage:" << argv[0]
                          << " [-l] [-g] [-c index] [-d data] [-r data] [-p "
                             "point_number] [-t threshold]"
                          << std::endl;
                return 1;
        }
    }

    FrameInfo captureFormat{FrameFormat::YUYV, 640, 480};
    SourceFactory factory(captureFormat);
    auto sources = factory.availableSources();
    if (sources.empty()) {
        std::cout << "Cannot find any source" << std::endl;
        return 2;
    } else if (showSources) {
        for (const auto& source : sources) {
            std::cout << source.id << std::endl;
        }
        return 0;
    }

    bool synthetic = true;
    const char* id;
    std::unique_ptr<ISourceProvider> source;
    if (data == nullptr) {
        source = factory.createProvider(sources[cameraIndex]);
        id = sources[cameraIndex].id.c_str();
        synthetic = sources[cameraIndex].type == SourceType::Synthetic;
    } else {
        source =
            factory.createProvider(SourceInfo(SourceType::Synthetic, data));
        id = data;
    }

    FrameInfo info = source->info();
    std::cout << "Opening source " << id << " with resolution: " << info.width
              << "x" << info.height << std::endl;

    std::ofstream os;
    if (record != nullptr && !synthetic) {
        os.open(record);
        os.write(reinterpret_cast<char*>(&info), sizeof(info));
    }

    GPU gpu(0, info, params);

    const auto captureWidth = 640;
    const auto captureHeight = 480;

    EGLWindow window(captureWidth, captureHeight,
        [&source, &os, &gpu, &info, &frames, &window](
            uint32_t width, uint32_t height) {
            static Renderer render;
            static FrameCounter counter;
            static FrameInfo renderInfo = {
                FrameFormat::RGB, info.width, info.height};

            Frame frame = source->nextFrame();
            auto w = height * captureWidth / captureHeight;
            render.render(
                gpu.compute(frame), renderInfo, (width - w) / 2, w, height);
            if (os.is_open()) {
                os.write(reinterpret_cast<char*>(frame.data()), frame.size());
                std::cout << "." << std::flush;
            }
            counter++;

            if (frames != 0 && counter.frames() > frames) {
                window.stop();
            };
        });

    return window.loop();
}
