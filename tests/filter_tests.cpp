#include "support.hpp"

#include <tessera/Filter.hpp>

#include <doctest/doctest.h>

using namespace tessera;
using namespace tessera::test;

TEST_CASE("Blurs leave a uniform canvas unchanged and soften an edge") {
    const Canvas flat = newCanvas(9, 9, orange);
    CHECK(same(boxBlur(2, flat), flat));
    CHECK(same(gaussianBlur(2, flat), flat));
    CHECK(same(boxBlur(0, numbered(4, 4)), numbered(4, 4)));

    Canvas edge = newCanvas(10, 1, black);
    fillRect(edge, 5, 0, 5, 1, white);
    const Canvas soft = boxBlur(1, edge);
    CHECK(getPixel(soft, 4, 0).r > 0);
    CHECK(getPixel(soft, 5, 0).r < 255);
}

TEST_CASE("Sharpening and edge detection see no detail in a flat canvas") {
    const Canvas flat = newCanvas(6, 6, gray);
    CHECK(same(sharpen(flat), flat));
    const Canvas edges = edgeDetect(flat);
    CHECK(getPixel(edges, 3, 3).r == 0);
    CHECK(getPixel(edges, 3, 3).a == 255);

    Canvas step = newCanvas(8, 8, black);
    fillRect(step, 4, 0, 4, 8, white);
    CHECK(getPixel(edgeDetect(step), 4, 4).r > 0);
}

TEST_CASE("Bloom brightens only around bright pixels") {
    const Canvas dark = newCanvas(8, 8, black);
    CHECK(same(bloom(2, 0.5, dark), dark));
    Canvas spot = newCanvas(9, 9, black);
    setPixel(spot, 4, 4, white);
    const Canvas glowing = bloom(2, 0.5, spot);
    CHECK(getPixel(glowing, 5, 4).r > 0);
}
