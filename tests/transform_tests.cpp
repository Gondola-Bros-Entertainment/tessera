#include "support.hpp"

#include <tessera/Draw.hpp>
#include <tessera/Transform.hpp>

#include <doctest/doctest.h>

using namespace tessera;
using namespace tessera::test;

TEST_CASE("Flips mirror pixels and undo themselves") {
    const Canvas c = numbered(5, 3);
    const Canvas h = flipH(c);
    CHECK(getPixel(h, 0, 1) == getPixel(c, 4, 1));
    CHECK(same(flipH(h), c));
    const Canvas v = flipV(c);
    CHECK(getPixel(v, 2, 0) == getPixel(c, 2, 2));
    CHECK(same(flipV(v), c));
}

TEST_CASE("Quarter turns swap dimensions and compose") {
    const Canvas c = numbered(5, 3);
    const Canvas once = rotate90(c);
    CHECK(once.w == 3);
    CHECK(once.h == 5);
    // Clockwise: the top-left pixel moves to the top-right.
    CHECK(getPixel(once, 2, 0) == getPixel(c, 0, 0));
    CHECK(same(rotate90(rotate90(rotate90(once))), c));
    CHECK(same(rotate180(c), rotate90(once)));
    CHECK(same(rotate270(c), rotate90(rotate180(c))));
}

TEST_CASE("Arbitrary rotation grows the canvas to fit") {
    const Canvas c = newCanvas(10, 4, white);
    const Canvas quarter = rotateArbitrary(90.0, c);
    CHECK(quarter.w >= 4);
    CHECK(quarter.h >= 10);
    // A 10 by 10 square turned 45 degrees spans 10 * sqrt(2), about 14 pixels.
    const Canvas tilted = rotateArbitrary(45.0, newCanvas(10, 10, white));
    CHECK(tilted.w >= 14);
    CHECK(tilted.h >= 14);
    CHECK(opaque(tilted) > 0);
}

TEST_CASE("Scaling resizes to the requested size") {
    const Canvas c = numbered(3, 2);
    const Canvas doubled = scaleNearest(2, c);
    CHECK(doubled.w == 6);
    CHECK(doubled.h == 4);
    CHECK(getPixel(doubled, 5, 3) == getPixel(c, 2, 1));
    CHECK(getPixel(doubled, 4, 2) == getPixel(c, 2, 1));
    CHECK(same(scaleNearest(1, c), c));

    const Canvas half = scaleBilinear(0.5, newCanvas(8, 6, red));
    CHECK(half.w == 4);
    CHECK(half.h == 3);
    CHECK(near(getPixel(half, 1, 1), red));
    CHECK(same(scaleBilinear(0.0, c), c));

    const Canvas exact = scaleTo(7, 5, c);
    CHECK(exact.w == 7);
    CHECK(exact.h == 5);
    CHECK(scaleTo(0, 0, c).w == 1);
}

TEST_CASE("Shearing by zero changes nothing") {
    const Canvas c = numbered(4, 4);
    CHECK(same(shearH(0.0, c), c));
    CHECK(same(shearV(0.0, c), c));
    CHECK(opaque(shearH(0.5, newCanvas(6, 6, white))) > 0);
}

TEST_CASE("Outlines surround opaque pixels without touching them") {
    Canvas c = newCanvas(7, 7, transparent);
    setPixel(c, 3, 3, white);
    const Canvas outlined = outline(red, c);
    CHECK(outlined.w == 7);
    CHECK(getPixel(outlined, 3, 3) == white);
    CHECK(getPixel(outlined, 2, 3) == red);
    CHECK(getPixel(outlined, 3, 4) == red);
    CHECK(getPixel(outlined, 0, 0) == transparent);
}

TEST_CASE("Drop shadows grow the canvas and sit behind the sprite") {
    const Canvas c = newCanvas(4, 4, white);
    const Canvas shadowed = dropShadow(2, 3, black, c);
    CHECK(shadowed.w == 6);
    CHECK(shadowed.h == 7);
    CHECK(getPixel(shadowed, 0, 0) == white);
    CHECK(getPixel(shadowed, 5, 6) == black);
}

