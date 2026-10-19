#include "llcv/image.hpp"

#include <cctype>
#include <fstream>
#include <stdexcept>

namespace llcv {

Image::Image(int w, int h, int c) : width(w), height(h), channels(c) {
    if (w <= 0 || h <= 0 || c <= 0) throw std::invalid_argument("Image: dimensions must be positive");
    data.assign(static_cast<size_t>(w) * h * c, 0);
}

floatImage::floatImage(int w, int h, int c) : width(w), height(h), channels(c) {
    if (w <= 0 || h <= 0 || c <= 0) throw std::invalid_argument("floatImage: dimensions must be positive");
    floatMap.assign(static_cast<size_t>(w) * h * c, 0);
}

namespace {

// Reads the next whitespace-separated integer from a PNM header, skipping '#' comments.
int read_header_int(std::istream& in) {
    int ch;
    while ((ch = in.peek()) != EOF) {
        if (std::isspace(ch)) {
            in.get();
        } else if (ch == '#') {
            while ((ch = in.get()) != EOF && ch != '\n') {}
        } else {
            break;
        }
    }
    int value;
    if (!(in >> value)) throw std::runtime_error("PNM: malformed header");
    return value;
}

}  // namespace

Image load_pnm(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("PNM: cannot open " + path);

    char magic[2];
    if (!in.read(magic, 2) || magic[0] != 'P' || (magic[1] != '5' && magic[1] != '6'))
        throw std::runtime_error("PNM: " + path + " is not a binary PGM (P5) or PPM (P6)");
    const int channels = magic[1] == '5' ? 1 : 3;

    const int width = read_header_int(in);
    const int height = read_header_int(in);
    const int maxval = read_header_int(in);
    if (width <= 0 || height <= 0) throw std::runtime_error("PNM: invalid dimensions");
    if (maxval <= 0 || maxval > 255) throw std::runtime_error("PNM: only 8-bit images are supported");

    // Exactly one whitespace byte separates the header from the pixel data.
    in.get();

    Image img(width, height, channels);
    if (!in.read(reinterpret_cast<char*>(img.data.data()), static_cast<std::streamsize>(img.size())))
        throw std::runtime_error("PNM: unexpected end of pixel data in " + path);
    return img;
}

void save_pnm(const std::string& path, const Image& img) {
    if (img.channels != 1 && img.channels != 3)
        throw std::runtime_error("PNM: can only save 1- or 3-channel images");

    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("PNM: cannot write " + path);

    out << (img.channels == 1 ? "P5" : "P6") << '\n'
        << img.width << ' ' << img.height << '\n'
        << 255 << '\n';
    out.write(reinterpret_cast<const char*>(img.data.data()), static_cast<std::streamsize>(img.size()));
    if (!out) throw std::runtime_error("PNM: write failed for " + path);
}

}  // namespace llcv
