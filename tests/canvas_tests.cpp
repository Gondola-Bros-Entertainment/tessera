#include "support.hpp"

#include <tessera/Canvas.hpp>
#include <tessera/Color.hpp>

#include <doctest/doctest.h>

using namespace tessera;
using namespace tessera::test;

TEST_CASE("A new canvas is filled with one color and sized width by height") {
    const Canvas c = newCanvas(3, 2, red);
    CHECK(c.w == 3);
    CHECK(c.h == 2);
    CHECK(c.rgba.size() == 3u * 2u * 4u);
    CHECK(count(c, [](Color col) { return col == red; }) == 6);
}

TEST_CASE("Generated canvases evaluate every pixel and clamp to at least 1 by 1") {
    const Canvas c = generate(
        4, 3, [](int x, int y) { return Color{static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), 0, 255}; });
    CHECK(getPixel(c, 3, 2) == Color{3, 2, 0, 255});
    const Canvas empty = generate(0, -5, [](int, int) { return white; });
    CHECK(empty.w == 1);
    CHECK(empty.h == 1);
}

TEST_CASE("Pixels read and write inside the canvas and ignore everything outside it") {
    Canvas c = newCanvas(4, 4, transparent);
    setPixel(c, 1, 2, green);
    CHECK(getPixel(c, 1, 2) == green);
    setPixel(c, -1, 0, green);
    setPixel(c, 4, 4, green);
    CHECK(opaque(c) == 1);
    CHECK(getPixel(c, -1, 0) == transparent);
    CHECK(getPixel(c, 0, 4) == transparent);
}

TEST_CASE("Blending composites over the existing pixel") {
    Canvas c = newCanvas(1, 1, blue);
    blendPixel(c, 0, 0, red);
    CHECK(getPixel(c, 0, 0) == red);
    blendPixel(c, 0, 0, transparent);
    CHECK(getPixel(c, 0, 0) == red);
    Canvas half = newCanvas(1, 1, blue);
    blendPixel(half, 0, 0, Color{255, 0, 0, 128});
    CHECK(near(getPixel(half, 0, 0), Color{128, 0, 127, 255}));
}

TEST_CASE("Lines include both endpoints") {
    Canvas c = newCanvas(8, 8, transparent);
    drawLine(c, 0, 0, 5, 5, white);
    for (int i = 0; i <= 5; ++i) {
        CHECK(getPixel(c, i, i) == white);
    }
    CHECK(opaque(c) == 6);
    Canvas row = newCanvas(8, 1, transparent);
    hLine(row, 6, 2, 0, white);
    CHECK(opaque(row) == 5);
}

TEST_CASE("Rectangles fill or outline exactly their area and clip at the edges") {
    Canvas c = newCanvas(10, 10, transparent);
    fillRect(c, 2, 3, 4, 5, white);
    CHECK(opaque(c) == 20);
    CHECK(getPixel(c, 2, 3) == white);
    CHECK(getPixel(c, 5, 7) == white);
    CHECK(getPixel(c, 6, 7) == transparent);

    Canvas clipped = newCanvas(4, 4, transparent);
    fillRect(clipped, -2, -2, 4, 4, white);
    CHECK(opaque(clipped) == 4);

    Canvas outline = newCanvas(10, 10, transparent);
    drawRect(outline, 1, 1, 5, 4, white);
    CHECK(getPixel(outline, 1, 1) == white);
    CHECK(getPixel(outline, 5, 4) == white);
    CHECK(getPixel(outline, 3, 2) == transparent);
    CHECK(opaque(outline) == 14);
}

TEST_CASE("Circles are symmetric and reach exactly their radius") {
    Canvas c = newCanvas(21, 21, transparent);
    fillCircle(c, 10, 10, 6, white);
    CHECK(getPixel(c, 10, 10) == white);
    CHECK(getPixel(c, 16, 10) == white);
    CHECK(getPixel(c, 17, 10) == transparent);
    CHECK(getPixel(c, 10, 4) == white);
    CHECK(getPixel(c, 10, 3) == transparent);
    for (int y = 0; y < c.h; ++y) {
        for (int x = 0; x < c.w; ++x) {
            REQUIRE(getPixel(c, x, y) == getPixel(c, 20 - x, y));
            REQUIRE(getPixel(c, x, y) == getPixel(c, x, 20 - y));
        }
    }
    Canvas ring = newCanvas(21, 21, transparent);
    drawCircle(ring, 10, 10, 6, white);
    CHECK(getPixel(ring, 16, 10) == white);
    CHECK(getPixel(ring, 10, 10) == transparent);
    // A circle drawn across the edge clips instead of writing outside the canvas.
    Canvas edge = newCanvas(4, 4, transparent);
    fillCircle(edge, 0, 0, 3, white);
    CHECK(opaque(edge) > 0);
}

TEST_CASE("Cropping copies a region and opacity scales alpha") {
    const Canvas source = numbered(6, 6);
    const Canvas part = crop(source, 2, 1, 3, 2);
    CHECK(part.w == 3);
    CHECK(part.h == 2);
    CHECK(getPixel(part, 0, 0) == getPixel(source, 2, 1));
    CHECK(getPixel(part, 2, 1) == getPixel(source, 4, 2));

    Canvas faded = newCanvas(2, 2, white);
    canvasOpacity(faded, 0.5);
    CHECK(near(getPixel(faded, 1, 1), Color{255, 255, 255, 128}));
}

TEST_CASE("Color math blends, multiplies and composites") {
    CHECK(lerp(0.0, red, blue) == red);
    CHECK(lerp(1.0, red, blue) == blue);
    CHECK(lerp(2.0, red, blue) == blue);
    CHECK(near(lerp(0.5, black, white), Color{128, 128, 128, 255}));
    CHECK(multiply(white, orange) == orange);
    CHECK(multiply(black, orange) == Color{0, 0, 0, 255});
    CHECK(alphaBlend(red, blue) == red);
    CHECK(alphaBlend(transparent, blue) == blue);
    CHECK(near(scaleAlpha(0.5, white), Color{255, 255, 255, 128}));
    CHECK(withAlpha(7, green) == Color{0, 255, 0, 7});
}

TEST_CASE("HSL and HSV round-trip the primary and secondary colors") {
    for (const Color c : {red, green, blue, yellow, cyan, magenta, white, black, gray}) {
        CHECK(near(fromHSL(toHSL(c)), c));
        CHECK(near(fromHSV(toHSV(c)), c));
    }
    CHECK(toHSL(red).h == doctest::Approx(0.0));
    CHECK(toHSV(green).h == doctest::Approx(120.0));
    CHECK(toHSL(white).l == doctest::Approx(1.0));
}

TEST_CASE("Color transforms change only what they say") {
    CHECK(invertColor(Color{0, 0, 0, 40}) == Color{255, 255, 255, 40});
    CHECK(grayscaleColor(white) == white);
    const Color gray_red = grayscaleColor(red);
    CHECK(gray_red.r == gray_red.g);
    CHECK(gray_red.g == gray_red.b);
    CHECK(near(gray_red, Color{54, 54, 54, 255})); // BT.709: 0.2126 * 255
    CHECK(tintColor(white, Color{10, 20, 30, 99}) == Color{10, 20, 30, 99});
    CHECK(brightenColor(0.5, gray).r > gray.r);
    CHECK(brightenColor(-0.5, gray).r < gray.r);
    CHECK(contrastColor(0.5, lightGray).r > lightGray.r);
    CHECK(saturateColor(-1.0, red).r == saturateColor(-1.0, red).g);
    CHECK(near(shiftHueColor(120.0, red), green));
}
