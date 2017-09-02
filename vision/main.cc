#include <getopt.h>
#include <fcntl.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

#include "SourceFactory.hh"
#include "FrameCounter.hh"
#include "Computer.hh"
#include "Renderer.hh"
#include "EGLWindow.hh"

int main(int argc, char *argv[]) {
    int c;
    int frames = 128;
    bool showSources = false;
    const char *data = nullptr, *record = nullptr;
    while ((c = getopt(argc, argv, "d:f:lr:")) != -1) {
        switch (c) {
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
                frames = std::stoi(optarg);
                break;

            default:
                std::cout << "usage:" << argv[0] << "[-l] [-d data] [-r data]" << std::endl;
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
        source = factory.createProvider(sources[0]);
        id = sources[0].id.c_str();
        synthetic = sources[0].type == SourceType::Synthetic;
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

    Computer computer(0, info);

    EGLWindow window(captureFormat.width, captureFormat.height,
                     [&source, &info, &os, &computer] () {
                         static Renderer render;
                         static FrameCounter counter;
                         Frame frame = source->nextFrame();

                         render.render(computer.compute(frame), info);

                         if (os.is_open()) {
                             os.write(reinterpret_cast<char *>(frame.data()), frame.size());
                             std::cout << "." << std::flush;
                         }
                         counter++;
                     });

    return window.loop();
}
