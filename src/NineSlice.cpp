// tessera - NineSlice implementation.
// renderNineSlice builds a fresh canvas in one pass: each destination pixel
// maps to a source pixel via mapCoord (pass-through borders, scaled center).
#include "tessera/NineSlice.hpp"

#include "tessera/Color.hpp"

#include <algorithm>

namespace tessera {
namespace {

// Borders may not exceed this fraction of the source dimension (half per side).
constexpr int borderClampDivisor = 2;

// Map a destination coordinate to its source coordinate.
// Left/top border: pass through. Center: scale proportionally. Right/bottom
// border: offset from the end.
int mapCoord(int dst, int border1, int border2, int centerSrc, int centerDst, int srcSize, int dstSize) {
    if (dst < border1)
        return dst;
    if (dst >= dstSize - border2)
        return srcSize - (dstSize - dst);
    if (centerDst <= 0)
        return border1;
    const int localDst = dst - border1;
    const int mapped = localDst * centerSrc / centerDst;
    return border1 + std::min(centerSrc - 1, mapped);
}

} // namespace

NineSlice nineSlice(const Canvas &canvas, int left, int right, int top, int bottom) {
    return NineSlice{canvas, std::max(0, left), std::max(0, right), std::max(0, top), std::max(0, bottom)};
}

Canvas renderNineSlice(const NineSlice &ns, int targetW, int targetH) {
    const int srcW = ns.canvas.w;
    const int srcH = ns.canvas.h;
    const int left = std::min(ns.left, srcW / borderClampDivisor);
    const int right = std::min(ns.right, srcW / borderClampDivisor);
    const int top = std::min(ns.top, srcH / borderClampDivisor);
    const int bottom = std::min(ns.bottom, srcH / borderClampDivisor);
    const int centerSrcW = std::max(1, srcW - left - right);
    const int centerSrcH = std::max(1, srcH - top - bottom);
    const int centerDstW = std::max(0, targetW - left - right);
    const int centerDstH = std::max(0, targetH - top - bottom);
    return generate(targetW, targetH, [&](int x, int y) {
        const int srcX = mapCoord(x, left, right, centerSrcW, centerDstW, srcW, targetW);
        const int srcY = mapCoord(y, top, bottom, centerSrcH, centerDstH, srcH, targetH);
        if (srcX >= 0 && srcX < srcW && srcY >= 0 && srcY < srcH)
            return getPixel(ns.canvas, srcX, srcY);
        return transparent;
    });
}

} // namespace tessera
