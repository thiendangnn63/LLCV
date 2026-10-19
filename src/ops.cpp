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

    // change image to grayscale
    Image to_grayscale(const Image& src) {
        if (src.channels == 1) return src;
        if (src.channels != 3) throw std::invalid_argument("to_grayscale: expected 1 or 3 channels");
        const int W = src.width;
        const int H = src.height;

        Image dst(W, H, 1);
        const uint8_t* in = src.data.data();
        uint8_t* out = dst.data.data();
        const size_t n = static_cast<size_t>(W) * H;

        for (size_t i = 0; i < n; ++i, in += 3) {
            out[i] = static_cast<uint8_t>((77 * in[0] + 150 * in[1] + 29 * in[2] + 128) >> 8);
        }
        return dst;
    }

    // ---------------------------------------------------------------------------
    // Convolution
    // ---------------------------------------------------------------------------

    // return enum to indicate kernel type
    template <typename T>
    KernelType kernel_type(const std::vector<T>& kernel) {
        const int ks = kernel.size();
        if (ks == 0 || ks % 2 == 0) return KernelType::General;

        const int kr = ks / 2;
        bool sym = true;
        bool anti = true;

        for (int i = 0; i < kr; i++) {
            T p1 = kernel[i];
            T p2 = kernel[ks - i - 1];
            if (p1 != p2) sym = false;
            if (p1 != -p2) anti = false;
        }
        if (kernel[kr] != T(0)) anti = false;

        if (sym) return KernelType::Symmetric;
        if (anti) return KernelType::Antisymmetric;
        return KernelType::General;
    }
    
    // these helper functions apply things based on kernel type
    // idk what da fuck im doing gang
    template <typename T>
    static void symmetric_helper(T* dst, const T* A, const T* B, T weight, size_t length) {
        for (size_t x = 0; x < length; ++x) {
            dst[x] += (A[x] + B[x]) * weight;
        }
    }

    template <typename T>
    static void antisymmetric_helper(T* dst, const T* A, const T* B, T weight, size_t length) {
        for (size_t x = 0; x < length; ++x) {
            dst[x] += (A[x] - B[x]) * weight;
        }
    }

    template <typename T>
    static void general_helper(T* dst, const T* A, T weight, size_t length) {
        for (size_t x = 0; x < length; ++x) {
            dst[x] += A[x] * weight;
        }
    }

    // apply convolution to image in two stages
    // time complexity: width * height * channels * 2k
    // only valid for separable kernels, i.e. 2D kernels that are the outer product of two 1D kernels
    template <typename T>
    ImageT<T> convolve_1D(const Image& src, const std::vector<T>& horizontal, const std::vector<T>& vertical) {
        int hSize = horizontal.size();
        int vSize = vertical.size();

        KernelType hType = kernel_type(horizontal);
        KernelType vType = kernel_type(vertical);

        const int W = src.width;
        const int H = src.height;
        const int C = src.channels;
        const size_t ROW_LEN = static_cast<size_t>(W) * C;
        
        ImageT<T> rowPass (W, H, C);
        ImageT<T> dst (W, H, C);

        const int hr = hSize / 2;
        const int vr = vSize / 2;

        T* base = rowPass.data.data();
        std::vector<T> scratch ((W + 2 * hr) * C);
        for (int y = 0; y < H; ++y) {
            const uint8_t* sourceRow = src.data.data() + y * ROW_LEN;
            T* destRow = base + y * ROW_LEN;

            for (int i = 0; i < hr; i++) {
                for (int j = 0; j < C; j++) {
                    scratch[i * C + j] = sourceRow[j];
                    scratch[(hr + W) * C + i * C + j] = sourceRow[(W - 1) * C + j];
                }
            }

            for (size_t i = 0; i < ROW_LEN; i++) {
                scratch[hr * C + i] = sourceRow[i];
            }

            // ugly ass code bruh
            // im not getting that internship
            if (hType == KernelType::General) {
                for (int i = 0; i < hSize; i++) {
                    T multiplier = horizontal[i];
                    general_helper(destRow, scratch.data() + i * C, multiplier, ROW_LEN);
                }
            }
            else {
                for (int i = 0; i < hr; i++) {
                    T multiplier = horizontal[i];
                    if (hType == KernelType::Symmetric) symmetric_helper(destRow, scratch.data() + i * C, scratch.data() + (hSize - 1 - i) * C, multiplier, ROW_LEN);
                    else if (hType == KernelType::Antisymmetric) antisymmetric_helper(destRow, scratch.data() + i * C, scratch.data() + (hSize - 1 - i) * C, multiplier, ROW_LEN);                    
                }
                if (hType == KernelType::Symmetric) {
                    T midmul = horizontal[hr];
                    for (size_t x = 0; x < ROW_LEN; ++x) {
                        destRow[x] += scratch[x + hr * C] * midmul;
                    }
                }
            }
        }

        T* dp = dst.data.data();
        for (int y = 0; y < H; ++y) {
            T* destRow = dp + y * ROW_LEN;

            if (vType == KernelType::General) {
                for (int i = 0; i < vSize; i++) {
                    int ny = y + i - vr;
                    ny = ny < 0 ? 0 : ny;
                    ny = ny > H - 1 ? H - 1 : ny;
                    
                    const T* sourceRow = base + ny * ROW_LEN;
                    T multiplier = vertical[i];

                    general_helper(destRow, sourceRow, multiplier, ROW_LEN);
                }
            }
            else {
                for (int i = 0; i < vr; i++) {
                    int ny = y + i - vr;
                    ny = ny < 0 ? 0 : ny;

                    int my = y + vr - i;
                    my = my > H - 1 ? H - 1 : my;

                    const T* sourceRow = base + ny * ROW_LEN;
                    const T* mirrorRow = base + my * ROW_LEN;
                    T multiplier = vertical[i];

                    if (vType == KernelType::Symmetric) {
                        symmetric_helper(destRow, sourceRow, mirrorRow, multiplier, ROW_LEN);
                    }
                    else if (vType == KernelType::Antisymmetric) {
                        antisymmetric_helper(destRow, sourceRow, mirrorRow, multiplier, ROW_LEN);
                    }
                }
                if (vType == KernelType::Symmetric) {
                    T midmul = vertical[vr];
                    general_helper(destRow, base + y * ROW_LEN, midmul, ROW_LEN);
                }
            }
        }
        return dst;
    }

    // templates live in this file, so every type used elsewhere needs an explicit instantiation here
    template KernelType kernel_type<float>(const std::vector<float>& kernel);
    template KernelType kernel_type<int16_t>(const std::vector<int16_t>& kernel);
    template floatImage convolve_1D<float>(const Image& src, const std::vector<float>& horizontal, const std::vector<float>& vertical);
    // int16_t overflows for Sobel radius >= 3: worst case 255 * 64 * 12 = 195840 > 32767
    template int16Image convolve_1D<int16_t>(const Image& src, const std::vector<int16_t>& horizontal, const std::vector<int16_t>& vertical);

    // ---------------------------------------------------------------------------
    // Float -> 8-bit conversion
    // ---------------------------------------------------------------------------

    // bound restrict value of blur images
    // float -> 8-bit: rounds each value to the nearest integer and clamps it to 0-255.
    // nothing blur-specific; works for any float image already on the 0-255 scale.
    Image clampBlur(const floatImage& blurMap) {
        const int W = blurMap.width;
        const int H = blurMap.height;
        const int C = blurMap.channels;
        const float* base = blurMap.data.data();
        const size_t SIZE = static_cast<size_t>(H) * W * C;

        Image dst (W, H, C);
        uint8_t* dstp = dst.data.data();

        for (size_t i = 0; i < SIZE; i++) {
            dstp[i] = static_cast<uint8_t> (std::clamp(base[i], 0.0f, 255.0f) + 0.5f);
        }

        return dst;
    }

    // ---------------------------------------------------------------------------
    // Gaussian blur
    // ---------------------------------------------------------------------------

    // get gaussian blur kernel
    std::vector<float> gaussian_kernel_1D(const float sigma) {
        if (!(sigma > 0)) throw std::invalid_argument("gaussian_kernel_1D: sigma must be > 0");
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

    // main gaussian blur function that gets gaussian kernel 
    // and call convolve_1D to apply it to source image
    Image gaussian_blur(const Image& src, const float sigma) {
        std::vector<float> horizontal = gaussian_kernel_1D(sigma);
        floatImage floatMap = convolve_1D(src, horizontal, horizontal);

        return clampBlur(floatMap);
    }

    // ---------------------------------------------------------------------------
    // Sobel
    // ---------------------------------------------------------------------------

    // smoothing kernel is a binomial row in pascal triangle (idk why)
    std::vector<float> sobel_kernel_smoothing(const int radius) {
        int kernelSize = radius * 2 + 1;
        std::vector<float> kernel_smooth (kernelSize, 1);

        for (int i = 1; i < kernelSize; i++) {
            kernel_smooth[i] = kernel_smooth[i-1] * (kernelSize - i) / i;
        }

        return kernel_smooth;
    }

    // differentiation kernel gives differences of adjacent entries of a binomial row one shorter
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

    // based on the type of edge (horizontal or vertical)
    // the manner in which the kernels are applied will be different
    // apply differential horizontally then smoothing vertically
    // to get horizontal edges: diff * smooth
    floatImage sobel_horizontal(const Image& src, const int radius) {
        if (radius < 1) throw std::invalid_argument("sobel_horizontal: sobel requires radius >= 1");

        std::vector<float> smooth = sobel_kernel_smoothing(radius);
        std::vector<float> diff = sobel_kernel_differentiation(radius);

        return convolve_1D(src, diff, smooth);
    }

    // apply smoothing horizontally then differential vertically
    // to get vertical edges: smooth * diff
    floatImage sobel_vertical(const Image& src, const int radius) {
        if (radius < 1) throw std::invalid_argument("sobel_vertical: sobel requires radius >= 1");

        std::vector<float> smooth = sobel_kernel_smoothing(radius);
        std::vector<float> diff = sobel_kernel_differentiation(radius);

        return convolve_1D(src, smooth, diff);
    }

    // get map of horizontal and vertical edges then mash them together
    // returns magnitude map
    std::pair<floatImage, float> sobel_image(const Image& src, const int radius) {
        floatImage horizontal = sobel_horizontal(src, radius);
        floatImage vertical = sobel_vertical(src, radius);

        const int W = horizontal.width;
        const int H = horizontal.height;
        const int C = horizontal.channels;
        const size_t SIZE = static_cast<size_t>(W) * H * C;
        const float* hp = horizontal.data.data();
        const float* vp = vertical.data.data();

        floatImage dst (W, H, C);
        float* dp = dst.data.data();

        float runningMax = 0.0f;
        for (size_t i = 0; i < SIZE; i++) {
            float gx = hp[i];
            float gy = vp[i];
            float magnitude = std::sqrt(gx * gx + gy * gy);

            dp[i] = magnitude;
            runningMax = std::max(runningMax, magnitude);
        }

        return {std::move(dst), runningMax};
    }

    // gradients can exceed well over 255
    // so divide by maximum value to go down to 0-1
    // and multiply by 255 to get a range from 0-255
    Image sobel(const Image& src, const int radius) {
        auto [dst, maxValue] = sobel_image(src, radius);

        const int W = dst.width;
        const int H = dst.height;
        const int C = dst.channels;
        if (maxValue == 0) return Image (W, H, C);

        const size_t SIZE = static_cast<size_t>(W) * H * C;
        const float multiplier = 255.0f / maxValue;
        const float* base = dst.data.data();

        Image sobelImage (W, H, C);
        uint8_t* sp = sobelImage.data.data();

        for (size_t i = 0; i < SIZE; i++) {
            float intermediate = (base[i] * multiplier) + 0.5f;
            sp[i] = static_cast<uint8_t>(intermediate);
        }

        return sobelImage;
    }

    // ---------------------------------------------------------------------------
    // Canny Edge
    // ---------------------------------------------------------------------------

    // canny edge algorithm
    // less noisy than sobel
    // idk how to explain it better
    Image canny(const Image& src, const int radius, float highThres, float lowThres) {
        if (highThres <= 0 || lowThres <= 0) throw std::invalid_argument("canny: threshold must be > 0");
        if (src.channels != 1) throw std::invalid_argument("canny: canny only accepts 1-channel images");

        const int W = src.width;
        const int H = src.height;
        const int C = 1;
        
        const float tan225 = 0.4142f;
        const float tan675 = 2.4142f;
        const size_t SIZE = static_cast<size_t>(W) * H;

        floatImage dx = sobel_horizontal(src, radius);
        floatImage dy = sobel_vertical(src, radius);
        floatImage mMap (W, H, C);

        const float* px = dx.data.data();
        const float* py = dy.data.data();
        float* pm = mMap.data.data();

        for (size_t i = 0; i < SIZE; i++) {
            pm[i] = px[i] * px[i] + py[i] * py[i];
        }

        Image dst (W, H, C);
        uint8_t* pd = dst.data.data();

        std::vector<int> st;
        st.reserve(SIZE * 0.2);
        const int LOW = 0;
        const int MID = 75;
        const int HIGH = 255;
        // neighbor offsets per direction bin: offy moves along rows (i), offx along columns (j)
        const int offy[] = {0, 1, 1, -1};
        const int offx[] = {1, 1, 0, 1};

        for (int i = 1; i < H - 1; i++) {
            for (int j = 1; j < W - 1; j++) {
                size_t idx = i * W + j;
                float base = pm[idx];
                if (base < lowThres * lowThres) continue;

                float gy = py[idx];
                float gx = px[idx];
                int bin = 3;

                if (std::abs(gy) < tan225 * std::abs(gx)) bin = 0;
                else if (std::abs(gy) > tan675 * std::abs(gx)) bin = 2;
                else if (gx * gy > 0) bin = 1;

                float n1 = pm[(i + offy[bin]) * W + (j + offx[bin])];
                float n2 = pm[(i - offy[bin]) * W + (j - offx[bin])];

                if (base < n1 || base < n2) continue;
                if (base >= highThres * highThres) {
                    pd[idx] = HIGH;
                    st.push_back(idx);
                }
                else {
                    pd[idx] = MID;
                }
            }
        }

        int offset[] = {
            offy[0] * W + offx[0],
            offy[1] * W + offx[1],
            offy[2] * W + offx[2],
            offy[3] * W + offx[3]
        };
        
        int idx;
        while (!st.empty()) {
            idx = st.back();
            st.pop_back();
            
            for (int i = 0; i < 4; i++) {
                int n1 = idx + offset[i];
                int n2 = idx - offset[i];
                if (pd[n1] == MID) {
                    pd[n1] = HIGH;
                    st.push_back(n1);
                }
                if (pd[n2] == MID) {
                    pd[n2] = HIGH;
                    st.push_back(n2);
                }
            }
        }

        for (size_t i = 0; i < SIZE; i++) {
            if (pd[i] == MID) pd[i] = LOW;
        }

        return dst;
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

    // UNUSED: original separable convolution with a clamp and at() on every tap.
    // convolve_1D replaced it (padded scratch row, flat row sweeps): same output, about 2.6-4.3x faster.
    // Restore it as the reference to compare convolve_1D against after changing it.
    /*
    floatImage convolve_1D_unoptimized(const Image& src, const std::vector<float>& horizontal, const std::vector<float>& vertical) {
        int hSize = horizontal.size();
        int vSize = vertical.size();
        const int W = src.width;
        const int H = src.height;
        const int C = src.channels;
        floatImage imgFloat (W, H, C);
        floatImage dst (W, H, C);

        const int hr = hSize / 2;
        const int vr = vSize / 2;

        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                for (int c = 0; c < C; ++c) {
                    float convV = 0.0f;

                    for (int i = 0; i < hSize; i++) {
                        int nx = x + i - hr;

                        if (nx < 0) nx = 0;
                        if (nx >= W) nx = W - 1;

                        convV += src.at(nx, y, c) * horizontal[i];
                    }

                    imgFloat.at(x, y, c) = convV;
                }
            }
        }

        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                for (int c = 0; c < C; ++c) {
                    float convV = 0.0f;

                    for (int i = 0; i < vSize; i++) {
                        int ny = y + i - vr;

                        if (ny < 0) ny = 0;
                        if (ny >= H) ny = H - 1;

                        convV += imgFloat.at(x, ny, c) * vertical[i];
                    }
                    dst.at(x, y, c) = convV;
                }
            }
        }
        return dst;
    }
    */
}  // namespace llcv