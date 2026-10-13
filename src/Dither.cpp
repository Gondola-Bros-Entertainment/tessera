// tessera - Dither implementation.
// Each output pixel depends only on the matching input pixel and its position,
// so the result is built fresh with generate().
#include "tessera/Dither.hpp"

#include "tessera/Color.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace tessera {
namespace {

constexpr int channelMax = 255;
constexpr double ditherStrength = 64.0; // how much the threshold shifts a channel
constexpr double cellCenter = 0.5;      // sample the center of each matrix cell
constexpr double thresholdBias = 0.5;   // recenter normalized value to [-0.5, 0.5]

constexpr int bayer2Size = 2;
constexpr int bayer4Size = 4;
constexpr int bayer8Size = 8;

constexpr std::array<std::uint8_t, bayer2Size * bayer2Size> bayer2 = {0, 2, 3, 1};

constexpr std::array<std::uint8_t, bayer4Size * bayer4Size> bayer4 = {0, 8,  2, 10, 12, 4, 14, 6,
                                                                      3, 11, 1, 9,  15, 7, 13, 5};

constexpr std::array<std::uint8_t, bayer8Size * bayer8Size> bayer8 = {
    0,  32, 8,  40, 2,  34, 10, 42, 48, 16, 56, 24, 50, 18, 58, 26, 12, 44, 4,  36, 14, 46,
    6,  38, 60, 28, 52, 20, 62, 30, 54, 22, 3,  35, 11, 43, 1,  33, 9,  41, 51, 19, 59, 27,
    49, 17, 57, 25, 15, 47, 7,  39, 13, 45, 5,  37, 63, 31, 55, 23, 61, 29, 53, 21};

std::uint8_t clampByte(int n) { return static_cast<std::uint8_t>(std::max(0, std::min(channelMax, n))); }

// Bayer threshold at (x, y), normalized to [-0.5, 0.5].
double bayerThreshold(DitherMatrix matrix, int x, int y) {
    switch (matrix) {
    case DitherMatrix::Bayer2: {
        const int idx = (y % bayer2Size) * bayer2Size + (x % bayer2Size);
        const double val = bayer2[static_cast<std::size_t>(idx)];
        return (val + cellCenter) / (bayer2Size * bayer2Size) - thresholdBias;
    }
    case DitherMatrix::Bayer4: {
        const int idx = (y % bayer4Size) * bayer4Size + (x % bayer4Size);
        const double val = bayer4[static_cast<std::size_t>(idx)];
        return (val + cellCenter) / (bayer4Size * bayer4Size) - thresholdBias;
    }
    case DitherMatrix::Bayer8: {
        const int idx = (y % bayer8Size) * bayer8Size + (x % bayer8Size);
        const double val = bayer8[static_cast<std::size_t>(idx)];
        return (val + cellCenter) / (bayer8Size * bayer8Size) - thresholdBias;
    }
    }
    return 0.0; // unreachable; satisfies -Wreturn-type
}

// Shift a single channel by the dither threshold, then clamp to byte range.
std::uint8_t adjustChannel(double threshold, std::uint8_t ch) {
    const double val = static_cast<double>(ch) + threshold * ditherStrength;
    return clampByte(static_cast<int>(std::lround(val)));
}

// Shift a color's RGB by the threshold; alpha is preserved.
Color adjustColor(double threshold, Color c) {
    return {adjustChannel(threshold, c.r), adjustChannel(threshold, c.g), adjustChannel(threshold, c.b), c.a};
}

// Squared Euclidean distance between two colors in RGB space (alpha ignored).
int colorDistance(Color a, Color b) {
    const int dr = static_cast<int>(a.r) - static_cast<int>(b.r);
    const int dg = static_cast<int>(a.g) - static_cast<int>(b.g);
    const int db = static_cast<int>(a.b) - static_cast<int>(b.b);
    return dr * dr + dg * dg + db * db;
}

// Nearest palette color by squared RGB distance; ties keep the earlier color.
Color findClosest(const Palette &palette, Color target) {
    Color best = palette.front();
    int bestDist = colorDistance(target, best);
    for (std::size_t i = 1; i < palette.size(); ++i) {
        const int dist = colorDistance(target, palette[i]);
        if (dist < bestDist) {
            bestDist = dist;
            best = palette[i];
        }
    }
    return best;
}

} // namespace

Canvas orderedDither(DitherMatrix matrix, const Palette &palette, const Canvas &canvas) {
    if (palette.empty())
        return canvas;
    return generate(canvas.w, canvas.h, [&](int x, int y) {
        const Color pixel = getPixel(canvas, x, y);
        const double threshold = bayerThreshold(matrix, x, y);
        return findClosest(palette, adjustColor(threshold, pixel));
    });
}

} // namespace tessera
