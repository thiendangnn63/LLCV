#include "llcv/ops.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace llcv {
    // ---------------------------------------------------------------------------
    // Color conversion
    // ---------------------------------------------------------------------------

    Image to_grayscale(const Image& src) {
        if (src.channels == 1) return src;
        if (src.channels != 3) throw std::invalid_argument("to_grayscale: expected 1 or 3 channels");

        Image dst(src.width, src.height, 1);
        const uint8_t* in = src.data.data();
        uint8_t* out = dst.data.data();
        const size_t n = static_cast<size_t>(src.width) * src.height;

        for (size_t i = 0; i < n; ++i, in += 3) {
            out[i] = static_cast<uint8_t>((77 * in[0] + 150 * in[1] + 29 * in[2] + 128) >> 8);
        }
        return dst;
    }

    // ---------------------------------------------------------------------------
    // Convolution
    // ---------------------------------------------------------------------------

    floatImage convolve_1D(const Image& src, const std::vector<float>& horizontal, const std::vector<float>& vertical) {
        int hSize = horizontal.size();
        int vSize = vertical.size();
        std::vector<float> imgFloat (src.height * src.width);
        floatImage dst;
        dst.height = src.height;
        dst.width = src.width;
        dst.floatMap.resize(src.height * src.width);

        const int hr = hSize / 2;
        const int vr = vSize / 2;

        for (int y = 0; y < src.height; ++y) {
            for (int x = 0; x < src.width; ++x) {
                float convV = 0.0f;

                for (int i = 0; i < hSize; i++) {
                    int nx = x + i - hr;

                    if (nx < 0) nx = 0;
                    if (nx >= src.width) nx = src.width - 1;

                    convV += src.at(nx, y) * horizontal[i];
                }

                imgFloat[y * src.width + x] = convV;
            }
        }

        for (int y = 0; y < src.height; ++y) {
            for (int x = 0; x < src.width; ++x) {
                float convV = 0.0f;

                for (int i = 0; i < vSize; i++) {
                    int ny = y + i - vr;

                    if (ny < 0) ny = 0;
                    if (ny >= src.height) ny = src.height - 1;

                    convV += imgFloat[ny * src.width + x] * vertical[i];
                }
                dst.floatMap[y * src.width + x] = convV;
            }
        }

        return dst;
    }

    // ---------------------------------------------------------------------------
    // Float -> 8-bit conversion
    // ---------------------------------------------------------------------------

    Image clampBlur(const floatImage& blurMap) {
        Image dst (blurMap.width, blurMap.height, 1);
        for (int i = 0; i < blurMap.height; i++) {
            for (int j = 0; j < blurMap.width; j++) {
                float val = std::lround(blurMap.floatMap[i * blurMap.width + j]);
                if (val < 0) val = 0;
                if (val > 255) val = 255;
                dst.at(j, i) = val;
            }
        }

        return dst;
    }

    // ---------------------------------------------------------------------------
    // Gaussian blur
    // ---------------------------------------------------------------------------

    std::vector<float> gaussian_kernel_1D(const float sigma) {
        int radius = std::ceil(sigma * 3);
        int kernelSize = radius * 2 + 1;

        std::vector<float> kernel (kernelSize);

        float weightTotal = 0.0f;
        for (int i = 0; i < kernelSize; i++) {
            int x2 = (i - radius) * (i - radius);
            float weight = std::exp(-1 * x2 / (2 * sigma * sigma));
            weightTotal += weight;
            kernel[i] = weight;
        }
        for (float& w : kernel) {
            w /= weightTotal;
        }

        return kernel;
    }

    Image gaussian_blur(const Image& src, const float sigma) {
        std::vector<float> horizontal = gaussian_kernel_1D(sigma);
        floatImage floatMap = convolve_1D(src, horizontal, horizontal);

        return clampBlur(floatMap);
    }

    // ---------------------------------------------------------------------------
    // Sobel
    // ---------------------------------------------------------------------------

    std::vector<float> sobel_kernel_smoothing(const int radius) {
        int kernelSize = radius * 2 + 1;
        std::vector<float> kernel_smooth (kernelSize, 1);

        for (int i = 1; i < kernelSize; i++) {
            kernel_smooth[i] = kernel_smooth[i-1] * (kernelSize - i) / i;
        }

        return kernel_smooth;
    }

    std::vector<float> sobel_kernel_differentiation(const int radius) {
        int kernelSize = radius * 2 + 1;
        std::vector<int> pascal (kernelSize - 1, 1);
        std::vector<float> kernel_differentiation (kernelSize);

        for (int i = 1; i < kernelSize - 2; i++) {
            pascal[i] = pascal[i - 1] * (kernelSize - i - 1) / i;
        }

        kernel_differentiation[0] = pascal[0] * -1;
        for (int i = 1; i < kernelSize - 1; i++) {
            kernel_differentiation[i] = pascal[i-1] - pascal[i];
        }
        kernel_differentiation[kernelSize - 1] = pascal[kernelSize - 2];

        return kernel_differentiation;
    }

    floatImage sobel_horizontal(const Image& src, const int radius) {
        if (radius < 1) throw std::invalid_argument("sobel_horizontal: sobel requires radius >= 1");

        std::vector<float> smooth = sobel_kernel_smoothing(radius);
        std::vector<float> diff = sobel_kernel_differentiation(radius);

        return convolve_1D(src, diff, smooth);
    }

    floatImage sobel_vertical(const Image& src, const int radius) {
        if (radius < 1) throw std::invalid_argument("sobel_vertical: sobel requires radius >= 1");

        std::vector<float> smooth = sobel_kernel_smoothing(radius);
        std::vector<float> diff = sobel_kernel_differentiation(radius);

        return convolve_1D(src, smooth, diff);
    }

    std::pair<floatImage, float> sobel_image(const Image& src, const int radius) {
        floatImage horizontal = sobel_horizontal(src, radius);
        floatImage vertical = sobel_vertical(src, radius);

        floatImage dst;
        dst.height = horizontal.height;
        dst.width = horizontal.width;
        dst.floatMap.resize(dst.height * dst.width);

        float runningMax = 0.0f;
        for (int i = 0; i < dst.height; i++) {
            for (int j = 0; j < dst.width; j++) {
                float gx = horizontal.floatMap[i * dst.width + j];
                float gy = vertical.floatMap[i * dst.width + j];
                float magnitude = std::hypot(gx, gy);

                dst.floatMap[i * dst.width + j] = magnitude;
                runningMax = std::max(runningMax, magnitude);
            }
        }

        return {dst, runningMax};
    }

    Image sobel(const Image& src, const int radius) {
        auto [dst, maxValue] = sobel_image(src, radius);
        if (maxValue == 0) return Image (dst.width, dst.height, 1);

        Image sobelImage (dst.width, dst.height, 1);
        for (int i = 0; i < dst.height * dst.width; i++) {
            float intermediate = dst.floatMap[i] / maxValue * 255;
            sobelImage.data[i] = std::lround(intermediate);
        }
        return sobelImage;
    }

    // ---------------------------------------------------------------------------
    // Commented out / unused
    // ---------------------------------------------------------------------------

    // UNUSED: general 2D convolution. gaussian_blur now uses the faster separable convolve_1D
    // instead, so nothing calls this anymore. Restore it for kernels that can't be split into
    // two 1D passes (e.g. the sharpen kernel), or as a reference to check convolve_1D against.
    /*
    // Convolve a 1-channel image with a k×k kernel (k odd). Clamp borders.
    Image convolve(const Image& src, const std::vector<float>& kernel, int k) {
        if (src.channels != 1) return src;

        Image dst(src.width, src.height, 1);
        const int r = k / 2;

        for (int y = 0; y < src.height; ++y) {
            for (int x = 0; x < src.width; ++x) {
                float convV = 0.0f;

                for (int i = 0; i < k; i++) {
                    for (int j = 0; j < k; j++) {
                        int nx = x + j - r;
                        int ny = y + i - r;

                        if (nx < 0) nx = 0;
                        if (ny < 0) ny = 0;
                        if (nx >= src.width) nx = src.width - 1;
                        if (ny >= src.height) ny = src.height - 1;

                        convV += src.at(nx, ny) * kernel[i * k + j];
                    }
                }
                int rounded = std::lround(convV);
                rounded = rounded < 0 ? 0 : rounded;
                rounded = rounded > 255 ? 255 : rounded;
                dst.at(x, y) = static_cast<uint8_t>(rounded);
            }
        }
        return dst;
    }
    */

    // UNUSED: full k×k 2D Gaussian kernel, only useful together with the 2D convolve above.
    // Replaced by gaussian_kernel_1D, since a Gaussian splits into two identical 1D passes
    // (k² multiplications per pixel down to 2k). Restore it alongside convolve for comparisons.
    /*
    // Gaussian blur
    std::vector<float> gaussian_kernel(const float sigma) {
        int radius = std::ceil(sigma * 3);
        int kernelSize = radius * 2 + 1;
        std::vector<float> kernel (kernelSize * kernelSize);

        float weightTotal = 0.0f;
        for (int i = 0; i < kernelSize; i++) {
            for (int j = 0; j < kernelSize; j++) {
                int x2 = (j - radius) * (j - radius);
                int y2 = (i - radius) * (i - radius);

                float weight = std::exp(-1 * (x2 + y2) / (2 * sigma * sigma));
                weightTotal += weight;
                kernel[i * kernelSize + j] = weight;
            }
        }
        for (float& w : kernel) {
            w /= weightTotal;
        }

        return kernel;
    }
    */
}  // namespace llcv
