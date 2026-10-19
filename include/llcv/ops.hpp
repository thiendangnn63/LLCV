#pragma once

#include <utility>
#include <vector>

#include "llcv/image.hpp"

namespace llcv {

// RGB -> single-channel luma (BT.601 weights). A 1-channel input is returned as a copy.
Image to_grayscale(const Image& src);
// Throws std::invalid_argument unless sigma > 0.
std::vector<float> gaussian_kernel_1D(const float sigma);
enum class KernelType {Symmetric, Antisymmetric, General};
// kernel_type and convolve_1D are defined in ops.cpp and explicitly instantiated there
// for T = float and int16_t only; any other T fails to link.
// int16_t overflows for Sobel radius >= 3 (max |value| ~ 255 * 64 * 12 > 32767).
template <typename T>
KernelType kernel_type(const std::vector<T>& kernel);
template <typename T>
ImageT<T> convolve_1D(const Image& src, const std::vector<T>& horizontal, const std::vector<T>& vertical);
Image clampBlur(const floatImage& blurMap);
Image gaussian_blur(const Image& src, const float sigma);
floatImage sobel_horizontal(const Image& src, const int radius);
floatImage sobel_vertical(const Image& src, const int radius);
std::pair<floatImage, float> sobel_image(const Image& src, const int radius);
Image sobel(const Image& src, const int radius);
std::vector<float> sobel_kernel_smoothing(const int radius);
std::vector<float> sobel_kernel_differentiation(const int radius);
Image canny(const Image& src, const int radius, float highThres = 100.0f, float lowThres = 40.0f);

// ---------------------------------------------------------------------------
// Commented out / unused (superseded by the separable versions above)
// ---------------------------------------------------------------------------
// Image convolve(const Image& src, const std::vector<float>& kernel, int k);
// std::vector<float> gaussian_kernel(const float sigma);
// floatImage convolve_1D_unoptimized(const Image& src, const std::vector<float>& horizontal, const std::vector<float>& vertical);

}  // namespace llcv
