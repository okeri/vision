#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <string>

#include "SourceFactory.hh"
#include "FrameCounter.hh"
#include "EGLWindow.hh"
#include "Renderer.hh"

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

    FrameInfo captureFormat{FrameFormat::RGB, 640, 480};
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

    if (record != nullptr && !synthetic) {
        int file = open(record, O_CREAT | O_WRONLY | 0440);
        write(file, &info, sizeof(info));
        for (int i = 0; i < frames; ++i) {
            Frame frame = source->nextFrame();
            write(file, frame.data(), frame.size());
            std::cout << "." << std::flush;
        }
        fchmod(file, 0440);
        close(file);
        std::cout << std::endl << frames << " frames written" << std::endl;
        return 0;
    }

    EGLWindow window(captureFormat.width, captureFormat.height,
                     [&source, &info] () {
                         static Renderer render;
                         static FrameCounter counter;
                         render.render(source->nextFrame(), info);
                         counter++;
                     });

    window.loop();
    return 0;
}
