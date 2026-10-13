// tessera - ordered (Bayer) dithering for palette reduction.
#pragma once

#include "Canvas.hpp"
#include "Palette.hpp"

namespace tessera {

// Bayer dither matrix sizes, coarsest to finest.
enum class DitherMatrix {
    Bayer2, // 2x2 (coarsest)
    Bayer4, // 4x4 (moderate)
    Bayer8, // 8x8 (finest)
};

// Reduce a canvas to a palette via ordered dithering: each pixel is nudged by a
// position-dependent Bayer threshold, then mapped to the nearest palette color.
// Returns a fresh canvas; the input is returned unchanged for an empty palette.
Canvas orderedDither(DitherMatrix matrix, const Palette &palette, const Canvas &canvas);

} // namespace tessera
