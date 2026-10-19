#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace llcv {

// Image with element type T, interleaved channels, row-major.
// Pixel (x, y), channel c lives at data[(y * width + x) * channels + c].
// The constructor is instantiated in image.cpp for uint8_t, float and int16_t only.
template <typename T>
struct ImageT {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<T> data;

    ImageT() = default;
    ImageT(int w, int h, int c);

    bool empty() const { return data.empty(); }
    size_t size() const { return data.size(); }

    T& at(int x, int y, int c = 0) {
        return data[(static_cast<size_t>(y) * width + x) * channels + c];
    }
    T at(int x, int y, int c = 0) const {
        return data[(static_cast<size_t>(y) * width + x) * channels + c];
    }
};

// 8-bit image: what load_pnm / save_pnm read and write.
using Image = ImageT<uint8_t>;
using floatImage = ImageT<float>;
using int16Image = ImageT<int16_t>;

// Load a binary PGM (P5, 1 channel) or PPM (P6, 3 channels). Throws std::runtime_error.
Image load_pnm(const std::string& path);

// Save as PGM if the image has 1 channel, PPM if it has 3. Throws std::runtime_error.
void save_pnm(const std::string& path, const Image& img);

}  // namespace llcv