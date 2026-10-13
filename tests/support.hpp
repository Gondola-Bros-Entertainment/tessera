#pragma once
#include <tessera/Canvas.hpp>
#include <tessera/Color.hpp>

#include <cstddef>
#include <cstdlib>
#include <functional>

namespace tessera::test {

/// Pixels for which @p wanted holds.
inline int count(const Canvas &c, const std::function<bool(Color)> &wanted) {
    int found = 0;
    for (int y = 0; y < c.h; ++y) {
        for (int x = 0; x < c.w; ++x) {
            found += wanted(getPixel(c, x, y)) ? 1 : 0;
        }
    }
    return found;
}

inline int opaque(const Canvas &c) {
    return count(c, [](Color col) { return col.a != 0; });
}

inline bool same(const Canvas &a, const Canvas &b) { return a.w == b.w && a.h == b.h && a.rgba == b.rgba; }

/// Equal channel by channel within @p tolerance, for results that round.
inline bool near(Color a, Color b, int tolerance = 1) {
    return std::abs(a.r - b.r) <= tolerance && std::abs(a.g - b.g) <= tolerance && std::abs(a.b - b.b) <= tolerance &&
           std::abs(a.a - b.a) <= tolerance;
}

/// A canvas whose every pixel differs, so moves and flips are visible.
inline Canvas numbered(int w, int h) {
    return generate(w, h, [w](int x, int y) {
        const int n = y * w + x;
        return Color{static_cast<std::uint8_t>(n), static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), 255};
    });
}

} // namespace tessera::test
