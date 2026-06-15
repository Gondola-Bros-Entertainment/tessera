// tessera - isometric projection: grid<->screen conversion, diamond hit testing,
// depth sorting, diamond drawing, and isometric tilemap rendering. Screen origin
// top-left, x right, y down; world (0,0) at the back corner of the map.
#pragma once

#include "Canvas.hpp"
#include "Color.hpp"

#include <vector>

namespace tessera {

// Isometric tile dimensions in pixels. Standard 2:1 uses width:height of 2:1
// (e.g. 64 wide by 32 tall).
struct IsoConfig {
    int tileWidth{};   // full tile width in pixels (diamond tip to tip, horizontal)
    int tileHeight{};  // full tile height in pixels (diamond tip to tip, vertical)
};

// Standard 2:1 isometric: 64 pixels wide, 32 pixels tall.
IsoConfig defaultIsoConfig();

// ---- coordinate conversion -------------------------------------------------

// World grid coordinates to screen pixel position.
Point worldToScreen(const IsoConfig& config, int wx, int wy);

// Screen pixel position back to world grid coordinates (rounded).
Point screenToWorld(const IsoConfig& config, int screenX, int screenY);

// ---- hit testing -----------------------------------------------------------

// True if screen point (px, py) lies inside the diamond whose bounding-box
// top-left corner is at (dx, dy).
bool pointInDiamond(const IsoConfig& config, int px, int py, int dx, int dy);

// ---- depth sorting ---------------------------------------------------------

// Compare two world positions for back-to-front order. Returns <0, 0, or >0:
// larger (wx + wy) sorts later (in front); ties broken by larger wy in front.
int isoDepthCompare(int wx1, int wy1, int wx2, int wy2);

// ---- drawing ---------------------------------------------------------------

// Draw an isometric diamond outline; bounding-box top-left at (sx, sy).
void drawDiamond(Canvas& canvas, const IsoConfig& config, int sx, int sy, Color color);

// Fill an isometric diamond by scanline; bounding-box top-left at (sx, sy).
void fillDiamond(Canvas& canvas, const IsoConfig& config, int sx, int sy, Color color);

// ---- map rendering ---------------------------------------------------------

// One tile-list entry: a tile index paired with its canvas.
struct IsoTile {
    int index{};
    Canvas canvas;
};

// Render an isometric tilemap. Tiles are looked up by index in the tile list;
// the grid is row-major (outer vector is rows). Tiles render back-to-front by
// isometric depth. Output is sized to fit all tiles; indices not present in the
// tile list are skipped. An empty grid yields a 1x1 transparent canvas.
Canvas renderIsoMap(const IsoConfig& config,
                    const std::vector<IsoTile>& tiles,
                    const std::vector<std::vector<int>>& grid);

} // namespace tessera
