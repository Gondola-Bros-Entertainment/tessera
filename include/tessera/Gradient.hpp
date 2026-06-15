// tessera - gradient generators for backgrounds, panels, and color ramps.
#pragma once

#include "Canvas.hpp"

namespace tessera {

// Left-to-right when horizontal, else top-to-bottom.
Canvas linearGradient(int w, int h, Color start, Color end, bool horizontal);

// Circular ramp from inner (at the center) to outer (at and beyond radius).
Canvas radialGradient(int w, int h, int cx, int cy, int radius, Color inner, Color outer);

// Top-left to bottom-right.
Canvas diagonalGradient(int w, int h, Color start, Color end);

} // namespace tessera
