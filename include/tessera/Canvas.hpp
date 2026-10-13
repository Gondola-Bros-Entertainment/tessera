// tessera - the 2D RGBA pixel grid plus drawing primitives.
#pragma once

#include "Color.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace tessera {

// A width x height RGBA buffer, row-major, 4 bytes per pixel.
// Invariant: rgba.size() == w * h * 4. Origin top-left, x right, y down.
struct Canvas {
    int w{};
    int h{};
    std::vector<std::uint8_t> rgba;
};

// A 2D integer point or coordinate.
struct Point {
    int x{}, y{};
};

inline int pixelIndex(int width, int x, int y) { return (y * width + x) * 4; }
inline bool inBounds(const Canvas &c, int x, int y) { return x >= 0 && x < c.w && y >= 0 && y < c.h; }

// Build a canvas by evaluating a color function at each pixel.
// F has signature Color(int x, int y). Dimensions clamp to a minimum of 1.
template <class F> Canvas generate(int w, int h, F &&at) {
    const int sw = w < 1 ? 1 : w;
    const int sh = h < 1 ? 1 : h;
    Canvas c{sw, sh, std::vector<std::uint8_t>(static_cast<std::size_t>(sw) * static_cast<std::size_t>(sh) * 4u)};
    std::size_t o = 0;
    for (int y = 0; y < sh; ++y)
        for (int x = 0; x < sw; ++x) {
            const Color col = at(x, y);
            c.rgba[o++] = col.r;
            c.rgba[o++] = col.g;
            c.rgba[o++] = col.b;
            c.rgba[o++] = col.a;
        }
    return c;
}

// Apply a function to every pixel in place. F has signature Color(Color).
template <class F> void mapPixels(Canvas &c, F &&f) {
    const std::size_t n = c.rgba.size();
    for (std::size_t o = 0; o < n; o += 4) {
        const Color out = f(Color{c.rgba[o], c.rgba[o + 1], c.rgba[o + 2], c.rgba[o + 3]});
        c.rgba[o] = out.r;
        c.rgba[o + 1] = out.g;
        c.rgba[o + 2] = out.b;
        c.rgba[o + 3] = out.a;
    }
}

Canvas newCanvas(int w, int h, Color fill);

Color getPixel(const Canvas &c, int x, int y); // transparent if out of bounds
void setPixel(Canvas &c, int x, int y, Color col);
void blendPixel(Canvas &c, int x, int y, Color col); // alpha-composite over existing

void hLine(Canvas &c, int x0, int x1, int y, Color col);
void drawLine(Canvas &c, int x0, int y0, int x1, int y1, Color col);
void drawRect(Canvas &c, int x, int y, int w, int h, Color col);
void fillRect(Canvas &c, int x, int y, int w, int h, Color col);
void drawCircle(Canvas &c, int cx, int cy, int radius, Color col);
void fillCircle(Canvas &c, int cx, int cy, int radius, Color col);

Canvas crop(const Canvas &c, int x, int y, int w, int h);
void canvasOpacity(Canvas &c, double factor);

} // namespace tessera
