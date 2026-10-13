#include "support.hpp"

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
