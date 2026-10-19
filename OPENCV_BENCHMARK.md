# LLCV vs OpenCV benchmark

The current versions of the functions in `src/ops.cpp`, compared with their OpenCV equivalents on speed and output.

Measured on 2026-10-18.

---

## Setup

- **LLCV:** built with the project's normal flags (`-std=c++17 -O2`, static, no `-march`). Single-threaded.
- **OpenCV:** 5.0.0 (`opencv-python-headless`, numpy 2.5.4), called from Python. At these image sizes the Python call overhead is negligible. Timed twice: with 1 thread (`cv2.setNumThreads(1)`) and with 16 threads (the default on this machine).
- **Timing:** best of 5 runs for every function, so cold caches and page faults are excluded.
- **Images:**
  - `data/random_693.ppm`, a 640×480 RGB photo.
  - A 3000×2000 RGB synthetic image: sine pattern, 150 px checkerboard and a large disc.
- **Same input for both:** Sobel and Canny run on the image after `to_grayscale` plus `gaussian_blur` with sigma 1. Both libraries get exactly the same blurred bytes, so differences come only from the function being tested.
- **Borders:** OpenCV is set to `BORDER_REPLICATE` everywhere, which matches LLCV's edge clamping.

| LLCV | OpenCV equivalent |
|---|---|
| `to_grayscale` | `cvtColor` with `COLOR_RGB2GRAY` |
| `gaussian_blur(img, 2)` on RGB | `GaussianBlur`, 13×13 kernel, sigma 2 |
| `sobel_image(img, 1)` | `Sobel` dx and dy (float32, ksize 3), then `magnitude` |
| `sobel(img, 1)` | the above, then `convertScaleAbs` with alpha = 255 / max |
| `canny(img, 1)`, thresholds 100 / 40 | `Canny` with thresholds 40 / 100, ksize 3, `L2gradient` on |

---

## Results

Times in ms. "LLCV slower by" compares against single-threaded OpenCV, since LLCV is single-threaded too.

> **Stale:** the `canny` rows (timings and accuracy) were measured before the rewrite that merged non-maximum suppression and thresholding into one pass and dropped `thinMap`. Re-run the benchmark to update them.

### 640×480 photo

| Function | LLCV | OpenCV, 1 thread | OpenCV, 16 threads | LLCV slower by |
|---|---|---|---|---|
| `to_grayscale` | 0.48 | 0.11 | 0.11 | 4.2× |
| `gaussian_blur` (sigma 2) | 11.76 | 1.85 | 0.91 | 6.3× |
| `sobel_image` | 3.50 | 1.10 | 1.05 | 3.2× |
| `sobel` | 3.55 | 1.14 | 1.07 | 3.1× |
| `canny` | 9.80 | 1.78 | 1.10 | 5.5× |

### 3000×2000 synthetic

| Function | LLCV | OpenCV, 1 thread | OpenCV, 16 threads | LLCV slower by |
|---|---|---|---|---|
| `to_grayscale` | 8.87 | 3.71 | 1.45 | 2.4× |
| `gaussian_blur` (sigma 2) | 217.01 | 31.42 | 10.72 | 6.9× |
| `sobel_image` | 65.99 | 21.38 | 19.76 | 3.1× |
| `sobel` | 66.49 | 27.31 | 24.37 | 2.4× |
| `canny` | 138.26 | 16.76 | 12.20 | 8.2× |

---

## Output accuracy

"Differ" is the share of output values that don't match OpenCV exactly.

| Function | Photo | Synthetic |
|---|---|---|
| `to_grayscale` | max diff 1, 0.03% differ | max diff 1, 5.15% differ |
| `gaussian_blur` | max diff 1, 3.22% differ | max diff 1, 0.97% differ |
| `sobel_image` | max diff 3e-5 (float rounding) | max diff 3e-5 (float rounding) |
| `sobel` | identical | identical |
| `canny` | edge pixels: 20716 LLCV vs 20728 OpenCV, 0.05% of pixels differ | edge pixels: 108664 LLCV vs 80845 OpenCV, 0.47% of pixels differ |

### Why the outputs differ

- **`to_grayscale`:** OpenCV uses 14-bit fixed-point BT.601 weights, LLCV uses 8-bit ones (77 / 150 / 29). Some pixels round differently by 1.
- **`gaussian_blur`:** OpenCV blurs 8-bit images with integer fixed-point arithmetic, LLCV uses float. Some pixels round differently by 1.
- **`canny`:** most likely the tie rule in non-maximum suppression (not confirmed). LLCV keeps a pixel when its magnitude is at least as large as both neighbours, so on a two-pixel-wide ridge where both pixels have equal magnitude it keeps both. OpenCV uses "greater than" on one side and "at least" on the other, so it keeps one. The synthetic image's hard checkerboard edges produce many such ties, which would explain its larger edge count. On the photo the two are almost identical.

---

## Why OpenCV is faster

Based on how OpenCV is implemented. None of these were profiled here.

- **Hand-written SIMD.** OpenCV picks AVX2 code paths at runtime. LLCV relies on auto-vectorization at `-O2` with no `-march`. The interleaved RGB loop in `to_grayscale` in particular doesn't auto-vectorize.
- **Integer arithmetic on 8-bit images.** OpenCV's 8-bit blur works in 16-bit fixed point, so twice as many values fit in each SIMD register as with float, and no full-size float temporary image is needed.
- **Fewer full-size temporaries.** LLCV's Sobel runs two full `convolve_1D` passes, each with its own float temporary image. Canny additionally builds the full-size squared-magnitude map `mMap`. OpenCV's Canny works on 16-bit integer gradients and processes the image in row buffers.
- **Threading.** Blur and Canny gain another 1.4-2.9× from 16 threads. Sobel barely changes, which suggests it is limited by memory bandwidth rather than computation.

---

## Reproducing

The benchmark programs live outside the project, so LLCV itself has no OpenCV dependency:

- `C:\Users\PC\AppData\Local\Temp\claude\C--Users-PC-Documents-Code-LLCV\57cd87d7-45ee-4313-a8a4-fb88137d4d25\scratchpad\cv\llcv_run.cpp`: runs and times the LLCV functions and saves their outputs.
- `C:\Users\PC\AppData\Local\Temp\claude\C--Users-PC-Documents-Code-LLCV\57cd87d7-45ee-4313-a8a4-fb88137d4d25\scratchpad\cv\cv_compare.py`: runs and times OpenCV on the same inputs and compares the outputs.
- The OpenCV venv is at `C:\Users\PC\AppData\Local\Temp\claude\C--Users-PC-Documents-Code-LLCV\57cd87d7-45ee-4313-a8a4-fb88137d4d25\scratchpad\cvenv`.

These are in a temporary folder and may be deleted later. Copy them somewhere permanent if you want to rerun the comparison after future changes.
