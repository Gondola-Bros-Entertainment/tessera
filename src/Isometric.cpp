// tessera - Isometric implementation.
// Coordinate transforms and hit testing are pure value math; drawing and map
// rendering mutate / build a canvas. Diamonds are stamped back-to-front, so a
// later (front) tile may overwrite an earlier one - the intended depth order.
#include "tessera/Isometric.hpp"

#include "tessera/Compose.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace tessera {
namespace {

// Factor for computing half-tile dimensions.
constexpr int tileHalvingFactor = 2;
// Divisor used in the inverse isometric transform.
constexpr double inverseScaleFactor = 2.0;
// Default tile size for standard 2:1 isometric projection.
constexpr int defaultTileWidth = 64;
constexpr int defaultTileHeight = 32;

// Round half to even (std::lround would round half away from zero, differing at
// exact .5 boundaries).
int roundHalfEven(double v) {
    const double r = std::nearbyint(v); // honours the default round-to-nearest-even mode
    return static_cast<int>(r);
}

// One grid cell for depth-sorted rendering.
struct Cell {
    int wx{};
    int wy{};
    int tileIdx{};
};

} // namespace

IsoConfig defaultIsoConfig() { return IsoConfig{defaultTileWidth, defaultTileHeight}; }

Point worldToScreen(const IsoConfig &config, int wx, int wy) {
    const int halfW = config.tileWidth / tileHalvingFactor;
    const int halfH = config.tileHeight / tileHalvingFactor;
    const int screenX = (wx - wy) * halfW;
    const int screenY = (wx + wy) * halfH;
    return {screenX, screenY};
}

Point screenToWorld(const IsoConfig &config, int screenX, int screenY) {
    const int halfW = config.tileWidth / tileHalvingFactor;
    const int halfH = config.tileHeight / tileHalvingFactor;
    const double ratioX = static_cast<double>(screenX) / std::max(1, halfW);
    const double ratioY = static_cast<double>(screenY) / std::max(1, halfH);
    const int worldX = roundHalfEven((ratioX + ratioY) / inverseScaleFactor);
    const int worldY = roundHalfEven((ratioY - ratioX) / inverseScaleFactor);
    return {worldX, worldY};
}

bool pointInDiamond(const IsoConfig &config, int px, int py, int dx, int dy) {
    const int halfW = config.tileWidth / tileHalvingFactor;
    const int halfH = config.tileHeight / tileHalvingFactor;
    const int cx = dx + halfW;
    const int cy = dy + halfH;
    const int distX = std::abs(px - cx);
    const int distY = std::abs(py - cy);
    return distX * halfH + distY * halfW <= halfW * halfH;
}

int isoDepthCompare(int wx1, int wy1, int wx2, int wy2) {
    const int sum1 = wx1 + wy1;
    const int sum2 = wx2 + wy2;
    if (sum1 < sum2)
        return -1;
    if (sum1 > sum2)
        return 1;
    if (wy1 < wy2)
        return -1;
    if (wy1 > wy2)
        return 1;
    return 0;
}

void drawDiamond(Canvas &canvas, const IsoConfig &config, int sx, int sy, Color color) {
    const int tw = config.tileWidth;
    const int th = config.tileHeight;
    const int halfW = tw / tileHalvingFactor;
    const int halfH = th / tileHalvingFactor;
    const int topX = sx + halfW;
    const int topY = sy;
    const int rightX = sx + tw - 1;
    const int rightY = sy + halfH;
    const int bottomX = sx + halfW;
    const int bottomY = sy + th - 1;
    const int leftX = sx;
    const int leftY = sy + halfH;
    drawLine(canvas, topX, topY, rightX, rightY, color);
    drawLine(canvas, rightX, rightY, bottomX, bottomY, color);
    drawLine(canvas, bottomX, bottomY, leftX, leftY, color);
    drawLine(canvas, leftX, leftY, topX, topY, color);
}

void fillDiamond(Canvas &canvas, const IsoConfig &config, int sx, int sy, Color color) {
    const int tw = config.tileWidth;
    const int th = config.tileHeight;
    const int halfW = tw / tileHalvingFactor;
    const int halfH = th / tileHalvingFactor;
    const int diamondCenterX = sx + halfW;
    for (int row = 0; row < th; ++row) {
        int edgeSpan;
        if (row < halfH) {
            // Upper half: expanding from top.
            edgeSpan = halfW * row / std::max(1, halfH);
        } else {
            // Lower half: contracting toward bottom.
            const int rowsFromBottom = th - 1 - row;
            edgeSpan = halfW * rowsFromBottom / std::max(1, halfH);
        }
        const int edgeLeft = diamondCenterX - edgeSpan;
        const int edgeRight = diamondCenterX + edgeSpan;
        hLine(canvas, edgeLeft, edgeRight, sy + row, color);
    }
}

Canvas renderIsoMap(const IsoConfig &config, const std::vector<IsoTile> &tiles,
                    const std::vector<std::vector<int>> &grid) {
    const int gridRows = static_cast<int>(grid.size());
    int gridCols = 0;
    for (const std::vector<int> &row : grid)
        gridCols = std::max(gridCols, static_cast<int>(row.size()));

    if (gridRows == 0 || gridCols == 0)
        return newCanvas(1, 1, transparent);

    const int halfW = config.tileWidth / tileHalvingFactor;
    const int halfH = config.tileHeight / tileHalvingFactor;
    // Canvas spans from the leftmost diamond to the rightmost tip, and from the
    // topmost diamond to the bottommost tip.
    const int totalWidth = (gridRows + gridCols) * halfW;
    const int totalHeight = (gridRows + gridCols) * halfH;
    // Offset so that world (0,0) maps to the top-centre.
    const int originOffsetX = (gridRows - 1) * halfW;
    const int originOffsetY = 0;
    Canvas out = newCanvas(totalWidth, totalHeight, transparent);

    // Expand all cells as (wx, wy, tileIndex), in row-major order.
    std::vector<Cell> cells;
    for (int wy = 0; wy < gridRows; ++wy) {
        const std::vector<int> &cols = grid[static_cast<std::size_t>(wy)];
        const int rowLen = static_cast<int>(cols.size());
        for (int wx = 0; wx < rowLen; ++wx)
            cells.push_back(Cell{wx, wy, cols[static_cast<std::size_t>(wx)]});
    }

    // Back-to-front depth sort; stable to keep equal-depth ties in row-major order.
    std::stable_sort(cells.begin(), cells.end(),
                     [](const Cell &a, const Cell &b) { return isoDepthCompare(a.wx, a.wy, b.wx, b.wy) < 0; });

    for (const Cell &cell : cells) {
        const auto it =
            std::find_if(tiles.begin(), tiles.end(), [&](const IsoTile &t) { return t.index == cell.tileIdx; });
        if (it == tiles.end())
            continue;
        const Point screen = worldToScreen(config, cell.wx, cell.wy);
        stamp(out, screen.x + originOffsetX, screen.y + originOffsetY, it->canvas);
    }
    return out;
}

} // namespace tessera
