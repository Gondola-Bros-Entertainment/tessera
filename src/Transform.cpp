// tessera - Transform implementation.
#include "tessera/Transform.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace tessera {
namespace {

constexpr double scaleEpsilon = 1.0e-10;
constexpr double pi = 3.14159265358979323846;
constexpr double degToRad = pi / 180.0;

// Offset from a pixel's integer coordinate to its center.
constexpr double pixelCenter = 0.5;
// Divisor that maps a span to its midpoint.
constexpr double halfDivisor = 2.0;

// Bilinear sample of the source at fractional (fx, fy). Out-of-bounds taps read
// as transparent (getPixel), so edges fade out.
Color bilinearSample(const Canvas &c, double fx, double fy) {
    const int ix = static_cast<int>(std::floor(fx));
    const int iy = static_cast<int>(std::floor(fy));
    const double fracX = fx - ix;
    const double fracY = fy - iy;
    const Color c00 = getPixel(c, ix, iy);
    const Color c10 = getPixel(c, ix + 1, iy);
    const Color c01 = getPixel(c, ix, iy + 1);
    const Color c11 = getPixel(c, ix + 1, iy + 1);
    const Color top = lerp(fracX, c00, c10);
    const Color bot = lerp(fracX, c01, c11);
    return lerp(fracY, top, bot);
}

// Bilinear scale to exact (newW, newH). Maps each output pixel center back into
// source space and samples there.
Canvas scaleToImpl(int newW, int newH, const Canvas &c) {
    const double ratioX = static_cast<double>(c.w) / static_cast<double>(newW);
    const double ratioY = static_cast<double>(c.h) / static_cast<double>(newH);
    return generate(newW, newH, [&](int ox, int oy) {
        const double srcX = (static_cast<double>(ox) + pixelCenter) * ratioX - pixelCenter;
        const double srcY = (static_cast<double>(oy) + pixelCenter) * ratioY - pixelCenter;
        return bilinearSample(c, srcX, srcY);
    });
}

// Whether (x, y) has at least one in-bounds opaque 4-neighbor.
bool hasOpaqueNeighbor(const Canvas &c, int x, int y) {
    const auto opaque = [&](int nx, int ny) { return inBounds(c, nx, ny) && getPixel(c, nx, ny).a > 0; };
    return opaque(x - 1, y) || opaque(x + 1, y) || opaque(x, y - 1) || opaque(x, y + 1);
}

} // namespace

Canvas flipH(const Canvas &c) {
    const int w = c.w;
    return generate(w, c.h, [&](int x, int y) { return getPixel(c, w - 1 - x, y); });
}

Canvas flipV(const Canvas &c) {
    const int h = c.h;
    return generate(c.w, h, [&](int x, int y) { return getPixel(c, x, h - 1 - y); });
}

Canvas rotate90(const Canvas &c) {
    const int h = c.h;
    return generate(h, c.w, [&](int x, int y) { return getPixel(c, y, h - 1 - x); });
}

Canvas rotate180(const Canvas &c) {
    const int w = c.w, h = c.h;
    return generate(w, h, [&](int x, int y) { return getPixel(c, w - 1 - x, h - 1 - y); });
}

Canvas rotate270(const Canvas &c) {
    const int w = c.w;
    return generate(c.h, w, [&](int x, int y) { return getPixel(c, w - 1 - y, x); });
}

Canvas scaleNearest(int factor, const Canvas &c) {
    if (factor <= 1)
        return c;
    const int w = c.w;
    const int newW = w * factor;
    const int newH = c.h * factor;
    return generate(newW, newH, [&](int x, int y) { return getPixel(c, x / factor, y / factor); });
}

