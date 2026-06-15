// tessera - canvas transforms: flip, rotate, scale, shear, outline, shadow.
#pragma once

#include "Canvas.hpp"

namespace tessera {

// ---- flip ------------------------------------------------------------------
Canvas flipH(const Canvas& c);   // mirror left-right
Canvas flipV(const Canvas& c);   // mirror top-bottom

// ---- rotate ----------------------------------------------------------------
Canvas rotate90(const Canvas& c);    // clockwise; w and h swap
Canvas rotate180(const Canvas& c);
Canvas rotate270(const Canvas& c);   // clockwise (= 90 counter-clockwise)

// Rotate by an arbitrary angle (degrees, clockwise) with bilinear sampling.
// The output is sized to contain the full rotated image.
Canvas rotateArbitrary(double angleDeg, const Canvas& c);

// ---- scale -----------------------------------------------------------------
// Nearest-neighbor upscale by an integer factor (factor <= 1 returns a copy).
Canvas scaleNearest(int factor, const Canvas& c);
// Bilinear scale by a positive factor (factor <= 0 returns a copy).
Canvas scaleBilinear(double factor, const Canvas& c);
// Bilinear scale to exact dimensions (each clamps to a minimum of 1).
Canvas scaleTo(int targetW, int targetH, const Canvas& c);

// ---- shear -----------------------------------------------------------------
// Shift each row by factor * (y - height/2) pixels; positive shifts bottom right.
Canvas shearH(double factor, const Canvas& c);
// Shift each column by factor * (x - width/2) pixels; positive shifts right down.
Canvas shearV(double factor, const Canvas& c);

// ---- effects ---------------------------------------------------------------
// Fill each transparent pixel adjacent to an opaque one with outlineColor.
Canvas outline(Color outlineColor, const Canvas& c);
// Composite a shadow (shadowColor where the sprite is opaque) offset by (dx,dy)
// behind the original. The canvas grows to fit the offset.
Canvas dropShadow(int dx, int dy, Color shadowColor, const Canvas& c);

} // namespace tessera
