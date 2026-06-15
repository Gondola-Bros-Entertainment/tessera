// tessera - Sheet implementation: shelf bin packing of canvases into an atlas.
#include "tessera/Sheet.hpp"

#include "tessera/Compose.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace tessera {
namespace {

using Item = std::pair<std::string, Canvas>;

constexpr int minSheetDimension = 1;  // atlas dimensions clamp to at least this

// Compute a reasonable atlas width: max of the square-root-of-area side and the
// widest single canvas, so the widest sprite always fits on a shelf.
int computeMaxWidth(const std::vector<Item>& items) {
    if (items.empty()) return minSheetDimension;
    long long totalArea = 0;
    int widest = 0;
    for (const Item& it : items) {
        totalArea += static_cast<long long>(it.second.w) * it.second.h;
        widest = std::max(widest, it.second.w);
    }
    const int side = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(totalArea))));
    return std::max(side, widest);
}

// Shelf-pack the (already height-sorted) items, returning entries plus the
// total atlas dimensions.
struct PackResult {
    std::vector<SheetEntry> entries;
    int totalW{};
    int totalH{};
};

PackResult shelfPack(int padding, const std::vector<Item>& items) {
    const int maxShelfWidth = computeMaxWidth(items);
    std::vector<SheetEntry> acc;
    int shelfX = 0, shelfY = 0, totalW = 0, totalH = 0;
    std::size_t i = 0;
    while (i < items.size()) {
        const int w = items[i].second.w;
        const int h = items[i].second.h;
        if (shelfX + w > maxShelfWidth && shelfX > 0) {
            // Start a new shelf and retry the same item.
            const int newShelfY = totalH + padding;
            shelfX = 0;
            shelfY = newShelfY;
            totalH = newShelfY;
        } else {
            acc.push_back(SheetEntry{ items[i].first, shelfX, shelfY, w, h });
            totalW = std::max(totalW, shelfX + w);  // right edge of the placed item
            totalH = std::max(totalH, shelfY + h);
            shelfX = shelfX + w + padding;
            ++i;
        }
    }
    return PackResult{ std::move(acc), std::max(totalW, minSheetDimension), std::max(totalH, minSheetDimension) };
}

// First canvas in items matching name, or a 1x1 transparent fallback.
Canvas findByName(const std::string& name, const std::vector<Item>& items) {
    for (const Item& it : items)
        if (it.first == name) return it.second;
    return newCanvas(1, 1, transparent);
}

} // namespace

SpriteSheet packSheet(int padding, const std::vector<Item>& items) {
    std::vector<Item> sorted = items;
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const Item& a, const Item& b) { return a.second.h > b.second.h; });
    PackResult packed = shelfPack(padding, sorted);
    Canvas atlas = newCanvas(packed.totalW, packed.totalH, transparent);
    for (const SheetEntry& e : packed.entries)
        stamp(atlas, e.x, e.y, findByName(e.name, items));
    return SpriteSheet{ std::move(atlas), std::move(packed.entries) };
}

} // namespace tessera
