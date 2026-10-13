// tessera - Sheet: pack multiple canvases into a single atlas image.
#pragma once

#include "Canvas.hpp"

#include <string>
#include <utility>
#include <vector>

namespace tessera {

// Metadata for a sprite placed in the sheet.
struct SheetEntry {
    std::string name; // original sprite name
    int x{};          // X position in the atlas
    int y{};          // Y position in the atlas
    int width{};      // width
    int height{};     // height
};

constexpr bool operator==(const SheetEntry &x, const SheetEntry &y) {
    return x.x == y.x && x.y == y.y && x.width == y.width && x.height == y.height && x.name == y.name;
}
constexpr bool operator!=(const SheetEntry &x, const SheetEntry &y) { return !(x == y); }

// A packed sprite sheet: atlas image plus placement metadata.
struct SpriteSheet {
    Canvas canvas;                   // the combined atlas canvas
    std::vector<SheetEntry> entries; // placement metadata for each sprite
};

// ---- packing ---------------------------------------------------------------
// Pack named canvases into a sprite sheet atlas. padding adds space between
// sprites. Items are sorted by height descending before placement.
SpriteSheet packSheet(int padding, const std::vector<std::pair<std::string, Canvas>> &items);

} // namespace tessera
