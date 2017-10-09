#include <CL/cl.hpp>
#include <stdexcept>
#include <iostream>
#include <kernel_convert.hh>
#include <kernel_median.hh>
#include <kernel_orb.hh>
#include <TimeCounter.hh>
#include "GPU.hh"

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
    cl::Buffer allfeatures_;
    cl::Buffer features_;
    const int channels_ = 3;
    size_t square_;
    size_t outputSize_;
    TimeCounter counter_;
    std::vector<uint8_t> rgbBuffer_;
    std::vector<int16_t> featureBuffer_;

  public:
    Impl(int device, const FrameInfo &info)
            : info_(info),
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
        if (program.build({default_device}, "-DFAST_POINTS=11 -DFAST_THRESHOLD=10") != CL_SUCCESS) {
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
        features_ = cl::Buffer(context, CL_MEM_READ_WRITE, featureBuffer_.size() *
                               sizeof(int16_t));
        allfeatures_ = cl::Buffer(context, CL_MEM_READ_WRITE, featureBuffer_.size() *
                                  sizeof(int16_t));
        grayscale_ = cl::Buffer(context, CL_MEM_READ_WRITE, square_);
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
                convertOperation(yuyv2rgb_, square_ >> 1,
                                 info_.width >> 1, input, rgb_);
                convertOperation(yuyv2gs_, square_ >> 1,
                                  info_.width >> 1, input, grayscale_);
                haveGrayscale = true;
                break;

            case FrameFormat::RGBA:
            case FrameFormat::RGBX:
                convertOperation(rgba2rgb_, square_,
                                 1, input, rgb_);
                break;

            case FrameFormat::BGR:
                convertOperation(bgr2rgb_, square_,
                                 1, input, rgb_);
                break;

            case FrameFormat::ARGB:
                convertOperation(argb2rgb_, square_,
                                 1, input, rgb_);
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

        // make median filtration to reduce noise and feature count
        median_.setArg(0, grayscale_);
        median_.setArg(1, input_);
        median_.setArg(2, 1);
        median_.setArg(3, 5 /*kernelSize*/);
        queue_.enqueueNDRangeKernel(
            median_, cl::NullRange, cl::NDRange(info_.width, info_.height),
            cl::NDRange(1, 1));

        // detect corners
        fast_.setArg(0, input_);
        fast_.setArg(1, allfeatures_);
        queue_.enqueueNDRangeKernel(
            fast_, cl::NDRange(3, 3), cl::NDRange(info_.width - 6, info_.height - 6),
            cl::NDRange(1, 1));

        // queue_.enqueueReadBuffer(features_, CL_TRUE, 0, rgbBuffer_.size(),
        //                          rgbBuffer_.data());
        // uint16_t *f = (uint16_t*)rgbBuffer_.data();
        queue_.enqueueReadBuffer(allfeatures_, CL_TRUE, 0, featureBuffer_.size() *
                                 sizeof(uint32_t),
                                 featureBuffer_.data());

        // non-max supression
        fast_nm_.setArg(0, allfeatures_);
        fast_nm_.setArg(1, features_);
        queue_.enqueueNDRangeKernel(
            fast_nm_, cl::NullRange, cl::NDRange(info_.width, info_.height),
            cl::NDRange(1, 1));


        queue_.enqueueReadBuffer(features_, CL_TRUE, 0, featureBuffer_.size() *
                                 sizeof(int16_t),
                                 featureBuffer_.data());
        int16_t *f = (int16_t*)featureBuffer_.data();

        // count
        // int cnt = 0;
        // for (auto x = 0; x < info_.width ; ++x) {
        //     for (auto y = 0; y < info_.height ; ++y) {
        //         if (f[y * 640 + x] && (x > 3 && y > 3) && x < info_.width - 3 && y < info_.height < 3) {
        //             std::cout << x << ":" << y  << " = " << f[y * 640 + x] <<  std::endl;
        //         }
        //     }
        // }
        //        std::cout << "======" << std::endl;


        // draw corners
        draw_.setArg(0, features_);
        draw_.setArg(1, rgb_);
        queue_.enqueueNDRangeKernel(draw_, cl::NullRange, cl::NDRange(square_),
                                    cl::NDRange(1));


        // std::cout << (int)cornerBuffer_[0] << "|"
        //           << (int)cornerBuffer_[info_.width * 2] << "|"
        //           << (int)cornerBuffer_[info_.width + 2] << "|"
        //           << (int)cornerBuffer_[info_.width * 3] << "|"
        //           << (int)cornerBuffer_[info_.width + 3] << "|"
        //           << (int)cornerBuffer_[info_.width * 3 + 3] << "|"
        //           << std::endl;
        //        std::cout << cnt << std::endl;
        // read back
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
