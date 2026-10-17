#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace llcv {

// 8-bit image with interleaved channels, row-major.
// Pixel (x, y), channel c lives at data[(y * width + x) * channels + c].
struct Image {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<uint8_t> data;

    Image() = default;
    Image(int w, int h, int c);

    bool empty() const { return data.empty(); }
    size_t size() const { return data.size(); }

    uint8_t& at(int x, int y, int c = 0) {
        return data[(static_cast<size_t>(y) * width + x) * channels + c];
    }
    uint8_t at(int x, int y, int c = 0) const {
        return data[(static_cast<size_t>(y) * width + x) * channels + c];
    }
};

struct floatImage {
    int height = 0;
    int width = 0;
    std::vector<float> floatMap;
};

// Load a binary PGM (P5, 1 channel) or PPM (P6, 3 channels). Throws std::runtime_error.
Image load_pnm(const std::string& path);

// Save as PGM if the image has 1 channel, PPM if it has 3. Throws std::runtime_error.
void save_pnm(const std::string& path, const Image& img);

}  // namespace llcv