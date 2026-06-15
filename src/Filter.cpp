// tessera - Filter implementation.
// Convolutions build a new Canvas with generate(), sampling the source canvas
// with edge-clamped coordinates so neighbor reads never hit partial output.
#include "tessera/Filter.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace tessera {
namespace {

constexpr int channelMax = 255;

// Box-blur passes used to approximate a Gaussian blur.
constexpr int gaussianPasses = 3;

// Blur radius used by the sharpen unsharp mask.
constexpr int sharpenBlurRadius = 1;

// Sharpening blend factor for the original image.
constexpr int sharpenOriginalWeight = 2;

// Sobel kernel radius (1 for a 3x3 kernel).
constexpr int sobelRadius = 1;

// Sobel X kernel (row-major, 3x3).
constexpr std::array<std::array<int, 3>, 3> sobelX{ {
    { { -1, 0, 1 } },
    { { -2, 0, 2 } },
    { { -1, 0, 1 } },
} };

// Sobel Y kernel (row-major, 3x3).
constexpr std::array<std::array<int, 3>, 3> sobelY{ {
    { { -1, -2, -1 } },
    { { 0, 0, 0 } },
    { { 1, 2, 1 } },
} };

// Number of RGB channels (excluding alpha).
constexpr int rgbChannelCount = 3;

// BT.709 luminance weights.
constexpr double luminanceWeightR = 0.2126;
constexpr double luminanceWeightG = 0.7152;
constexpr double luminanceWeightB = 0.0722;

std::uint8_t clampByte(int n) {
    return static_cast<std::uint8_t>(std::max(0, std::min(channelMax, n)));
}

double clampUnit(double x) { return std::max(0.0, std::min(1.0, x)); }

// Read one pixel with coordinates clamped to the nearest edge (repeat edge).
Color sampleClamped(const Canvas& src, int x, int y) {
    const int cx = std::max(0, std::min(src.w - 1, x));
    const int cy = std::max(0, std::min(src.h - 1, y));
    const std::size_t i = static_cast<std::size_t>(pixelIndex(src.w, cx, cy));
    return { src.rgba[i], src.rgba[i + 1], src.rgba[i + 2], src.rgba[i + 3] };
}

// Sum one channel across a horizontal window centered at (cx, cy).
int sumHorizontal(const Canvas& src, int cx, int cy, int radius, int channel) {
    int acc = 0;
    for (int sx = cx - radius; sx <= cx + radius; ++sx) {
        const Color c = sampleClamped(src, sx, cy);
        const std::array<std::uint8_t, 3> rgb{ c.r, c.g, c.b };
        acc += rgb[static_cast<std::size_t>(channel)];
    }
    return acc;
}

// Sum one channel across a vertical window centered at (cx, cy).
int sumVertical(const Canvas& src, int cx, int cy, int radius, int channel) {
    int acc = 0;
    for (int sy = cy - radius; sy <= cy + radius; ++sy) {
        const Color c = sampleClamped(src, cx, sy);
        const std::array<std::uint8_t, 3> rgb{ c.r, c.g, c.b };
        acc += rgb[static_cast<std::size_t>(channel)];
    }
    return acc;
}

// Horizontal box-blur pass: each RGB channel averages a horizontal window;
// alpha is taken from the source pixel.
Canvas blurHorizontal(int radius, const Canvas& src) {
    const int windowSize = 2 * radius + 1;
    return generate(src.w, src.h, [&](int x, int y) -> Color {
        const Color a = sampleClamped(src, x, y);
        return { static_cast<std::uint8_t>(sumHorizontal(src, x, y, radius, 0) / windowSize),
                 static_cast<std::uint8_t>(sumHorizontal(src, x, y, radius, 1) / windowSize),
                 static_cast<std::uint8_t>(sumHorizontal(src, x, y, radius, 2) / windowSize),
                 a.a };
    });
}

// Vertical box-blur pass: each RGB channel averages a vertical window;
// alpha is taken from the source pixel.
Canvas blurVertical(int radius, const Canvas& src) {
    const int windowSize = 2 * radius + 1;
    return generate(src.w, src.h, [&](int x, int y) -> Color {
        const Color a = sampleClamped(src, x, y);
        return { static_cast<std::uint8_t>(sumVertical(src, x, y, radius, 0) / windowSize),
                 static_cast<std::uint8_t>(sumVertical(src, x, y, radius, 1) / windowSize),
                 static_cast<std::uint8_t>(sumVertical(src, x, y, radius, 2) / windowSize),
                 a.a };
    });
}

// Apply a 3x3 kernel to one channel at (cx, cy).
int applyKernel3x3(const std::array<std::array<int, 3>, 3>& kernel, const Canvas& src,
                   int cx, int cy, int channel) {
    int acc = 0;
    for (int dy = -sobelRadius; dy <= sobelRadius; ++dy)
        for (int dx = -sobelRadius; dx <= sobelRadius; ++dx) {
            const int weight = kernel[static_cast<std::size_t>(dy + sobelRadius)]
                                     [static_cast<std::size_t>(dx + sobelRadius)];
            const Color c = sampleClamped(src, cx + dx, cy + dy);
            const std::array<std::uint8_t, 3> rgb{ c.r, c.g, c.b };
            acc += weight * rgb[static_cast<std::size_t>(channel)];
        }
    return acc;
}

// Extract pixels above a luminance threshold; below threshold becomes black.
// Alpha is preserved.
Canvas extractBright(int threshByte, const Canvas& src) {
    return generate(src.w, src.h, [&](int x, int y) -> Color {
        const Color c = sampleClamped(src, x, y);
        const double lumD = c.r * luminanceWeightR + c.g * luminanceWeightG + c.b * luminanceWeightB;
        const int lum = static_cast<int>(std::lround(lumD));
        if (lum >= threshByte) return c;
        return { 0, 0, 0, c.a };
    });
}

} // namespace

