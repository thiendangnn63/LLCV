#include <cstdio>
#include <exception>

#include "llcv/image.hpp"
#include "llcv/ops.hpp"

using namespace llcv;

// Synthetic RGB test pattern so the pipeline can run without an input file.
static Image make_test_pattern(int w, int h) {
    Image img(w, h, 3);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            img.at(x, y, 0) = static_cast<uint8_t>(255 * x / (w - 1));
            img.at(x, y, 1) = static_cast<uint8_t>(255 * y / (h - 1));
            img.at(x, y, 2) = ((x / 32 + y / 32) % 2) ? 255 : 0;
        }
    }
    return img;
}

int main(int argc, char** argv) {
    try {
        Image input;
        const char* out_path = "out.pgm";

        if (argc >= 2) {
            input = load_pnm(argv[1]);
            if (argc >= 3) out_path = argv[2];
        } else {
            input = make_test_pattern(256, 256);
            save_pnm("test_pattern.ppm", input);
            std::printf("No input given; wrote test_pattern.ppm\n");
        }

        std::printf("Loaded %dx%d, %d channel(s)\n", input.width, input.height, input.channels);

        Image output = sobel(input, 3);
        save_pnm(out_path, output);
        std::printf("Wrote %s\n", out_path);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
