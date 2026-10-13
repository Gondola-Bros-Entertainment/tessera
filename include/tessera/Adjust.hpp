// tessera - canvas-wide color adjustments and LUT color grading.
#pragma once

#include "Canvas.hpp"
#include "Color.hpp"

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace tessera {

// ---- grayscale and inversion ----------------------------------------------
void grayscale(Canvas &c);    // BT.709 luminance
void invertColors(Canvas &c); // negate RGB, preserve alpha

// ---- tinting ---------------------------------------------------------------
void applyTint(Color tint, Canvas &c); // multiply RGB by tint, keep alpha

// ---- brightness, contrast, saturation -------------------------------------
void adjustBrightness(double amount, Canvas &c); // amount in [-1,1]
void adjustContrast(double amount, Canvas &c);   // amount in [-1,1]
void adjustSaturation(double amount, Canvas &c); // amount in [-1,1]

// ---- hue -------------------------------------------------------------------
void shiftHue(double degrees, Canvas &c);

// ---- color remapping -------------------------------------------------------
void remapColor(Color src, Color dst, Canvas &c);                                 // exact match
void remapColors(const std::vector<std::pair<Color, Color>> &mapping, Canvas &c); // first match wins

// ---- stylization -----------------------------------------------------------
void posterize(int levels, Canvas &c);          // reduce each channel to n levels (>= 2)
void threshold(std::uint8_t cutoff, Canvas &c); // luminance >= cutoff -> white else black
void sepia(Canvas &c);                          // grayscale + warm tint

// ---- color LUT grading -----------------------------------------------------

// A 3D color lookup table. size levels per channel; data holds size^3 entries
// of 3 bytes (RGB), packed so the index is (b*size*size + g*size + r)*3.
struct ColorLUT {
    int size{};
    std::vector<std::uint8_t> data;
};

ColorLUT identityLUT(int size);                // output equals input; size clamped to >= 2
void applyLUT(const ColorLUT &lut, Canvas &c); // trilinear lookup, alpha preserved

// Build a new LUT by transforming every entry of an existing LUT.
ColorLUT modifyLUT(const ColorLUT &lut, const std::function<Color(Color)> &f);

ColorLUT warmLUT(int size);      // shift toward orange/amber
ColorLUT coolLUT(int size);      // shift toward blue
ColorLUT cinematicLUT(int size); // teal shadows, warm highlights

} // namespace tessera
