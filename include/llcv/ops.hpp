#pragma once

#include <utility>
#include <vector>

#include "llcv/image.hpp"

namespace llcv {

// RGB -> single-channel luma (BT.601 weights). A 1-channel input is returned as a copy.
Image to_grayscale(const Image& src);
std::vector<float> gaussian_kernel_1D(const float sigma);
floatImage convolve_1D(const Image& src, const std::vector<float>& horizontal, const std::vector<float>& vertical);
Image clampBlur(const floatImage& blurMap);
Image clampGradient(const floatImage& gradientMap);
Image gaussian_blur(const Image& src, const float sigma);
floatImage sobel_horizontal(const Image& src, const int radius);
floatImage sobel_vertical(const Image& src, const int radius);
std::pair<floatImage, float> sobel_image(const Image& src, const int radius);
Image sobel(const Image& src, const int radius);
std::vector<float> sobel_kernel_smoothing(const int radius);
std::vector<float> sobel_kernel_differentiation(const int radius);

// ---------------------------------------------------------------------------
// Commented out / unused (superseded by the separable versions above)
// ---------------------------------------------------------------------------
// Image convolve(const Image& src, const std::vector<float>& kernel, int k);
// std::vector<float> gaussian_kernel(const float sigma);

}  // namespace llcv
