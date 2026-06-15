// tessera - canvas compositing implementation.
#include "tessera/Compose.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace tessera {
namespace {

constexpr int    channelMax      = 255;
constexpr double channelMaxF      = 255.0;
constexpr int    overlayMidpoint  = 128;  // dst brightness split for Overlay

std::uint8_t clampByte(int n) {
    return static_cast<std::uint8_t>(std::max(0, std::min(channelMax, n)));
}

std::uint8_t blendChannel(BlendMode mode, std::uint8_t s, std::uint8_t d) {
    const int si = s, di = d;
    switch (mode) {
        case BlendMode::Multiply:
            return clampByte(si * di / channelMax);
        case BlendMode::Screen:
            return clampByte(channelMax - (channelMax - si) * (channelMax - di) / channelMax);
        case BlendMode::Overlay:
            return di < overlayMidpoint
                       ? clampByte(2 * si * di / channelMax)
                       : clampByte(channelMax - 2 * (channelMax - si) * (channelMax - di) / channelMax);
        case BlendMode::Additive:
            return clampByte(si + di);
        case BlendMode::SoftLight: {
            const double sf = si / channelMaxF, df = di / channelMaxF;
            const double result = sf < 0.5 ? df - (1.0 - 2.0 * sf) * df * (1.0 - df)
                                           : df + (2.0 * sf - 1.0) * (std::sqrt(df) - df);
            return clampByte(static_cast<int>(std::lround(result * channelMaxF)));
        }
        case BlendMode::Difference:
            return static_cast<std::uint8_t>(std::abs(si - di));
    }
    return s; // unreachable; satisfies -Wreturn-type
}

} // namespace

void stamp(Canvas& dst, int ox, int oy, const Canvas& src) {
    for (int sy = 0; sy < src.h; ++sy)
        for (int sx = 0; sx < src.w; ++sx) {
            const Color s = getPixel(src, sx, sy);
            if (s.a > 0) setPixel(dst, ox + sx, oy + sy, s);
        }
}

void stampAlpha(Canvas& dst, int ox, int oy, const Canvas& src) {
    for (int sy = 0; sy < src.h; ++sy)
        for (int sx = 0; sx < src.w; ++sx) {
            const Color s = getPixel(src, sx, sy);
            if (s.a > 0) blendPixel(dst, ox + sx, oy + sy, s);
        }
}

void overlayAt(Canvas& dst, int x, int y, const Canvas& top) { stampAlpha(dst, x, y, top); }
void overlay(Canvas& dst, const Canvas& top) { stampAlpha(dst, 0, 0, top); }

void blendCompose(Canvas& dst, BlendMode mode, int ox, int oy, const Canvas& src) {
    for (int sy = 0; sy < src.h; ++sy)
        for (int sx = 0; sx < src.w; ++sx) {
            const Color s = getPixel(src, sx, sy);
            if (s.a == 0) continue;
            const int dx = ox + sx, dy = oy + sy;
            if (!inBounds(dst, dx, dy)) continue;
            const Color d = getPixel(dst, dx, dy);
            const int srcA = s.a;
            const auto mix = [&](std::uint8_t blended, std::uint8_t dch) {
                return clampByte((blended * srcA + dch * (channelMax - srcA)) / channelMax);
            };
            const Color out{
                mix(blendChannel(mode, s.r, d.r), d.r),
                mix(blendChannel(mode, s.g, d.g), d.g),
                mix(blendChannel(mode, s.b, d.b), d.b),
                clampByte(srcA + d.a * (channelMax - srcA) / channelMax),
            };
            setPixel(dst, dx, dy, out);
        }
}

void maskCanvas(Canvas& c, const Canvas& mask) {
    for (int y = 0; y < c.h; ++y)
        for (int x = 0; x < c.w; ++x) {
            Color px = getPixel(c, x, y);
            const std::uint8_t maskA = (x < mask.w && y < mask.h) ? getPixel(mask, x, y).a
                                                                  : static_cast<std::uint8_t>(channelMax);
            px.a = clampByte(px.a * maskA / channelMax);
            setPixel(c, x, y, px);
        }
}

} // namespace tessera
