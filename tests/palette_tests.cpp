#include "support.hpp"

#include <tessera/Dither.hpp>
#include <tessera/Gradient.hpp>
#include <tessera/Palette.hpp>

#include <doctest/doctest.h>

#include <algorithm>
#include <vector>

using namespace tessera;
using namespace tessera::test;

TEST_CASE("Built-in palettes have their documented sizes") {
    CHECK(paletteSize(grayscale4()) == 4);
    CHECK(paletteSize(grayscale8()) == 8);
    CHECK(paletteSize(gameboy()) == 4);
    CHECK(paletteSize(nes()) == 16);
    CHECK(paletteSize(fromColors({red, blue})) == 2);
}

TEST_CASE("Palette lookups clamp out-of-range indices") {
    const Palette pal = fromColors({red, green, blue});
    CHECK(paletteColor(pal, 1) == green);
    CHECK(paletteColor(pal, 9) == blue);
    CHECK(paletteColor(pal, -1) == red);
    CHECK(paletteColor(Palette{}, 0) == transparent);
}

TEST_CASE("Palette swaps map by position and leave other colors alone") {
    const Palette from = fromColors({red, green});
    const Palette to = fromColors({white, black});
    CHECK(paletteSwap(from, to, green) == black);
    CHECK(paletteSwap(from, to, blue) == blue);
}

TEST_CASE("Palette interpolation pads the shorter palette") {
    const Palette a = fromColors({black, black, black});
    const Palette b = fromColors({white});
    const Palette mixed = paletteLerp(1.0, a, b);
    REQUIRE(paletteSize(mixed) == 3);
    CHECK(mixed[0] == white);
    CHECK(mixed[2] == transparent);
    CHECK(paletteLerp(0.0, a, b)[1] == black);
}

TEST_CASE("Quantization finds representative and nearest colors") {
    std::vector<Color> pixels(50, red);
    pixels.insert(pixels.end(), 50, blue);
    const Palette extracted = extractPalette(2, pixels);
    CHECK(paletteSize(extracted) <= 2);
    CHECK(std::find(extracted.begin(), extracted.end(), red) != extracted.end());

    const Palette pal = fromColors({black, white});
    CHECK(quantizeColor(pal, lightGray) == white);
    CHECK(quantizeColor(pal, darkGray) == black);
    CHECK(quantizeColor(Palette{}, orange) == orange);
}

TEST_CASE("Ordered dithering uses only palette colors") {
    const Canvas ramp = linearGradient(32, 4, black, white, true);
    const Palette pal = grayscale4();
    for (const DitherMatrix matrix : {DitherMatrix::Bayer2, DitherMatrix::Bayer4, DitherMatrix::Bayer8}) {
        const Canvas dithered = orderedDither(matrix, pal, ramp);
        CHECK(dithered.w == ramp.w);
        CHECK(count(dithered, [&](Color col) {
                  return std::find(pal.begin(), pal.end(), withAlpha(255, col)) == pal.end();
              }) == 0);
    }
    CHECK(same(orderedDither(DitherMatrix::Bayer4, Palette{}, ramp), ramp));
}
