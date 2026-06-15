// tessera - tile-based map rendering: a grid of tile indices stamped to one canvas.
#pragma once

#include "Canvas.hpp"

#include <vector>

namespace tessera {

// Configuration for tilemap rendering.
struct TilemapConfig {
    int tileWidth{};              // width of each tile in pixels
    int tileHeight{};             // height of each tile in pixels
    int gridWidth{};              // grid width in tiles
    int gridHeight{};             // grid height in tiles
    std::vector<int> tiles;       // tile indices, row-major; index into the tile list
};

// Render a tilemap from a list of tile canvases and a config.
// Tile index i selects tiles[i]; its top-left tileWidth x tileHeight region is
// stamped into the matching grid cell, copying only non-transparent pixels.
// Negative indices and indices outside the tile list are skipped.
Canvas renderTilemap(const std::vector<Canvas>& tiles, const TilemapConfig& config);

} // namespace tessera
