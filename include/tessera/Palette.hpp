// tessera - indexed color palettes, palette swap, and median-cut quantization.
#pragma once

#include "Color.hpp"

#include <vector>

namespace tessera {

// An ordered collection of colors, indexed by position.
using Palette = std::vector<Color>;

// ---- built-in palettes -----------------------------------------------------
Palette grayscale4(); // 4-shade grayscale ramp (evenly spaced)
Palette grayscale8(); // 8-shade grayscale ramp
Palette gameboy();    // Game Boy DMG green palette
Palette nes();        // small NES-inspired palette (16 colors)

// ---- operations ------------------------------------------------------------

// Build a palette from a list of colors.
Palette fromColors(std::vector<Color> colors);

// Number of colors in the palette.
int paletteSize(const Palette& pal);

// Look up a color by index. Returns the last color for out-of-range indices,
// or transparent for an empty palette; a negative index returns the first.
Color paletteColor(const Palette& pal, int idx);

// Replace a color matching a src palette entry with the dst entry at the same
// position; colors with no match are returned unchanged.
Color paletteSwap(const Palette& src, const Palette& dst, Color color);

// Interpolate two palettes element-wise at t in [0,1]. The shorter palette is
// padded with transparent so the result has the length of the longer one.
Palette paletteLerp(double t, const Palette& a, const Palette& b);

// Extract up to n representative colors from pixel colors via median cut.
Palette extractPalette(int targetCount, const std::vector<Color>& colors);

// Map a color to the nearest palette color by squared RGB distance.
// Returns the color unchanged for an empty palette.
Color quantizeColor(const Palette& pal, Color c);

} // namespace tessera
