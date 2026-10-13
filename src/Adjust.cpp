// tessera - Adjust implementation.
// Per-pixel adjustments reuse the Color transforms via mapPixels; LUT grading
// builds a packed 3D table and samples it with trilinear interpolation.
#include "tessera/Adjust.hpp"

#include <algorithm>
#include <cmath>

namespace tessera {
namespace {

constexpr int channelMax = 255;
constexpr double channelMaxF = 255.0;
constexpr std::uint8_t maxByte = 255;

// Minimum posterize levels.
constexpr int posterizeMinLevels = 2;

// Sepia tone matrix coefficients.
constexpr double sepiaRR = 0.393, sepiaRG = 0.769, sepiaRB = 0.189;
constexpr double sepiaGR = 0.349, sepiaGG = 0.686, sepiaGB = 0.168;
constexpr double sepiaBR = 0.272, sepiaBG = 0.534, sepiaBB = 0.131;

// LUT layout.
constexpr int lutMinSize = 2;
constexpr int lutBytesPerEntry = 3;
constexpr int lutRedOffset = 0;
constexpr int lutGreenOffset = 1;
constexpr int lutBlueOffset = 2;
constexpr int lutCeilOffset = 2; // clamp floor so ceil stays in bounds

// Warm LUT channel shifts.
constexpr int warmRedShift = 10;
constexpr int warmGreenShift = 5;
constexpr int warmBlueSuppress = 15;

// Cool LUT channel shifts.
constexpr int coolRedSuppress = 10;
constexpr int coolGreenShift = 3;
constexpr int coolBlueShift = 15;

// Cinematic LUT shadow (teal) and highlight (warm) shifts.
constexpr double cineShadowR = -10.0, cineShadowG = 5.0, cineShadowB = 15.0;
constexpr double cineHighlightR = 10.0, cineHighlightG = 5.0, cineHighlightB = -10.0;

std::uint8_t clampByte(int n) { return static_cast<std::uint8_t>(std::max(0, std::min(channelMax, n))); }

// Luminance byte, reusing Color's BT.709 grayscale.
std::uint8_t luminanceByte(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return grayscaleColor(Color{r, g, b, maxByte}).r;
}

Color posterizeColor(int levels, Color c) {
    const auto quantCh = [&](std::uint8_t ch) {
        const int step = channelMax / std::max(1, levels - 1);
        const int idx = static_cast<int>(ch) / std::max(1, channelMax / levels);
        const int clamped = std::min(levels - 1, idx);
        return clampByte(clamped * step);
    };
    return {quantCh(c.r), quantCh(c.g), quantCh(c.b), c.a};
}

Color thresholdColor(std::uint8_t cutoff, Color c) {
    const std::uint8_t lum = luminanceByte(c.r, c.g, c.b);
    if (lum >= cutoff)
        return {maxByte, maxByte, maxByte, c.a};
    return {0, 0, 0, c.a};
}

Color sepiaColor(Color c) {
    const double rf = c.r, gf = c.g, bf = c.b;
    const std::uint8_t sr = clampByte(static_cast<int>(std::lround(rf * sepiaRR + gf * sepiaRG + bf * sepiaRB)));
    const std::uint8_t sg = clampByte(static_cast<int>(std::lround(rf * sepiaGR + gf * sepiaGG + bf * sepiaGB)));
    const std::uint8_t sb = clampByte(static_cast<int>(std::lround(rf * sepiaBR + gf * sepiaBG + bf * sepiaBB)));
    return {sr, sg, sb, c.a};
}

// ---- LUT helpers ----------------------------------------------------------

// Identity channel value for axis index idx out of size levels.
std::uint8_t indexToChannel(int size, int channel, int ri, int gi, int bi) {
    const auto toVal = [&](int idx) {
        return clampByte(static_cast<int>(std::lround(idx * channelMaxF / (size - 1))));
    };
    switch (channel) {
    case lutRedOffset:
        return toVal(ri);
    case lutGreenOffset:
        return toVal(gi);
    default:
        return toVal(bi);
    }
}

// Read one byte from the LUT table at grid coordinates (ri, gi, bi) and offset.
std::uint8_t lutByte(const std::vector<std::uint8_t> &table, int size, int ri, int gi, int bi, int offset) {
    const int idx = (bi * size * size + gi * size + ri) * lutBytesPerEntry + offset;
    return table[static_cast<std::size_t>(idx)];
}

// Trilinear interpolation of one channel offset within the LUT cube.
double trilinear(double rx, double gx, double bx, const std::vector<std::uint8_t> &table, int size, int ri, int gi,
                 int bi, int offset) {
    const double v000 = lutByte(table, size, ri, gi, bi, offset);
    const double v100 = lutByte(table, size, ri + 1, gi, bi, offset);
    const double v010 = lutByte(table, size, ri, gi + 1, bi, offset);
    const double v110 = lutByte(table, size, ri + 1, gi + 1, bi, offset);
    const double v001 = lutByte(table, size, ri, gi, bi + 1, offset);
    const double v101 = lutByte(table, size, ri + 1, gi, bi + 1, offset);
    const double v011 = lutByte(table, size, ri, gi + 1, bi + 1, offset);
    const double v111 = lutByte(table, size, ri + 1, gi + 1, bi + 1, offset);
    const double c00 = v000 + rx * (v100 - v000);
    const double c10 = v010 + rx * (v110 - v010);
    const double c01 = v001 + rx * (v101 - v001);
    const double c11 = v011 + rx * (v111 - v011);
    const double c0 = c00 + gx * (c10 - c00);
    const double c1 = c01 + gx * (c11 - c01);
    return c0 + bx * (c1 - c0);
}

Color lookupLUT(const ColorLUT &lut, Color c) {
    const int size = lut.size;
    const double sizeF = size - 1;
    const double rPos = c.r * sizeF / channelMaxF;
    const double gPos = c.g * sizeF / channelMaxF;
    const double bPos = c.b * sizeF / channelMaxF;
    const int rFloor = std::min(size - lutCeilOffset, static_cast<int>(std::floor(rPos)));
    const int gFloor = std::min(size - lutCeilOffset, static_cast<int>(std::floor(gPos)));
    const int bFloor = std::min(size - lutCeilOffset, static_cast<int>(std::floor(bPos)));
    const double rFrac = rPos - rFloor;
    const double gFrac = gPos - gFloor;
    const double bFrac = bPos - bFloor;
    const double outR = trilinear(rFrac, gFrac, bFrac, lut.data, size, rFloor, gFloor, bFloor, lutRedOffset);
    const double outG = trilinear(rFrac, gFrac, bFrac, lut.data, size, rFloor, gFloor, bFloor, lutGreenOffset);
    const double outB = trilinear(rFrac, gFrac, bFrac, lut.data, size, rFloor, gFloor, bFloor, lutBlueOffset);
    return {clampByte(static_cast<int>(std::lround(outR))), clampByte(static_cast<int>(std::lround(outG))),
            clampByte(static_cast<int>(std::lround(outB))), c.a};
}

} // namespace

void grayscale(Canvas &c) {
    mapPixels(c, [](Color px) { return grayscaleColor(px); });
}

void invertColors(Canvas &c) {
    mapPixels(c, [](Color px) { return invertColor(px); });
}

void applyTint(Color tint, Canvas &c) {
    mapPixels(c, [&](Color px) { return tintColor(tint, px); });
}

void adjustBrightness(double amount, Canvas &c) {
    mapPixels(c, [&](Color px) { return brightenColor(amount, px); });
}

void adjustContrast(double amount, Canvas &c) {
    mapPixels(c, [&](Color px) { return contrastColor(amount, px); });
}

void adjustSaturation(double amount, Canvas &c) {
    mapPixels(c, [&](Color px) { return saturateColor(amount, px); });
}

void shiftHue(double degrees, Canvas &c) {
    mapPixels(c, [&](Color px) { return shiftHueColor(degrees, px); });
}

void remapColor(Color src, Color dst, Canvas &c) {
    mapPixels(c, [&](Color px) { return px == src ? dst : px; });
}

void remapColors(const std::vector<std::pair<Color, Color>> &mapping, Canvas &c) {
    mapPixels(c, [&](Color px) {
        for (const auto &[src, dst] : mapping)
            if (px == src)
                return dst;
        return px;
    });
}

void posterize(int levels, Canvas &c) {
    const int clamped = std::max(posterizeMinLevels, levels);
    mapPixels(c, [&](Color px) { return posterizeColor(clamped, px); });
}

void threshold(std::uint8_t cutoff, Canvas &c) {
    mapPixels(c, [&](Color px) { return thresholdColor(cutoff, px); });
}

void sepia(Canvas &c) {
    mapPixels(c, [](Color px) { return sepiaColor(px); });
}

ColorLUT identityLUT(int size) {
    const int safeSize = std::max(lutMinSize, size);
    const int entries = safeSize * safeSize * safeSize;
    std::vector<std::uint8_t> table(static_cast<std::size_t>(entries) * lutBytesPerEntry);
    for (int entryIdx = 0; entryIdx < entries; ++entryIdx) {
        const int ri = entryIdx % safeSize;
        const int gi = (entryIdx / safeSize) % safeSize;
        const int bi = entryIdx / (safeSize * safeSize);
        const std::size_t base = static_cast<std::size_t>(entryIdx) * lutBytesPerEntry;
        table[base + lutRedOffset] = indexToChannel(safeSize, lutRedOffset, ri, gi, bi);
        table[base + lutGreenOffset] = indexToChannel(safeSize, lutGreenOffset, ri, gi, bi);
        table[base + lutBlueOffset] = indexToChannel(safeSize, lutBlueOffset, ri, gi, bi);
    }
    return {safeSize, std::move(table)};
}

void applyLUT(const ColorLUT &lut, Canvas &c) {
    mapPixels(c, [&](Color px) { return lookupLUT(lut, px); });
}

ColorLUT modifyLUT(const ColorLUT &lut, const std::function<Color(Color)> &f) {
    const int size = lut.size;
    const int entries = size * size * size;
    std::vector<std::uint8_t> newTable(static_cast<std::size_t>(entries) * lutBytesPerEntry);
    for (int entryIdx = 0; entryIdx < entries; ++entryIdx) {
        const std::size_t base = static_cast<std::size_t>(entryIdx) * lutBytesPerEntry;
        const std::uint8_t origR = lut.data[base + lutRedOffset];
        const std::uint8_t origG = lut.data[base + lutGreenOffset];
        const std::uint8_t origB = lut.data[base + lutBlueOffset];
        const Color adj = f(Color{origR, origG, origB, maxByte});
        newTable[base + lutRedOffset] = adj.r;
        newTable[base + lutGreenOffset] = adj.g;
        newTable[base + lutBlueOffset] = adj.b;
    }
    return {size, std::move(newTable)};
}

ColorLUT warmLUT(int size) {
    return modifyLUT(identityLUT(size), [](Color c) -> Color {
        return {clampByte(static_cast<int>(c.r) + warmRedShift), clampByte(static_cast<int>(c.g) + warmGreenShift),
                clampByte(static_cast<int>(c.b) - warmBlueSuppress), c.a};
    });
}

ColorLUT coolLUT(int size) {
    return modifyLUT(identityLUT(size), [](Color c) -> Color {
        return {clampByte(static_cast<int>(c.r) - coolRedSuppress), clampByte(static_cast<int>(c.g) + coolGreenShift),
                clampByte(static_cast<int>(c.b) + coolBlueShift), c.a};
    });
}

ColorLUT cinematicLUT(int size) {
    return modifyLUT(identityLUT(size), [](Color c) -> Color {
        const double lumNorm = luminanceByte(c.r, c.g, c.b) / channelMaxF;
        const double rShift = cineShadowR + lumNorm * (cineHighlightR - cineShadowR);
        const double gShift = cineShadowG + lumNorm * (cineHighlightG - cineShadowG);
        const double bShift = cineShadowB + lumNorm * (cineHighlightB - cineShadowB);
        return {clampByte(static_cast<int>(std::lround(c.r + rShift))),
                clampByte(static_cast<int>(std::lround(c.g + gShift))),
                clampByte(static_cast<int>(std::lround(c.b + bShift))), c.a};
    });
}

} // namespace tessera
