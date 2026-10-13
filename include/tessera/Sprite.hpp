// tessera - Sprite: a named multi-frame image with origin and bounding box.
#pragma once

#include "Canvas.hpp"

#include <optional>
#include <string>
#include <vector>

namespace tessera {

// A bounding box for hit detection.
struct BoundingBox {
    int x{};
    int y{};
    int width{};
    int height{};
};

constexpr bool operator==(const BoundingBox &x, const BoundingBox &y) {
    return x.x == y.x && x.y == y.y && x.width == y.width && x.height == y.height;
}
constexpr bool operator!=(const BoundingBox &x, const BoundingBox &y) { return !(x == y); }

// A named sprite with one or more frames and an origin point.
struct Sprite {
    std::string name;                  // human-readable name
    int originX{};                     // origin X offset (for positioning)
    int originY{};                     // origin Y offset (for positioning)
    std::vector<Canvas> frames;        // animation frames (at least one)
    std::optional<BoundingBox> bounds; // optional hit region
};

// ---- construction ----------------------------------------------------------
Sprite singleFrame(const std::string &name, const Canvas &canvas); // origin (0,0)
Sprite multiFrame(const std::string &name, int ox, int oy, const std::vector<Canvas> &frames);

// ---- queries ---------------------------------------------------------------
int spriteWidth(const Sprite &s);  // width of the first frame (0 if none)
int spriteHeight(const Sprite &s); // height of the first frame (0 if none)
int frameCount(const Sprite &s);   // number of animation frames

// Frame at index, or nullopt if the index is out of range.
std::optional<Canvas> getFrame(const Sprite &s, int idx);

// ---- transforms ------------------------------------------------------------
Sprite mirrorSprite(const Sprite &sprite); // flip every frame horizontally
Sprite trimSprite(const Sprite &sprite);   // trim transparent borders per frame

} // namespace tessera
