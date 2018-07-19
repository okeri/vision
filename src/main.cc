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

int main(int argc, char *argv[]) {
    int c;
    unsigned frames = 0;
    unsigned cameraIndex = 0;
    bool showSources = false;
    const char *data = nullptr, *record = nullptr;
    while ((c = getopt(argc, argv, "c:d:f:lr:")) != -1) {
        switch (c) {
            case 'c':
                std::from_chars(optarg, optarg + strlen(optarg), cameraIndex);
                break;

            case 'l':
                showSources = true;
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
                std::cout << "usage:" << argv[0] << " [-l] [-c index] [-d data] [-r data]" << std::endl;
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
    const char *id;
    std::unique_ptr<ISourceProvider> source;
    if (data == nullptr) {
        source = factory.createProvider(sources[cameraIndex]);
        id = sources[cameraIndex].id.c_str();
        synthetic = sources[cameraIndex].type == SourceType::Synthetic;
    } else {
        source = factory.createProvider(
            SourceInfo(SourceType::Synthetic, data));
        id = data;
    }

    FrameInfo info = source->info();
    std::cout << "Opening source " << id
              << " with resolution: "
              << info.width << "x" << info.height
              << std::endl;

    std::ofstream os;
    if (record != nullptr && !synthetic) {
        os.open(record);
        os.write(reinterpret_cast<char *>(&info), sizeof(info));
    }

    GPU gpu(0, info);

    EGLWindow window(captureFormat.width, captureFormat.height,
                     [&source, &info, &os, &gpu, &frames, &window] () {
                         static Renderer render;
                         static FrameCounter counter;
                         static FrameInfo renderInfo =
                                 {FrameFormat::RGB, info.width, info.height};
                         Frame frame = source->nextFrame();
                         render.render(gpu.compute(frame), renderInfo);

                         if (os.is_open()) {
                             os.write(reinterpret_cast<char *>(frame.data()), frame.size());
                             std::cout << "." << std::flush;
                         }
                         counter++;

                         if (frames != 0 && counter.frames() > frames) {
                             window.stop();
                         };
                     });

    return window.loop();
}
