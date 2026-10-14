// tessera - vector shapes: circles, ellipses, rounded rectangles, capsules,
// polygons, curves and rings with solid or gradient fills and outlines,
// rasterized with anti-aliased edges at any size.
#pragma once

#include "Canvas.hpp"
#include "Color.hpp"

#include <variant>
#include <vector>

namespace tessera {

// A point or offset in a drawing's design units, x right and y down.
struct VecD {
    double x{};
    double y{};
};

// ---- paints ----------------------------------------------------------------
// Blends start to end along the line from -> to; points past either end take
// that end's color. Colors blend as stored, in sRGB.
struct LinearGradient {
    VecD from{};
    VecD to{};
    Color start{};
    Color end{};
};

// Blends inner at center to outer at radius and beyond.
struct RadialGradient {
    VecD center{};
    double radius{};
    Color inner{};
    Color outer{};
};

using Paint = std::variant<Color, LinearGradient, RadialGradient>;

// ---- shapes ----------------------------------------------------------------
// Rotations are radians, clockwise on the canvas.
struct Circle {
    VecD center{};
    double radius{};
};

struct Ellipse {
    VecD center{};
    VecD radii{};
    double rotation{};
};

// A rectangle of half extents halfSize whose corners round with cornerRadius,
// which clamps to the shorter half extent.
struct RoundRect {
    VecD center{};
    VecD halfSize{};
    double cornerRadius{};
    double rotation{};
};

// A segment thickened to radius on every side, so its ends are round.
struct Capsule {
    VecD from{};
    VecD to{};
    double radius{};
};

// A simple polygon, convex or not, grown outward by rounding, which also
// rounds its corners. Fewer than three points draw nothing.
struct Polygon {
    std::vector<VecD> points;
    double rounding{};
};

// A quadratic Bezier curve from start to end, pulled toward control, stroked
// width wide with round ends.
struct Curve {
    VecD start{};
    VecD control{};
    VecD end{};
    double width{};
};

// A circle's outline, width wide and centered on radius.
struct Ring {
    VecD center{};
    double radius{};
    double width{};
};

using Shape = std::variant<Circle, Ellipse, RoundRect, Capsule, Polygon, Curve, Ring>;

// A shape filled with fill, then outlined in stroke strokeWidth wide, centered
// on the shape's edge. A strokeWidth of zero draws no outline.
struct Layer {
    Shape shape{};
    Paint fill{};
    Color stroke{};
    double strokeWidth{};
};

// Layers in design units, painted in order over an area width by height.
struct Drawing {
    double width{};
    double height{};
    std::vector<Layer> layers;
};

// Rasterize drawing onto a new transparent canvas pixelWidth by pixelHeight
// (each at least 1), mapping its area onto the whole canvas. Each edge is
// anti-aliased from its exact distance to the pixel center, so a drawing is
// crisp at any size. Layers composite in linear light with the straight-alpha
// "over" operator, and a partly covered pixel keeps its shape's color.
// Distances convert to pixels at the smaller axis scale, so a canvas with the
// drawing's aspect ratio gives exact edges. A drawing without area is empty.
Canvas rasterize(const Drawing &drawing, int pixelWidth, int pixelHeight);

} // namespace tessera
