#include "support.hpp"

#include <tessera/Isometric.hpp>

#include <doctest/doctest.h>

#include <vector>

using namespace tessera;
using namespace tessera::test;

TEST_CASE("The default isometric tile is 64 by 32") {
    const IsoConfig config = defaultIsoConfig();
    CHECK(config.tileWidth == 64);
    CHECK(config.tileHeight == 32);
}

TEST_CASE("World and screen coordinates convert back and forth") {
    const IsoConfig config = defaultIsoConfig();
    for (int wy = 0; wy < 6; ++wy) {
        for (int wx = 0; wx < 6; ++wx) {
            const Point screen = worldToScreen(config, wx, wy);
            const Point world = screenToWorld(config, screen.x, screen.y);
            CHECK(world.x == wx);
            CHECK(world.y == wy);
        }
    }
    // One step along world x moves half a tile right and down on screen.
    const Point origin = worldToScreen(config, 0, 0);
    const Point step = worldToScreen(config, 1, 0);
    CHECK(step.x - origin.x == config.tileWidth / 2);
    CHECK(step.y - origin.y == config.tileHeight / 2);
}

TEST_CASE("Diamond hit tests accept the center and reject the bounding box corners") {
    const IsoConfig config = defaultIsoConfig();
    CHECK(pointInDiamond(config, 32, 16, 0, 0));
    CHECK_FALSE(pointInDiamond(config, 1, 1, 0, 0));
    CHECK_FALSE(pointInDiamond(config, 62, 30, 0, 0));
}

TEST_CASE("Depth sorting draws back to front") {
    CHECK(isoDepthCompare(0, 0, 1, 1) < 0);
    CHECK(isoDepthCompare(2, 2, 1, 1) > 0);
    CHECK(isoDepthCompare(1, 1, 1, 1) == 0);
    CHECK(isoDepthCompare(2, 0, 0, 2) < 0); // same depth: larger y in front
}

TEST_CASE("Diamonds fill their center and leave their corners empty") {
    const IsoConfig config = defaultIsoConfig();
    Canvas c = newCanvas(64, 32, transparent);
    fillDiamond(c, config, 0, 0, white);
    CHECK(getPixel(c, 32, 16) == white);
    CHECK(getPixel(c, 0, 0) == transparent);
    CHECK(getPixel(c, 63, 31) == transparent);

    Canvas edges = newCanvas(64, 32, transparent);
    drawDiamond(edges, config, 0, 0, white);
    CHECK(opaque(edges) > 0);
    CHECK(getPixel(edges, 32, 16) == transparent);
}

TEST_CASE("Isometric maps fit every tile and skip unknown indices") {
    const IsoConfig config = defaultIsoConfig();
    Canvas tile = newCanvas(64, 32, transparent);
    fillDiamond(tile, config, 0, 0, green);
    const std::vector<IsoTile> tiles{{.index = 1, .canvas = tile}};
    const Canvas map = renderIsoMap(config, tiles, {{1, 1}, {1, 9}});
    CHECK(map.w >= 2 * config.tileWidth);
    CHECK(map.h >= 2 * config.tileHeight);
    CHECK(opaque(map) > 0);

    const Canvas empty = renderIsoMap(config, tiles, {});
    CHECK(empty.w == 1);
    CHECK(empty.h == 1);
    CHECK(opaque(empty) == 0);
}
