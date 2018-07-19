#include <stdexcept>
#include <iostream>
#include <map>
#include <string_view>
#include <charconv>
#include <CL/cl.hpp>

#include <kernel_convert.hh>
#include <kernel_median.hh>
#include <kernel_orb.hh>
#include <TimeCounter.hh>
#include "GPU.hh"

using Feature = uint16_t;

class GPU::Impl {
    FrameInfo info_;
    cl::CommandQueue queue_;
    std::map<std::string, cl::Kernel, std::less<>> kernels_;

    cl::Buffer input_;
    cl::Buffer rgb_;

    cl::Image2D gsimage_;
    cl::Image2D filtered_;
    cl::Image2D allfeatures_;
    cl::Image2D features_;
    const int channels_ = 3;
    uint32_t square_;
    size_t outputSize_;
    TimeCounter counter_;
    std::vector<uint8_t> rgbBuffer_;
    std::vector<Feature> featureBuffer_;

  public:
    Impl(int device, const FrameInfo &info) :
            info_(info),
            square_(info.width * info.height),
            outputSize_(square_ * channels_),
            rgbBuffer_(outputSize_),
            featureBuffer_(square_) {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        if (platforms.empty()) {
            throw std::runtime_error("No OpenCL platforms found");
        }
        std::vector<cl::Device> devices;

        platforms[0].getDevices(CL_DEVICE_TYPE_DEFAULT, &devices);

        if (devices.empty()) {
            throw std::runtime_error("No OpenCL devices found");
        }
        cl::Device default_device = devices[device];
        cl::Context context({default_device});
        cl::Program::Sources sources = {
            {kernel_convert_src, strlen(kernel_convert_src)},
            {kernel_median_src, strlen(kernel_median_src)},
            {kernel_orb_src, strlen(kernel_orb_src)}
        };

        cl::Program program(context, sources);
        constexpr auto medianKernelSize = 5;
        auto defines = std::string("-DFAST_POINTS=10 -DFAST_THRESHOLD=12 -DMEDIAN_WINDOW_SIZE=") +
                std::to_string(medianKernelSize) +
                " -DMEDIAN_WINDOW_SIZE=" + std::to_string(medianKernelSize * medianKernelSize) +
                " -DMEDIAN_KERNEL_OFFSET=" + std::to_string(medianKernelSize / 2);
        auto compiled = program.build({default_device}, defines.c_str());
        std::cout << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device)
                  << std::endl;

        if (compiled != CL_SUCCESS) {
            throw std::runtime_error("Error building opencl code");
        }

        queue_ =  cl::CommandQueue(context, default_device);
        const std::string_view kernels[] = {
            "argb2rgb", "rgba2rgb", "bgr2rgb", "yuyv2rgb", "yuyv2gs","rgb2gs",
            "gs2r", "median", "median_mean",
            "fast", "fastNonMax", "draw", ""};

        for (auto i = 0; kernels[i] != ""; ++i) {
            kernels_.emplace(kernels[i], cl::Kernel(program, kernels[i].data()));
        }

