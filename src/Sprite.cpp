// tessera - Sprite implementation.
#include "tessera/Sprite.hpp"

#include "tessera/Transform.hpp"

namespace tessera {
namespace {

// Crop a canvas to the bounding box of its non-transparent pixels.
// Returns a 1x1 transparent canvas if every pixel is transparent.
Canvas trimTransparent(const Canvas &c) {
    int minX = c.w, minY = c.h, maxX = -1, maxY = -1;
    for (int y = 0; y < c.h; ++y)
        for (int x = 0; x < c.w; ++x)
            if (getPixel(c, x, y).a != 0) {
                if (x < minX)
                    minX = x;
                if (y < minY)
                    minY = y;
                if (x > maxX)
                    maxX = x;
                if (y > maxY)
                    maxY = y;
            }
    if (maxX < 0 || maxY < 0)
        return newCanvas(1, 1, transparent);
    return crop(c, minX, minY, maxX - minX + 1, maxY - minY + 1);
}

} // namespace

Sprite singleFrame(const std::string &name, const Canvas &canvas) { return Sprite{name, 0, 0, {canvas}, std::nullopt}; }

Sprite multiFrame(const std::string &name, int ox, int oy, const std::vector<Canvas> &frames) {
    return Sprite{name, ox, oy, frames, std::nullopt};
}

int spriteWidth(const Sprite &s) { return s.frames.empty() ? 0 : s.frames.front().w; }

int spriteHeight(const Sprite &s) { return s.frames.empty() ? 0 : s.frames.front().h; }

int frameCount(const Sprite &s) { return static_cast<int>(s.frames.size()); }

std::optional<Canvas> getFrame(const Sprite &s, int idx) {
    if (idx < 0 || idx >= frameCount(s))
        return std::nullopt;
    return s.frames[static_cast<std::size_t>(idx)];
}

Sprite mirrorSprite(const Sprite &sprite) {
    Sprite out = sprite;
    for (Canvas &frame : out.frames)
        frame = flipH(frame);
    return out;
}

Sprite trimSprite(const Sprite &sprite) {
    Sprite out = sprite;
    for (Canvas &frame : out.frames)
        frame = trimTransparent(frame);
    return out;
}

} // namespace tessera
