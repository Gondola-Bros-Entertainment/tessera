#include "support.hpp"

#include <tessera/Compose.hpp>
#include <tessera/NineSlice.hpp>
#include <tessera/Sheet.hpp>
#include <tessera/Tilemap.hpp>

#include <doctest/doctest.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

using namespace tessera;
using namespace tessera::test;

TEST_CASE("Stamping copies only the opaque pixels of the source") {
    Canvas dst = newCanvas(6, 6, blue);
    Canvas src = newCanvas(2, 2, transparent);
    setPixel(src, 0, 0, red);
    stamp(dst, 3, 3, src);
    CHECK(getPixel(dst, 3, 3) == red);
    CHECK(getPixel(dst, 4, 4) == blue);
    stamp(dst, 5, 5, newCanvas(3, 3, red)); // clips at the edge
    CHECK(getPixel(dst, 5, 5) == red);
}

TEST_CASE("Alpha stamping and overlays composite over the destination") {
    Canvas dst = newCanvas(4, 4, blue);
    stampAlpha(dst, 1, 1, newCanvas(1, 1, Color{255, 0, 0, 128}));
    CHECK(near(getPixel(dst, 1, 1), Color{128, 0, 127, 255}));

    Canvas base = newCanvas(3, 3, blue);
    overlay(base, newCanvas(3, 3, red));
    CHECK(getPixel(base, 2, 2) == red);
    Canvas offset = newCanvas(3, 3, blue);
    overlayAt(offset, 2, 2, newCanvas(3, 3, red));
    CHECK(getPixel(offset, 2, 2) == red);
    CHECK(getPixel(offset, 1, 1) == blue);
}

TEST_CASE("Blend modes combine source and destination channels") {
    Canvas additive = newCanvas(1, 1, blue);
    blendCompose(additive, BlendMode::Additive, 0, 0, newCanvas(1, 1, red));
    CHECK(getPixel(additive, 0, 0) == magenta);

    Canvas multiplied = newCanvas(1, 1, orange);
    blendCompose(multiplied, BlendMode::Multiply, 0, 0, newCanvas(1, 1, white));
    CHECK(getPixel(multiplied, 0, 0) == orange);

    Canvas screened = newCanvas(1, 1, orange);
    blendCompose(screened, BlendMode::Screen, 0, 0, newCanvas(1, 1, black));
    CHECK(getPixel(screened, 0, 0) == orange);

    Canvas difference = newCanvas(1, 1, white);
    blendCompose(difference, BlendMode::Difference, 0, 0, newCanvas(1, 1, white));
    CHECK(getPixel(difference, 0, 0) == black);

    for (const BlendMode mode : {BlendMode::Overlay, BlendMode::SoftLight}) {
        Canvas c = newCanvas(1, 1, gray);
        blendCompose(c, mode, 0, 0, newCanvas(1, 1, gray));
        CHECK(getPixel(c, 0, 0).a == 255);
    }
}

TEST_CASE("Masks multiply alpha") {
    Canvas c = newCanvas(2, 1, white);
    Canvas mask = newCanvas(2, 1, white);
    setPixel(mask, 1, 0, transparent);
    maskCanvas(c, mask);
    CHECK(getPixel(c, 0, 0).a == 255);
    CHECK(getPixel(c, 1, 0).a == 0);
}

TEST_CASE("Nine-slices keep their corners and stretch to the target size") {
    Canvas source = newCanvas(6, 6, gray);
    setPixel(source, 0, 0, red);
    setPixel(source, 5, 5, blue);
    const NineSlice slices = nineSlice(source, 2, 2, 2, 2);
    const Canvas panel = renderNineSlice(slices, 40, 20);
    CHECK(panel.w == 40);
    CHECK(panel.h == 20);
    CHECK(getPixel(panel, 0, 0) == red);
    CHECK(getPixel(panel, 39, 19) == blue);
    CHECK(getPixel(panel, 20, 10) == gray);
    CHECK(nineSlice(source, -3, 1, 1, 1).left == 0);
}

TEST_CASE("Tilemaps place each tile in its cell and skip unknown indices") {
    const std::vector<Canvas> tiles{newCanvas(4, 4, red), newCanvas(4, 4, blue)};
    const TilemapConfig config{
        .tileWidth = 4, .tileHeight = 4, .gridWidth = 3, .gridHeight = 2, .tiles = {0, 1, -1, 1, 0, 7}};
    const Canvas map = renderTilemap(tiles, config);
    CHECK(map.w == 12);
    CHECK(map.h == 8);
    CHECK(getPixel(map, 1, 1) == red);
    CHECK(getPixel(map, 5, 1) == blue);
    CHECK(getPixel(map, 9, 1) == transparent);
    CHECK(getPixel(map, 1, 5) == blue);
    CHECK(getPixel(map, 9, 5) == transparent);
}

TEST_CASE("Packed sheets place every sprite without overlap and copy its pixels") {
    const std::vector<std::pair<std::string, Canvas>> items{
        {"small", newCanvas(3, 3, red)}, {"tall", newCanvas(4, 9, green)}, {"wide", newCanvas(10, 2, blue)}};
    const SpriteSheet sheet = packSheet(1, items);
    REQUIRE(sheet.entries.size() == items.size());
    for (const SheetEntry &entry : sheet.entries) {
        const auto source =
            std::find_if(items.begin(), items.end(), [&](const auto &item) { return item.first == entry.name; });
        REQUIRE(source != items.end());
        CHECK(entry.width == source->second.w);
        CHECK(entry.height == source->second.h);
        CHECK(entry.x + entry.width <= sheet.canvas.w);
        CHECK(entry.y + entry.height <= sheet.canvas.h);
        CHECK(getPixel(sheet.canvas, entry.x, entry.y) == getPixel(source->second, 0, 0));
        for (const SheetEntry &other : sheet.entries) {
            if (&other == &entry) {
                continue;
            }
            const bool apart = entry.x + entry.width <= other.x || other.x + other.width <= entry.x ||
                               entry.y + entry.height <= other.y || other.y + other.height <= entry.y;
            CHECK(apart);
        }
    }
}