Canvas rotateArbitrary(double angleDeg, const Canvas &c) {
    if (angleDeg == 0.0)
        return c;
    const int w = c.w;
    const int h = c.h;
    const double radians = angleDeg * degToRad;
    const double cosA = std::cos(radians);
    const double sinA = std::sin(radians);
    const double absCos = std::abs(cosA);
    const double absSin = std::abs(sinA);
    const int newW = static_cast<int>(std::ceil(w * absCos + h * absSin));
    const int newH = static_cast<int>(std::ceil(w * absSin + h * absCos));
    const double centerSrcX = static_cast<double>(w) / halfDivisor;
    const double centerSrcY = static_cast<double>(h) / halfDivisor;
    const double centerDstX = static_cast<double>(newW) / halfDivisor;
    const double centerDstY = static_cast<double>(newH) / halfDivisor;
    return generate(newW, newH, [&](int ox, int oy) {
        const double dxF = static_cast<double>(ox) - centerDstX + pixelCenter;
        const double dyF = static_cast<double>(oy) - centerDstY + pixelCenter;
        const double srcX = dxF * cosA + dyF * sinA + centerSrcX - pixelCenter;
        const double srcY = -dxF * sinA + dyF * cosA + centerSrcY - pixelCenter;
        return bilinearSample(c, srcX, srcY);
    });
}

Canvas scaleBilinear(double factor, const Canvas &c) {
    if (factor <= 0.0)
        return c;
    if (std::abs(factor - 1.0) < scaleEpsilon)
        return c;
    const int newW = std::max(1, static_cast<int>(std::lround(c.w * factor)));
    const int newH = std::max(1, static_cast<int>(std::lround(c.h * factor)));
    return scaleToImpl(newW, newH, c);
}

Canvas scaleTo(int targetW, int targetH, const Canvas &c) {
    return scaleToImpl(std::max(1, targetW), std::max(1, targetH), c);
}

Canvas shearH(double factor, const Canvas &c) {
    if (std::abs(factor) < scaleEpsilon)
        return c;
    const int h = c.h;
    const double maxShift = std::abs(factor) * static_cast<double>(h) / halfDivisor;
    const int newW = c.w + static_cast<int>(std::ceil(halfDivisor * maxShift));
    const int offsetX = static_cast<int>(std::ceil(maxShift));
    return generate(newW, h, [&](int ox, int oy) {
        const double shift = factor * (static_cast<double>(oy) - static_cast<double>(h) / halfDivisor);
        const double srcX = static_cast<double>(ox) - static_cast<double>(offsetX) - shift;
        const double srcY = static_cast<double>(oy);
        return bilinearSample(c, srcX, srcY);
    });
}

Canvas shearV(double factor, const Canvas &c) {
    if (std::abs(factor) < scaleEpsilon)
        return c;
    const int w = c.w;
    const double maxShift = std::abs(factor) * static_cast<double>(w) / halfDivisor;
    const int newH = c.h + static_cast<int>(std::ceil(halfDivisor * maxShift));
    const int offsetY = static_cast<int>(std::ceil(maxShift));
    return generate(w, newH, [&](int ox, int oy) {
        const double shift = factor * (static_cast<double>(ox) - static_cast<double>(w) / halfDivisor);
        const double srcX = static_cast<double>(ox);
        const double srcY = static_cast<double>(oy) - static_cast<double>(offsetY) - shift;
        return bilinearSample(c, srcX, srcY);
    });
}

Canvas outline(Color outlineColor, const Canvas &c) {
    return generate(c.w, c.h, [&](int x, int y) {
        const Color px = getPixel(c, x, y);
        if (px.a == 0 && hasOpaqueNeighbor(c, x, y))
            return outlineColor;
        return px;
    });
}

Canvas dropShadow(int dx, int dy, Color shadowColor, const Canvas &c) {
    const int w = c.w;
    const int h = c.h;
    const int padW = w + std::abs(dx);
    const int padH = h + std::abs(dy);
    const int shadowOffX = std::max(0, dx);
    const int shadowOffY = std::max(0, dy);
    const int origOffX = std::max(0, -dx);
    const int origOffY = std::max(0, -dy);
    return generate(padW, padH, [&](int px, int py) {
        // Original sprite coordinates.
        const int ox = px - origOffX;
        const int oy = py - origOffY;
        const Color origPixel = inBounds(c, ox, oy) ? getPixel(c, ox, oy) : transparent;
        // Shadow sprite coordinates.
        const int sx = px - shadowOffX;
        const int sy = py - shadowOffY;
        const bool hasShadow = inBounds(c, sx, sy) && getPixel(c, sx, sy).a > 0;
        if (origPixel.a > 0)
            return hasShadow ? alphaBlend(origPixel, shadowColor) : origPixel;
        return hasShadow ? shadowColor : transparent;
    });
}

} // namespace tessera
