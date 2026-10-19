#include <cstdio>
#include <exception>

#include "llcv/image.hpp"
#include "llcv/ops.hpp"

using namespace llcv;

int main(int argc, char** argv) {
    try {
        Image input;
        const char* input_path = "data/random_693.ppm";
        const char* out_path = "out.pgm";

        if (argc >= 2) {
            input = load_pnm(argv[1]);
            if (argc >= 3) out_path = argv[2];
        } else {
            input = load_pnm(input_path);
        }

        std::printf("Loaded %dx%d, %d channel(s)\n", input.width, input.height, input.channels);

        Image output = gaussian_blur(to_grayscale(input), 1);
        Image canny_op = canny(output, 1);
        save_pnm(out_path, canny_op);
        std::printf("Wrote %s\n", out_path);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
