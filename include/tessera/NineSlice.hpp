// tessera - nine-slice UI panel scaling.
// A nine-slice splits a canvas into 9 regions (4 corners, 4 edges, 1 center):
// corners keep their size, edges stretch along one axis, the center stretches
// in both. Used for retro UI (health bars, dialog boxes, menus).
#pragma once

#include "Canvas.hpp"

namespace tessera {

// A nine-slice definition: source canvas plus border insets (clamped >= 0).
struct NineSlice {
    Canvas canvas;  // source canvas to slice
    int left{};     // left border width in pixels
    int right{};    // right border width in pixels
    int top{};      // top border height in pixels
    int bottom{};   // bottom border height in pixels
};

// Build a nine-slice from a canvas and border insets. Insets are clamped to be
// non-negative.
NineSlice nineSlice(const Canvas& canvas, int left, int right, int top, int bottom);

// Render a nine-slice at a target size, producing a new canvas of the given
// dimensions with the regions properly stretched.
Canvas renderNineSlice(const NineSlice& ns, int targetW, int targetH);

} // namespace tessera
