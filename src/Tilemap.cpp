// tessera - Tilemap implementation.
// Each cell's tile is stamped at a fixed grid offset; cells never overlap, so
// blitting into the shared output canvas in place is safe.
#include "tessera/Tilemap.hpp"

#include "tessera/Color.hpp"

#include <cstddef>

namespace tessera {
namespace {

// Copy the top-left tw x th region of a tile into dst at (destX, destY),
// writing only non-transparent pixels.
void stampTile(Canvas& dst, const Canvas& tile, int destX, int destY, int tw, int th) {
    for (int ty = 0; ty < th; ++ty)
        for (int tx = 0; tx < tw; ++tx) {
            const Color px = getPixel(tile, tx, ty);
            if (px.a > 0) setPixel(dst, destX + tx, destY + ty, px);
        }
}

} // namespace

Canvas renderTilemap(const std::vector<Canvas>& tiles, const TilemapConfig& config) {
    const int tw = config.tileWidth;
    const int th = config.tileHeight;
    const int gw = config.gridWidth;
    const int gh = config.gridHeight;
    const int totalW = gw * tw;
    const int totalH = gh * th;
    Canvas out = newCanvas(totalW, totalH, transparent);
    const std::size_t cellCount = config.tiles.size();
    for (std::size_t gridIdx = 0; gridIdx < cellCount; ++gridIdx) {
        const int tileIdx = config.tiles[gridIdx];
        if (tileIdx < 0) continue;
        if (static_cast<std::size_t>(tileIdx) >= tiles.size()) continue;
        const int gi = static_cast<int>(gridIdx);
        const int gridX = (gi % gw) * tw;
        const int gridY = (gi / gw) * th;
        stampTile(out, tiles[static_cast<std::size_t>(tileIdx)], gridX, gridY, tw, th);
    }
    return out;
}

} // namespace tessera
