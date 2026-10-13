// tessera - Canvas implementation: pixel access and draw primitives.
#include "tessera/Canvas.hpp"

#include <algorithm>
#include <cstdlib>

namespace tessera {

Canvas newCanvas(int w, int h, Color fill) {
    return generate(w, h, [&](int, int) { return fill; });
}

Color getPixel(const Canvas &c, int x, int y) {
    if (!inBounds(c, x, y))
        return transparent;
    const int i = pixelIndex(c.w, x, y);
    return {c.rgba[static_cast<std::size_t>(i)], c.rgba[static_cast<std::size_t>(i) + 1],
            c.rgba[static_cast<std::size_t>(i) + 2], c.rgba[static_cast<std::size_t>(i) + 3]};
}

void setPixel(Canvas &c, int x, int y, Color col) {
    if (!inBounds(c, x, y))
        return;
    const std::size_t i = static_cast<std::size_t>(pixelIndex(c.w, x, y));
    c.rgba[i] = col.r;
    c.rgba[i + 1] = col.g;
    c.rgba[i + 2] = col.b;
    c.rgba[i + 3] = col.a;
}

void blendPixel(Canvas &c, int x, int y, Color col) {
    if (!inBounds(c, x, y))
        return;
    setPixel(c, x, y, alphaBlend(col, getPixel(c, x, y)));
}

void hLine(Canvas &c, int x0, int x1, int y, Color col) {
    if (y < 0 || y >= c.h)
        return;
    const int minX = std::max(0, std::min(x0, x1));
    const int maxX = std::min(c.w - 1, std::max(x0, x1));
    for (int x = minX; x <= maxX; ++x)
        setPixel(c, x, y, col);
}

void drawLine(Canvas &c, int x0, int y0, int x1, int y1, Color col) {
    const int dx = std::abs(x1 - x0);
    const int dy = -std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int cx = x0, cy = y0;
    for (;;) {
        setPixel(c, cx, cy, col);
        if (cx == x1 && cy == y1)
            break;
        const int e2 = err * 2;
        if (e2 >= dy) {
            err += dy;
            cx += sx;
        }
        if (e2 <= dx) {
            err += dx;
            cy += sy;
        }
    }
}

void fillRect(Canvas &c, int x, int y, int w, int h, Color col) {
    if (w <= 0 || h <= 0)
        return;
    const int minX = std::max(0, x), maxX = std::min(c.w - 1, x + w - 1);
    const int minY = std::max(0, y), maxY = std::min(c.h - 1, y + h - 1);
    for (int row = minY; row <= maxY; ++row)
        for (int colX = minX; colX <= maxX; ++colX)
            setPixel(c, colX, row, col);
}

void drawRect(Canvas &c, int x, int y, int w, int h, Color col) {
    if (w <= 0 || h <= 0)
        return;
    const int x2 = x + w - 1, y2 = y + h - 1;
    hLine(c, x, x2, y, col);
    hLine(c, x, x2, y2, col);
    for (int row = std::max(0, y); row <= std::min(c.h - 1, y2); ++row) {
        setPixel(c, x, row, col);
        setPixel(c, x2, row, col);
    }
}

void drawCircle(Canvas &c, int cx, int cy, int radius, Color col) {
    if (radius <= 0) {
        setPixel(c, cx, cy, col);
        return;
    }
    int x = 0, y = radius, d = 1 - radius;
    while (x <= y) {
        setPixel(c, cx + x, cy + y, col);
        setPixel(c, cx - x, cy + y, col);
        setPixel(c, cx + x, cy - y, col);
        setPixel(c, cx - x, cy - y, col);
        setPixel(c, cx + y, cy + x, col);
        setPixel(c, cx - y, cy + x, col);
        setPixel(c, cx + y, cy - x, col);
        setPixel(c, cx - y, cy - x, col);
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            --y;
        }
        ++x;
    }
}

void fillCircle(Canvas &c, int cx, int cy, int radius, Color col) {
    if (radius <= 0) {
        setPixel(c, cx, cy, col);
        return;
    }
    int x = 0, y = radius, d = 1 - radius;
    while (x <= y) {
        hLine(c, cx - x, cx + x, cy + y, col);
        hLine(c, cx - x, cx + x, cy - y, col);
        hLine(c, cx - y, cx + y, cy + x, col);
        hLine(c, cx - y, cx + y, cy - x, col);
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            --y;
        }
        ++x;
    }
}

Canvas crop(const Canvas &c, int x, int y, int w, int h) {
    if (w <= 0 || h <= 0)
        return newCanvas(1, 1, transparent);
    return generate(w, h, [&](int px, int py) { return getPixel(c, px + x, py + y); });
}

void canvasOpacity(Canvas &c, double factor) {
    mapPixels(c, [&](Color px) { return scaleAlpha(factor, px); });
}

} // namespace tessera