TEST_CASE("Area downscaling averages each block, past-the-edge pixels counting as transparent") {
    const Canvas solid = downscaleArea(4, newCanvas(8, 8, red));
    CHECK(solid.w == 2);
    CHECK(solid.h == 2);
    CHECK(getPixel(solid, 1, 1) == red);

    // Half of the block is white and half is empty: half coverage, and still white.
    Canvas half = newCanvas(4, 4, transparent);
    fillRect(half, 0, 0, 2, 4, white);
    CHECK(getPixel(downscaleArea(4, half), 0, 0) == Color{255, 255, 255, 128});

    // Red and green average in linear light: 0.5 encodes to 188, not 128.
    Canvas mixed = newCanvas(2, 2, red);
    fillRect(mixed, 1, 0, 1, 2, green);
    CHECK(near(getPixel(downscaleArea(2, mixed), 0, 0), Color{188, 188, 0, 255}));

    // A 5 by 4 canvas leaves the second block one column of four.
    const Canvas ragged = downscaleArea(4, newCanvas(5, 4, white));
    CHECK(ragged.w == 2);
    CHECK(ragged.h == 1);
    CHECK(getPixel(ragged, 1, 0) == Color{255, 255, 255, 64});

    CHECK(same(downscaleArea(1, numbered(3, 2)), numbered(3, 2)));
}

TEST_CASE("Shapes drawn large and downscaled have anti-aliased edges without dark fringes") {
    constexpr int factor = 4;
    Canvas large = newCanvas(16 * factor, 16 * factor, transparent);
    fillEllipse(large, 8 * factor, 8 * factor, 6 * factor, 6 * factor, white);
    const Canvas smooth = downscaleArea(factor, large);
    Canvas hard = newCanvas(16, 16, transparent);
    fillEllipse(hard, 8, 8, 6, 6, white);
    const auto partial = [](Color c) { return c.a > 0 && c.a < 255; };
    CHECK(count(hard, partial) == 0);
    CHECK(count(smooth, partial) > 0);
    CHECK(count(smooth, [](Color c) { return c.a > 0 && (c.r != 255 || c.g != 255 || c.b != 255); }) == 0);
}

TEST_CASE("Edge color spreads into transparent pixels ring by ring") {
    Canvas dot = newCanvas(5, 5, transparent);
    setPixel(dot, 2, 2, red);
    const Canvas spread = spreadEdgeColor(dot);
    CHECK(getPixel(spread, 2, 2) == red);
    CHECK(getPixel(spread, 1, 2) == Color{255, 0, 0, 0});
    CHECK(getPixel(spread, 0, 0) == Color{255, 0, 0, 0});

    // The middle pixel is two rings from both ends, so it takes the mean of the first rings.
    Canvas row = newCanvas(5, 1, transparent);
    setPixel(row, 0, 0, red);
    setPixel(row, 4, 0, blue);
    const Canvas met = spreadEdgeColor(row);
    CHECK(getPixel(met, 1, 0) == Color{255, 0, 0, 0});
    CHECK(getPixel(met, 3, 0) == Color{0, 0, 255, 0});
    CHECK(near(getPixel(met, 2, 0), Color{188, 0, 188, 0}));
}

TEST_CASE("Spreading edge color keeps visible pixels and empty canvases") {
    Canvas faint = newCanvas(2, 1, transparent);
    setPixel(faint, 0, 0, Color{10, 20, 30, 40});
    const Canvas spread = spreadEdgeColor(faint);
    CHECK(getPixel(spread, 0, 0) == Color{10, 20, 30, 40});
    CHECK(getPixel(spread, 1, 0) == Color{10, 20, 30, 0});
    const Canvas empty = newCanvas(3, 3, Color{9, 9, 9, 0});
    CHECK(same(spreadEdgeColor(empty), empty));
}