        input_ = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR,
                            std::max(outputSize_, info.size()));
        rgb_ = cl::Buffer(context, CL_MEM_USE_HOST_PTR, outputSize_,
                          rgbBuffer_.data());

        gsimage_ = cl::Image2D(context, CL_MEM_READ_WRITE,
                                 cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8),
                                 info_.width, info_.height);

        filtered_ = cl::Image2D(context, CL_MEM_READ_WRITE,
                                 cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8),
                                 info_.width, info_.height);

        features_ = cl::Image2D(context, CL_MEM_READ_WRITE,
                                cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT16),
                                 info_.width, info_.height);

        allfeatures_ = cl::Image2D(context, CL_MEM_READ_WRITE,
                                 cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT16),
                                 info_.width, info_.height);
    }

    template <class Destination, class Range>
    void convertOperation(cl::Kernel &convertKernel,
                          const Range &global,
                          const Range &local,
                          Destination out) {
        convertKernel.setArg(0, input_);
        convertKernel.setArg(1, out);

        auto ret = queue_.enqueueNDRangeKernel(convertKernel, cl::NullRange,
                                               cl::NDRange(global),
                                               cl::NDRange(local));
        if (ret != CL_SUCCESS) {
            std::cout << "error of kernel call " << ret << std::endl;
        }
    }

    Frame compute(const Frame &input) {
        counter_.start();
        bool haveGrayscale = false;

        queue_.enqueueWriteBuffer(input_, CL_TRUE, 0, input.size(), input.data());
        switch (info_.format) {
            case FrameFormat::RGB:
                queue_.enqueueWriteBuffer(rgb_, CL_TRUE, 0, input.size(), input.data());
                break;

            case FrameFormat::YUYV:
                // 4 yuyv bytes = 2 rgb pixels => iteration count = w * h / 2
                convertOperation(kernels_["yuyv2rgb"], square_ >> 1,
                                 info_.width >> 1, rgb_);
                convertOperation(kernels_["yuyv2gs"], square_ >> 1,
                                 info_.width >> 1, gsimage_);
                haveGrayscale = true;

                break;

            case FrameFormat::RGBA:
            case FrameFormat::RGBX:
                convertOperation(kernels_["rgba2rgb"], square_, 1u, rgb_);
                break;

            case FrameFormat::BGR:
                convertOperation(kernels_["bgr2rgb"], square_, 1u, rgb_);
                break;

            case FrameFormat::ARGB:
                convertOperation(kernels_["argb2rgb"], square_, 1u, rgb_);
                break;

            default:
                throw std::logic_error("Unsupported format");
                break;
        }

        if (!haveGrayscale) {
            // convert to grayscale
            kernels_["rgb2gs"].setArg(0, rgb_);
            kernels_["rgb2gs"].setArg(1, gsimage_);

            queue_.enqueueNDRangeKernel(
                kernels_["rgb2gs"], cl::NullRange, cl::NDRange(square_),
                cl::NDRange(info_.width));
        }

        // make median filtration to reduce noise and feature count
        kernels_["median_mean"].setArg(0, gsimage_);
        kernels_["median_mean"].setArg(1, filtered_);
        queue_.enqueueNDRangeKernel(
            kernels_["median_mean"], cl::NullRange, cl::NDRange(info_.width, info_.height),
            cl::NDRange(1, 1));

        // detect corners
        kernels_["fast"].setArg(0, filtered_);
        kernels_["fast"].setArg(1, allfeatures_);
        queue_.enqueueNDRangeKernel(
            kernels_["fast"], cl::NDRange(3, 3), cl::NDRange(info_.width - 6, info_.height - 6),
            cl::NDRange(1, 1));

        // non-max supression
        kernels_["fastNonMax"].setArg(0, allfeatures_);
        kernels_["fastNonMax"].setArg(1, features_);
        queue_.enqueueNDRangeKernel(
            kernels_["fastNonMax"], cl::NullRange, cl::NDRange(info_.width, info_.height),
            cl::NDRange(1, 1));

        // draw corners
        kernels_["draw"].setArg(0, features_);
        kernels_["draw"].setArg(1, rgb_);
        queue_.enqueueNDRangeKernel(
            kernels_["draw"], cl::NullRange, cl::NDRange(info_.width, info_.height),
            cl::NDRange(1, 1));

        auto debug = [this] () {
            kernels_["gs2r"].setArg(0, gsimage_);
            kernels_["gs2r"].setArg(1, rgb_);
            queue_.enqueueNDRangeKernel(
                kernels_["gs2r"], cl::NullRange, cl::NDRange(info_.width, info_.height),
                cl::NDRange(1, 1));
        };

        queue_.enqueueReadBuffer(rgb_, CL_TRUE, 0, rgbBuffer_.size(),
                                 rgbBuffer_.data());

        queue_.finish();

        if (counter_.end()) {
            std::cout << "compute:" << counter_.average() << std::endl;
        }
        return rgbBuffer_;
    }
};

GPU::GPU(int device, const FrameInfo &info) :
        pImpl_(std::make_unique<GPU::Impl>(device, info)) {
}

GPU::~GPU() {
}

Frame GPU::compute(const Frame &input) {
    return pImpl_->compute(input);
}