Canvas boxBlur(int radius, const Canvas& src) {
    if (radius <= 0) return src;
    return blurVertical(radius, blurHorizontal(radius, src));
}

Canvas gaussianBlur(int radius, const Canvas& src) {
    if (radius <= 0) return src;
    Canvas acc = src;
    for (int pass = 0; pass < gaussianPasses; ++pass) acc = boxBlur(radius, acc);
    return acc;
}

Canvas sharpen(const Canvas& src) {
    const Canvas blurred = boxBlur(sharpenBlurRadius, src);
    return generate(src.w, src.h, [&](int x, int y) -> Color {
        const Color o = sampleClamped(src, x, y);
        const Color b = sampleClamped(blurred, x, y);
        const auto mask = [&](std::uint8_t ov, std::uint8_t bv) {
            return clampByte(sharpenOriginalWeight * static_cast<int>(ov) - static_cast<int>(bv));
        };
        return { mask(o.r, b.r), mask(o.g, b.g), mask(o.b, b.b), o.a };
    });
}

Canvas edgeDetect(const Canvas& src) {
    return generate(src.w, src.h, [&](int x, int y) -> Color {
        // Per-channel Sobel gradient magnitude, averaged into a single gray value.
        const auto edge = [&](int channel) {
            const int gx = applyKernel3x3(sobelX, src, x, y, channel);
            const int gy = applyKernel3x3(sobelY, src, x, y, channel);
            const double magnitude = std::sqrt(static_cast<double>(gx * gx + gy * gy));
            return static_cast<int>(clampByte(static_cast<int>(std::lround(magnitude))));
        };
        const int avg = (edge(0) + edge(1) + edge(2)) / rgbChannelCount;
        const std::uint8_t gray = clampByte(avg);
        const std::uint8_t alpha = sampleClamped(src, x, y).a;
        return { gray, gray, gray, alpha };
    });
}

Canvas bloom(int radius, double threshold, const Canvas& src) {
    const int threshByte = static_cast<int>(std::lround(clampUnit(threshold) * channelMax));
    const Canvas blurredBright = gaussianBlur(radius, extractBright(threshByte, src));
    return generate(src.w, src.h, [&](int x, int y) -> Color {
        const Color o = sampleClamped(src, x, y);
        const Color blo = sampleClamped(blurredBright, x, y);
        const auto add = [&](std::uint8_t ov, std::uint8_t bv) {
            return clampByte(static_cast<int>(ov) + static_cast<int>(bv));
        };
        return { add(o.r, blo.r), add(o.g, blo.g), add(o.b, blo.b), o.a };
    });
}

} // namespace tessera
