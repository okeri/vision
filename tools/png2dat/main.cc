#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <cstring>
#include <png.h>
#include <Frame.hh>

typedef struct PngBuf {
    const png_byte* data;
    size_t offset;
    size_t size;
}* PngBufPtr;

void cbread(void* data, uint8_t* dst, size_t size) {
    PngBufPtr buffer = static_cast<PngBufPtr>(png_get_io_ptr(
        static_cast<png_structp>(data)));
    memcpy(dst, buffer->data + buffer->offset, size);
    buffer->offset += size;
}

bool readFile(const std::string &filename, std::vector<uint8_t> &data) {
    std::ifstream file(filename);
    if (file) {
        size_t size;
        file.seekg(0, std::ios_base::end);
        size = file.tellg();
        file.seekg(0, std::ios_base::beg);
        data.resize(size);
        return file.read(reinterpret_cast<char *>(data.data()), size).good();
    }
    return false;
}

uint8_t * load(const void *data, size_t size, FrameInfo &info) noexcept {
    if (size == 0) {
        return nullptr;
    }
    png_structp pngPtr = png_create_read_struct(
        PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop infoPtr = png_create_info_struct(pngPtr);

    PngBuf buffer{static_cast<const uint8_t*>(data), 0, size};
    png_set_read_fn(pngPtr, &buffer, reinterpret_cast<png_rw_ptr>(cbread));

    if (setjmp(png_jmpbuf(pngPtr))) {
        png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
        return nullptr;
    }

    // read image header
    int bit_depth, fmt;
    png_read_info(pngPtr, infoPtr);
    png_get_IHDR(
        pngPtr, infoPtr, &info.width, &info.height, &bit_depth, &fmt, NULL, NULL, NULL);

    if (fmt == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(pngPtr);
    }

    if (fmt == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(pngPtr);
    }

    if (fmt == PNG_COLOR_TYPE_RGBA) {
        png_set_strip_alpha(pngPtr);
    }

    // Ensure 8-bit packing
    if (bit_depth < 8)
        png_set_packing(pngPtr);
    else if (bit_depth == 16)
        png_set_scale_16(pngPtr);

    png_read_update_info(pngPtr, infoPtr);

    // read data
    uint8_t *img = nullptr;
    const png_size_t stride = png_get_rowbytes(pngPtr, infoPtr);
    if (stride) {
        size_t imgSize = stride * info.height;
        img = new uint8_t[imgSize];

        png_byte* row_ptrs[info.height];

        png_uint_32 i;
        for (i = 0; i < info.height; i++) {
            row_ptrs[i] = img + i * stride;
        }
        png_read_image(pngPtr, &row_ptrs[0]);
    }

    // free
    png_read_end(pngPtr, infoPtr);
    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
    return img;
}



int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "usage" << argv[0] << " pngfile outfile" << std::endl;
        return -1;
    }
    std::vector<uint8_t> pngData;
    if (readFile(argv[1], pngData)) {
        FrameInfo info;
        info.format = FrameFormat::BGR;
        uint8_t * rgb = load(pngData.data(), pngData.size(), info);
        if (rgb != nullptr) {
            std::ofstream file(argv[2], std::ofstream::app);
            if (file.tellp() == 0) {
                file.write(reinterpret_cast<char *>(&info), sizeof(info));
                std::cout << "wrote header [" << info.width
                          << " x "<< info.height << "]"
                          << std::endl;
            }
            file.write(reinterpret_cast<char *>(rgb), info.size());
            std::cout << "wrote " << info.size() << " bytes" << std::endl;
            delete [] rgb;
        } else {
            std::cout << "Cannot convert png data" << std::endl;
        }
    } else {
        std::cout << "Cannot read input file" << std::endl;
    }
    return 0;
}
