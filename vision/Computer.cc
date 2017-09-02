#include <CL/cl.hpp>
#include <stdexcept>
#include <iostream>
#include <kernel_convert.hh>
#include <kernel_median.hh>
#include <TimeCounter.hh>
#include "Computer.hh"

/*

  const int kernelSize = 3;
                         const int channels = 3;
                         Frame kernel(kernelSize * kernelSize);

                         Frame frame = source->nextFrame();
#ifdef CPU_MEDIAN_FILTER
                         int edge  = kernelSize / 2;
                         for (int j = edge; j < info.height - 2 * edge; ++j) {
                             for (int i = edge; i < info.width - 2 * edge; ++i) {
                                 for (int c = 0; c < channels; ++c) {
                                     for (int r = 0; r < kernelSize; ++r) {
                                         uint8_t *row = frame.data() + ((j - edge + r) *
                                                                        info.rowSize());
                                         for (int p = 0; p < kernelSize; ++p) {
                                             kernel[r * kernelSize + p] =
                                                     row[(p + i - edge) * channels + c];
                                         }
                                     }
                                     std::sort(kernel.begin(), kernel.end());
                                     frame[(j * info.width + i) * channels + c] = kernel[4];
                                 }

                             }
                         }
#endif
*/

class Computer::Impl {
    FrameInfo info_;
    cl::CommandQueue queue_;
    cl::Kernel convert_;
    cl::Kernel median_;
    cl::Buffer input_;
    cl::Buffer output_;
    const int channels_ = 3;
    size_t outputSize_;
    TimeCounter counter_;
    std::vector<uint8_t> outputBuffer_;

  public:
    Impl(int device, const FrameInfo &info)
            : info_(info),
              outputSize_(info_.width * info_.height * channels_),
              outputBuffer_(outputSize_) {

        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        if (platforms.empty()) {
            throw std::runtime_error("No OpenCL platforms found");
        }
        std::vector<cl::Device> devices;

        platforms[0].getDevices(CL_DEVICE_TYPE_DEFAULT, &devices);
        if (devices.empty()) {
            throw std::runtime_error("No OpenCL platforms found");
        }
        cl::Device default_device = devices[device];
        cl::Context context({default_device});
        cl::Program::Sources sources = {
            {kernel_convert_src, strlen(kernel_convert_src)},
            {kernel_median_src, strlen(kernel_median_src)}
        };

        cl::Program program(context, sources);
        if (program.build({default_device}) != CL_SUCCESS) {
            throw std::runtime_error(
                "Error building: " + program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(
                    default_device));
        }

        queue_ =  cl::CommandQueue(context, default_device);
        convert_   = cl::Kernel(program, "convert");
        median_   = cl::Kernel(program, "median");
        input_ = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR, outputSize_);
        output_ = cl::Buffer(context, CL_MEM_USE_HOST_PTR, outputSize_,
                             outputBuffer_.data());
    }

    Frame compute(const Frame &input) {
        counter_.start();

        if (info_.format == FrameFormat::YUYV) {
            convert_.setArg(0, input_);
            convert_.setArg(1, output_);
            queue_.enqueueWriteBuffer(input_, CL_TRUE, 0, input.size(), input.data());

            // 4 yuyv bytes = 2 rgb pixels => iteration count = w * h / 2
            queue_.enqueueNDRangeKernel(convert_, cl::NullRange,
                                        cl::NDRange(info_.width * info_.height >> 1),
                                        cl::NDRange(info_.width >> 1));


            // TODO: make a proper sync instead of readBuffer
            queue_.enqueueReadBuffer(output_, CL_TRUE, 0, outputSize_,
                                     outputBuffer_.data());
        } else {
            queue_.enqueueWriteBuffer(output_, CL_TRUE, 0, input.size(), input.data());
        }

        // make median filtration to reduce noise
        //#define NOMEDIAN
#ifndef NOMEDIAN
        median_.setArg(0, output_);
        median_.setArg(1, input_);
        median_.setArg(2, info_.width * channels_);
        const int windowSize = 5;
        const int ofs = windowSize / 2;
        queue_.enqueueNDRangeKernel(median_, cl::NullRange,
                                    cl::NDRange(info_.width,
                                                info_.height),
                                    cl::NDRange(1, 1));
        queue_.enqueueReadBuffer(input_, CL_TRUE, 0, outputSize_, outputBuffer_.data());
#endif
        queue_.finish();
        if (counter_.end()) {
            std::cout << "compute:" << counter_.average() << std::endl;
        }
        return outputBuffer_;
    }
};


Computer::Computer(int device, const FrameInfo &info) :
        pImpl_(std::make_unique<Computer::Impl>(device, info)) {
}

Computer::~Computer() {
}

Frame Computer::compute(const Frame &input) {
    return pImpl_->compute(input);
}
