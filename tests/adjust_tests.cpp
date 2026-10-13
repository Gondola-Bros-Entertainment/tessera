#include "support.hpp"

#include <tessera/Adjust.hpp>

#include <doctest/doctest.h>

#include <utility>
#include <vector>

using namespace tessera;
using namespace tessera::test;

TEST_CASE("Grayscale, inversion and tint act on every pixel") {
    Canvas c = numbered(4, 4);
    grayscale(c);
    CHECK(count(c, [](Color col) { return col.r != col.g || col.g != col.b; }) == 0);

    const Canvas original = numbered(4, 4);
    Canvas twice = original;
    invertColors(twice);
    invertColors(twice);
    CHECK(same(twice, original));

    Canvas tinted = original;
    applyTint(white, tinted);
    CHECK(same(tinted, original));
}

TEST_CASE("Brightness, contrast, saturation and hue do nothing at zero") {
    const Canvas original = numbered(5, 5);
    for (const auto adjust : {adjustBrightness, adjustContrast, adjustSaturation, shiftHue}) {
        Canvas c = original;
        adjust(0.0, c);
        for (int y = 0; y < c.h; ++y) {
            for (int x = 0; x < c.w; ++x) {
                REQUIRE(near(getPixel(c, x, y), getPixel(original, x, y)));
            }
        }
    }
    Canvas brighter = newCanvas(1, 1, gray);
    adjustBrightness(0.5, brighter);
    CHECK(getPixel(brighter, 0, 0).r > gray.r);
}

TEST_CASE("Remapping replaces exact matches only") {
    Canvas c = newCanvas(2, 1, red);
    setPixel(c, 1, 0, blue);
    remapColor(red, green, c);
    CHECK(getPixel(c, 0, 0) == green);
    CHECK(getPixel(c, 1, 0) == blue);
    remapColors({{blue, white}, {blue, black}}, c);
    CHECK(getPixel(c, 1, 0) == white);
}

TEST_CASE("Stylization reduces colors") {
    Canvas poster = numbered(16, 16);
    posterize(2, poster);
    CHECK(count(poster, [](Color col) { return (col.r != 0 && col.r != 255) || (col.g != 0 && col.g != 255); }) == 0);

    Canvas split = newCanvas(2, 1, white);
    setPixel(split, 1, 0, darkGray);
    threshold(128, split);
    CHECK(getPixel(split, 0, 0) == white);
    CHECK(getPixel(split, 1, 0) == black);

    Canvas aged = newCanvas(1, 1, gray);
    sepia(aged);
    const Color warm = getPixel(aged, 0, 0);
    CHECK(warm.r >= warm.g);
    CHECK(warm.g >= warm.b);
}

TEST_CASE("An identity lookup table changes nothing, and graded tables do") {
    const Canvas original = numbered(6, 6);
    Canvas graded = original;
    applyLUT(identityLUT(17), graded);
    for (int y = 0; y < graded.h; ++y) {
        for (int x = 0; x < graded.w; ++x) {
            REQUIRE(near(getPixel(graded, x, y), getPixel(original, x, y)));
        }
    }
    CHECK(identityLUT(1).size == 2);
    const ColorLUT darkened = modifyLUT(identityLUT(4), [](Color) { return black; });
    Canvas dark = newCanvas(2, 2, white);
    applyLUT(darkened, dark);
    CHECK(getPixel(dark, 0, 0) == black);

    for (const ColorLUT &lut : {warmLUT(8), coolLUT(8), cinematicLUT(8)}) {
        CHECK(lut.size == 8);
        CHECK(lut.data.size() == 8u * 8u * 8u * 3u);
    }
    Canvas warm = newCanvas(1, 1, gray);
    applyLUT(warmLUT(8), warm);
    CHECK(getPixel(warm, 0, 0).r > getPixel(warm, 0, 0).b);
}
