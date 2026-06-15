// tessera - canvas compositing: stamping, overlaying, blend modes, masking.
#pragma once

#include "Canvas.hpp"

namespace tessera {

// Pixel-level blend modes (applied per channel, then mixed by source alpha).
enum class BlendMode {
    Multiply,    // darken:  src * dst / 255
    Screen,      // lighten: 255 - (255-src)(255-dst)/255
    Overlay,     // multiply or screen, by dst brightness
    Additive,    // src + dst, clamped
    SoftLight,   // gentle lightening with reduced contrast
    Difference,  // |src - dst|
};

// Copy src onto dst at (x,y), overwriting only where src is non-transparent.
void stamp(Canvas& dst, int x, int y, const Canvas& src);

// Alpha-blend src onto dst at (x,y) (Porter-Duff "over").
void stampAlpha(Canvas& dst, int x, int y, const Canvas& src);

// Alpha-blend top onto dst, at the origin / at an offset.
void overlay(Canvas& dst, const Canvas& top);
void overlayAt(Canvas& dst, int x, int y, const Canvas& top);

// Composite src onto dst at (x,y) using a blend mode; source alpha respected.
void blendCompose(Canvas& dst, BlendMode mode, int x, int y, const Canvas& src);

// Multiply each pixel's alpha by the mask's alpha (out-of-bounds mask = opaque).
void maskCanvas(Canvas& c, const Canvas& mask);

} // namespace tessera
