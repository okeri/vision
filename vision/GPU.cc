#include <CL/cl.hpp>
#include <stdexcept>
#include <iostream>
#include <kernel_convert.hh>
#include <kernel_median.hh>
#include <kernel_orb.hh>
#include <TimeCounter.hh>
#include "GPU.hh"

using Feature = uint16_t;

class GPU::Impl {
    FrameInfo info_;
    cl::CommandQueue queue_;
    cl::Kernel median_;
    cl::Kernel yuyv2rgb_;
    cl::Kernel argb2rgb_;
    cl::Kernel rgba2rgb_;
    cl::Kernel bgr2rgb_;
    cl::Kernel yuyv2gs_;
    cl::Kernel rgb2gs_;
    cl::Kernel fast_;
    cl::Kernel fast_nm_;
    cl::Kernel draw_;

    cl::Buffer input_;
    cl::Buffer rgb_;
    cl::Buffer grayscale_;
    cl::Image2D gsimage_;
    cl::Image2D filtered_;
    cl::Image2D allfeatures_;
    cl::Image2D features_;
    const int channels_ = 3;
    size_t square_;
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
            featureBuffer_(square_)
    {
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
        if (program.build({default_device}, "-DFAST_POINTS=11 -DFAST_THRESHOLD=20") != CL_SUCCESS) {
            throw std::runtime_error(
                "Error building: " + program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(
                    default_device));
        }

        std::cout << "ok" << std::endl;
        queue_ =  cl::CommandQueue(context, default_device);
        yuyv2rgb_ = cl::Kernel(program, "yuyv2rgb");
        yuyv2gs_  = cl::Kernel(program, "yuyv2gs");
        argb2rgb_ = cl::Kernel(program, "argb2rgb");
        rgba2rgb_ = cl::Kernel(program, "rgba2rgb");
        bgr2rgb_ = cl::Kernel(program, "bgr2rgb");
        rgb2gs_   = cl::Kernel(program, "rgb2gs");
        median_   = cl::Kernel(program, "median_mean");
        fast_   = cl::Kernel(program, "fast");
        fast_nm_   = cl::Kernel(program, "fastNonMax");
        draw_   = cl::Kernel(program, "draw");

        input_ = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR,
                            std::max(outputSize_, info.size()));
        rgb_ = cl::Buffer(context, CL_MEM_USE_HOST_PTR, outputSize_,
                          rgbBuffer_.data());

        grayscale_ = cl::Buffer(context, CL_MEM_READ_WRITE, square_);
        gsimage_ = cl::Image2D(context, CL_MEM_READ_ONLY,
                                 cl::ImageFormat(CL_INTENSITY, CL_UNSIGNED_INT8),
                                 info_.width, info_.height);

        filtered_ = cl::Image2D(context, CL_MEM_WRITE_ONLY,
                                 cl::ImageFormat(CL_INTENSITY, CL_UNSIGNED_INT8),
                                 info_.width, info_.height);

        features_ = cl::Image2D(context, CL_MEM_READ_WRITE,
                                 cl::ImageFormat(CL_A, CL_UNSIGNED_INT16),
                                 info_.width, info_.height);

        allfeatures_ = cl::Image2D(context, CL_MEM_READ_WRITE,
                                 cl::ImageFormat(CL_A, CL_UNSIGNED_INT16),
                                 info_.width, info_.height);
    }

    inline void convertOperation(cl::Kernel &convertKernel,
                                 unsigned global, unsigned local,
                                 const Frame &input, cl::Buffer out) {
        convertKernel.setArg(0, input_);
        convertKernel.setArg(1, out);
        queue_.enqueueWriteBuffer(input_, CL_TRUE, 0, input.size(), input.data());

        queue_.enqueueNDRangeKernel(convertKernel, cl::NullRange,
                                    cl::NDRange(global),
                                    cl::NDRange(local));
    }

    Frame compute(const Frame &input) {
        counter_.start();
        bool haveGrayscale = false;
        // convert to rgb is necessary
        switch (info_.format) {
            case FrameFormat::RGB:
                queue_.enqueueWriteBuffer(rgb_, CL_TRUE, 0, input.size(), input.data());
                break;

            case FrameFormat::YUYV:
                // 4 yuyv bytes = 2 rgb pixels => iteration count = w * h / 2
                // convertOperation(yuyv2rgb_, square_ >> 1,
                //                  info_.width >> 1, input, rgb_);
                convertOperation(yuyv2gs_, square_ >> 1,
                                 info_.width >> 1, input, grayscale_);

                haveGrayscale = true;
                break;

            case FrameFormat::RGBA:
            case FrameFormat::RGBX:
                convertOperation(rgba2rgb_, square_, 1, input, rgb_);
                break;

            case FrameFormat::BGR:
                convertOperation(bgr2rgb_, square_, 1, input, rgb_);
                break;

            case FrameFormat::ARGB:
                convertOperation(argb2rgb_, square_, 1, input, rgb_);
                break;

            default:
                throw std::logic_error("Unsupported format");
                break;
        }

        if (!haveGrayscale) {
            // convert to grayscale
            rgb2gs_.setArg(0, rgb_);
            rgb2gs_.setArg(1, grayscale_);
            queue_.enqueueNDRangeKernel(rgb2gs_, cl::NullRange,
                                        cl::NDRange(square_),
                                        cl::NDRange(1));
        }
        cl::size_t<3> dimensions;
        dimensions[0] = info_.width;
        dimensions[1] = info_.height;
        dimensions[2] = 1;
        queue_.enqueueCopyBufferToImage(grayscale_, gsimage_, 0,
                                        cl::size_t<3>(),
                                        dimensions);

        // make median filtration to reduce noise and feature count
        median_.setArg(0, gsimage_);
        median_.setArg(1, filtered_);
        median_.setArg(2, 5 /*kernelSize*/);
        queue_.enqueueNDRangeKernel(
            median_, cl::NullRange, cl::NDRange(info_.width, info_.height),
            cl::NDRange(1, 1));

        // detect corners
        fast_.setArg(0, filtered_);
        fast_.setArg(1, allfeatures_);
        queue_.enqueueNDRangeKernel(
            fast_, cl::NDRange(3, 3), cl::NDRange(info_.width - 6, info_.height - 6),
            cl::NDRange(1, 1));

        // non-max supression
        fast_nm_.setArg(0, allfeatures_);
        fast_nm_.setArg(1, features_);
        queue_.enqueueNDRangeKernel(
            fast_nm_, cl::NullRange, cl::NDRange(info_.width, info_.height),
            cl::NDRange(1, 1));


        // draw corners
        draw_.setArg(0, features_);
        draw_.setArg(1, rgb_);
        queue_.enqueueNDRangeKernel(draw_, cl::NullRange, cl::NDRange(square_),
                                    cl::NDRange(1));


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
