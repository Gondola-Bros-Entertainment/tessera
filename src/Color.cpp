// tessera - Color implementation: color math and HSL/HSV conversions.
#include "tessera/Color.hpp"

#include <algorithm>
#include <cmath>

namespace tessera {
namespace {

constexpr int    channelMax  = 255;
constexpr double channelMaxF = 255.0;
constexpr double channelMid  = 128.0;
constexpr double colorEps    = 1.0e-10;
constexpr double hueSegment  = 60.0;
constexpr double fullCircle  = 360.0;

// BT.709 luminance weights (sum to 1.0).
constexpr double lumWeightR  = 0.2126;
constexpr double lumWeightG  = 0.7152;
constexpr double lumWeightB  = 0.0722;

double clampUnit(double x) { return std::max(0.0, std::min(1.0, x)); }
double clampSigned(double x) { return std::max(-1.0, std::min(1.0, x)); }

std::uint8_t clampByte(int n) {
    return static_cast<std::uint8_t>(std::max(0, std::min(channelMax, n)));
}
std::uint8_t clampRound(double x) { return clampByte(static_cast<int>(std::lround(x))); }

std::uint8_t lerpChannel(double t, std::uint8_t a, std::uint8_t b) {
    const double fa = a, fb = b;
    return clampRound(std::max(0.0, std::min(channelMaxF, fa + t * (fb - fa))));
}
std::uint8_t mulChannel(std::uint8_t a, std::uint8_t b) {
    return static_cast<std::uint8_t>(static_cast<int>(a) * static_cast<int>(b) / channelMax);
}
std::uint8_t luminance(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return clampRound(r * lumWeightR + g * lumWeightG + b * lumWeightB);
}

double wrapHue(double h) {
    const double wrapped = h - fullCircle * std::floor(h / fullCircle);
    return wrapped < 0.0 ? wrapped + fullCircle : wrapped;
}

double computeHue(double rf, double gf, double bf, double cmax, double delta) {
    if (delta < colorEps) return 0.0;
    if (std::fabs(cmax - rf) < colorEps) return wrapHue(hueSegment * (gf - bf) / delta);
    if (std::fabs(cmax - gf) < colorEps) return wrapHue(hueSegment * ((bf - rf) / delta + 2.0));
    return wrapHue(hueSegment * ((rf - gf) / delta + 4.0));
}

struct Rgb { double r, g, b; };
Rgb hueComponents(int sector, double c, double x) {
    switch (((sector % 6) + 6) % 6) {
        case 0:  return { c, x, 0 };
        case 1:  return { x, c, 0 };
        case 2:  return { 0, c, x };
        case 3:  return { 0, x, c };
        case 4:  return { x, 0, c };
        default: return { c, 0, x };
    }
}

} // namespace

Color lerp(double t, Color a, Color b) {
    const double tc = clampUnit(t);
    return { lerpChannel(tc, a.r, b.r), lerpChannel(tc, a.g, b.g),
             lerpChannel(tc, a.b, b.b), lerpChannel(tc, a.a, b.a) };
}

Color multiply(Color a, Color b) {
    return { mulChannel(a.r, b.r), mulChannel(a.g, b.g), mulChannel(a.b, b.b), mulChannel(a.a, b.a) };
}

Color alphaBlend(Color src, Color dst) {
    if (src.a == channelMax) return src;
    if (src.a == 0) return dst;
    const int srcA = src.a, dstA = dst.a, invSrcA = channelMax - srcA;
    const int outA = srcA + dstA * invSrcA / channelMax;
    if (outA == 0) return transparent;
    const auto blend = [&](std::uint8_t s, std::uint8_t d) {
        const int dstContrib = static_cast<int>(d) * dstA * invSrcA / channelMax;
        const int result = (static_cast<int>(s) * srcA + dstContrib) / outA;
        return static_cast<std::uint8_t>(std::max(0, std::min(channelMax, result)));
    };
    return { blend(src.r, dst.r), blend(src.g, dst.g), blend(src.b, dst.b),
             static_cast<std::uint8_t>(outA) };
}

Color scaleAlpha(double factor, Color c) {
    return { c.r, c.g, c.b, clampRound(c.a * clampUnit(factor)) };
}

HSL toHSL(Color c) {
    const double rf = c.r / channelMaxF, gf = c.g / channelMaxF, bf = c.b / channelMaxF;
    const double cmax = std::max({ rf, gf, bf }), cmin = std::min({ rf, gf, bf });
    const double delta = cmax - cmin;
    const double lightness = (cmax + cmin) / 2.0;
    double saturation = 0.0;
    if (delta >= colorEps) {
        const double denom = 1.0 - std::fabs(2.0 * lightness - 1.0);
        saturation = denom < colorEps ? 0.0 : delta / denom;
    }
    return { computeHue(rf, gf, bf, cmax, delta), clampUnit(saturation), clampUnit(lightness) };
}

Color fromHSL(HSL hsl) {
    const double chroma = (1.0 - std::fabs(2.0 * hsl.l - 1.0)) * hsl.s;
    const double hPrime = wrapHue(hsl.h) / hueSegment;
    const int sector = static_cast<int>(std::floor(hPrime));
    const double frac = hPrime - sector;
    const double secondary = chroma * ((sector % 2 == 0) ? frac : 1.0 - frac);
    const double m = hsl.l - chroma / 2.0;
    const Rgb rgb = hueComponents(sector, chroma, secondary);
    return { clampRound((rgb.r + m) * channelMaxF), clampRound((rgb.g + m) * channelMaxF),
             clampRound((rgb.b + m) * channelMaxF), 255 };
}

HSV toHSV(Color c) {
    const double rf = c.r / channelMaxF, gf = c.g / channelMaxF, bf = c.b / channelMaxF;
    const double cmax = std::max({ rf, gf, bf }), cmin = std::min({ rf, gf, bf });
    const double delta = cmax - cmin;
    const double saturation = cmax < colorEps ? 0.0 : delta / cmax;
    return { computeHue(rf, gf, bf, cmax, delta), saturation, cmax };
}

Color fromHSV(HSV hsv) {
    const double chroma = hsv.v * hsv.s;
    const double hPrime = wrapHue(hsv.h) / hueSegment;
    const int sector = static_cast<int>(std::floor(hPrime));
    const double frac = hPrime - sector;
    const double secondary = chroma * ((sector % 2 == 0) ? frac : 1.0 - frac);
    const double m = hsv.v - chroma;
    const Rgb rgb = hueComponents(sector, chroma, secondary);
    return { clampRound((rgb.r + m) * channelMaxF), clampRound((rgb.g + m) * channelMaxF),
             clampRound((rgb.b + m) * channelMaxF), 255 };
}

Color tintColor(Color tint, Color c) {
    return { mulChannel(tint.r, c.r), mulChannel(tint.g, c.g), mulChannel(tint.b, c.b), c.a };
}

Color invertColor(Color c) {
    return { static_cast<std::uint8_t>(channelMax - c.r), static_cast<std::uint8_t>(channelMax - c.g),
             static_cast<std::uint8_t>(channelMax - c.b), c.a };
}

Color grayscaleColor(Color c) {
    const std::uint8_t lum = luminance(c.r, c.g, c.b);
    return { lum, lum, lum, c.a };
}

Color brightenColor(double amount, Color c) {
    if (amount >= 0.0) {
        const double t = clampUnit(amount);
        const auto lift = [&](std::uint8_t ch) { return clampRound(ch + t * (channelMaxF - ch)); };
        return { lift(c.r), lift(c.g), lift(c.b), c.a };
    }
    const double t = clampUnit(-amount);
    const auto dimCh = [&](std::uint8_t ch) { return clampRound(ch * (1.0 - t)); };
    return { dimCh(c.r), dimCh(c.g), dimCh(c.b), c.a };
}

Color contrastColor(double amount, Color c) {
    const double factor = 1.0 + clampSigned(amount);
    const auto adjust = [&](std::uint8_t ch) { return clampRound(channelMid + (ch - channelMid) * factor); };
    return { adjust(c.r), adjust(c.g), adjust(c.b), c.a };
}

Color saturateColor(double amount, Color c) {
    const double lum = luminance(c.r, c.g, c.b);
    const double factor = 1.0 + clampSigned(amount);
    const auto adjust = [&](std::uint8_t ch) { return clampRound(lum + (ch - lum) * factor); };
    return { adjust(c.r), adjust(c.g), adjust(c.b), c.a };
}

Color shiftHueColor(double degrees, Color c) {
    HSL hsl = toHSL(c);
    hsl.h = wrapHue(hsl.h + degrees);
    return withAlpha(c.a, fromHSL(hsl));
}

} // namespace tessera
