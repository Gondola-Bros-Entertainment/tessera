#include "support.hpp"

#include <tessera/Draw.hpp>
#include <tessera/Gradient.hpp>

#include <doctest/doctest.h>

using namespace tessera;
using namespace tessera::test;

TEST_CASE("Thick lines cover more than thin ones along the same path") {
    Canvas thin = newCanvas(20, 20, transparent);
    drawLine(thin, 2, 10, 17, 10, white);
    Canvas thick = newCanvas(20, 20, transparent);
    drawThickLine(thick, 2, 10, 17, 10, 4, white);
    CHECK(opaque(thick) > opaque(thin));
    CHECK(getPixel(thick, 10, 10) == white);
}

TEST_CASE("Polygons fill their interior and outline their edges") {
    Canvas c = newCanvas(20, 20, transparent);
    fillPolygon(c, {{2, 2}, {17, 2}, {17, 17}, {2, 17}}, white);
    CHECK(getPixel(c, 10, 10) == white);
    CHECK(getPixel(c, 1, 10) == transparent);
    CHECK(getPixel(c, 18, 10) == transparent);

    Canvas triangle = newCanvas(20, 20, transparent);
    fillPolygon(triangle, {{10, 1}, {18, 18}, {2, 18}}, white);
    CHECK(getPixel(triangle, 10, 12) == white);
    CHECK(getPixel(triangle, 2, 2) == transparent);

    Canvas edges = newCanvas(20, 20, transparent);
    drawPolygon(edges, {{2, 2}, {17, 2}, {17, 17}, {2, 17}}, white);
    CHECK(getPixel(edges, 2, 10) == white);
    CHECK(getPixel(edges, 10, 10) == transparent);

    Canvas degenerate = newCanvas(4, 4, transparent);
    fillPolygon(degenerate, {}, white);
    fillPolygon(degenerate, {{1, 1}}, white);
    CHECK(opaque(degenerate) <= 1);
}

TEST_CASE("Ellipses reach their radii on both axes") {
    Canvas c = newCanvas(30, 20, transparent);
    fillEllipse(c, 15, 10, 10, 5, white);
    CHECK(getPixel(c, 15, 10) == white);
    CHECK(getPixel(c, 25, 10) == white);
    CHECK(getPixel(c, 26, 10) == transparent);
    CHECK(getPixel(c, 15, 15) == white);
    CHECK(getPixel(c, 15, 16) == transparent);

    Canvas ring = newCanvas(30, 20, transparent);
    drawEllipse(ring, 15, 10, 10, 5, white);
    CHECK(getPixel(ring, 25, 10) == white);
    CHECK(getPixel(ring, 15, 10) == transparent);
}

TEST_CASE("Arcs draw only their span of the ellipse") {
    Canvas c = newCanvas(30, 30, transparent);
    drawArc(c, 15, 15, 10, 10, 0.0, 90.0, white);
    CHECK(opaque(c) > 0);
    Canvas full = newCanvas(30, 30, transparent);
    drawArc(full, 15, 15, 10, 10, 0.0, 360.0, white);
    CHECK(opaque(full) > opaque(c));
}

TEST_CASE("Curves start and end on their endpoints") {
    Canvas c = newCanvas(40, 40, transparent);
    drawBezier(c, {2, 30}, {20, 0}, {37, 30}, white);
    CHECK(getPixel(c, 2, 30) == white);
    CHECK(getPixel(c, 37, 30) == white);

    Canvas cubic = newCanvas(40, 40, transparent);
    drawCubicBezier(cubic, {2, 35}, {10, 0}, {30, 0}, {37, 35}, white);
    CHECK(getPixel(cubic, 2, 35) == white);
    CHECK(getPixel(cubic, 37, 35) == white);

    Canvas spline = newCanvas(40, 40, transparent);
    drawCatmullRom(spline, {{2, 20}, {12, 5}, {25, 35}, {37, 20}}, white);
    CHECK(opaque(spline) > 0);
    CHECK(getPixel(spline, 12, 5) == white);
    CHECK(getPixel(spline, 25, 35) == white);
}

TEST_CASE("Anti-aliased lines shade pixels partially along a slope") {
    Canvas c = newCanvas(20, 20, transparent);
    drawAALine(c, 1, 1, 18, 9, white);
    const int partial = count(c, [](Color col) { return col.a != 0 && col.a != 255; });
    CHECK(partial > 0);
}

TEST_CASE("Pattern fill tiles a pattern across a region") {
    Canvas pattern = newCanvas(2, 2, transparent);
    setPixel(pattern, 0, 0, red);
    Canvas c = newCanvas(8, 8, transparent);
    patternFill(c, 0, 0, 8, 8, pattern);
    CHECK(getPixel(c, 0, 0) == red);
    CHECK(getPixel(c, 2, 2) == red);
    CHECK(getPixel(c, 6, 4) == red);
    CHECK(getPixel(c, 1, 0) == transparent);
}

TEST_CASE("Rounded rectangles cut their corners") {
    Canvas c = newCanvas(20, 20, transparent);
    fillRoundRect(c, 2, 2, 16, 12, 4, white);
    CHECK(getPixel(c, 10, 8) == white);
    CHECK(getPixel(c, 2, 2) == transparent);
    CHECK(getPixel(c, 6, 2) == white);

    Canvas edges = newCanvas(20, 20, transparent);
    drawRoundRect(edges, 2, 2, 16, 12, 4, white);
    CHECK(getPixel(edges, 10, 2) == white);
    CHECK(getPixel(edges, 10, 8) == transparent);
}

TEST_CASE("Gradients run from their start color to their end color") {
    const Canvas across = linearGradient(10, 3, black, white, true);
    CHECK(getPixel(across, 0, 1) == black);
    CHECK(getPixel(across, 9, 1) == white);
    CHECK(getPixel(across, 4, 0).r < getPixel(across, 6, 0).r);

    const Canvas down = linearGradient(3, 10, black, white, false);
    CHECK(getPixel(down, 1, 0) == black);
    CHECK(getPixel(down, 1, 9) == white);

    const Canvas radial = radialGradient(21, 21, 10, 10, 10, red, transparent);
    CHECK(getPixel(radial, 10, 10) == red);
    CHECK(getPixel(radial, 0, 0) == transparent);

    const Canvas diagonal = diagonalGradient(10, 10, black, white);
    CHECK(getPixel(diagonal, 0, 0) == black);
    CHECK(getPixel(diagonal, 9, 9) == white);
}
