// tessera - Gradient implementation.
#include "tessera/Gradient.hpp"

#include <algorithm>
#include <cmath>

namespace tessera {

Canvas linearGradient(int w, int h, Color start, Color end, bool horizontal) {
    return generate(w, h, [&](int x, int y) {
        double t;
        if (horizontal) t = w <= 1 ? 0.0 : static_cast<double>(x) / (w - 1);
        else            t = h <= 1 ? 0.0 : static_cast<double>(y) / (h - 1);
        return lerp(t, start, end);
    });
}

Canvas radialGradient(int w, int h, int cx, int cy, int radius, Color inner, Color outer) {
    const double radiusF = std::max(1, radius);
    return generate(w, h, [&](int x, int y) {
        const double dx = x - cx, dy = y - cy;
        const double dist = std::sqrt(dx * dx + dy * dy);
        return lerp(std::min(1.0, dist / radiusF), inner, outer);
    });
}

Canvas diagonalGradient(int w, int h, Color start, Color end) {
    const double maxDist = std::max(1, (w - 1) + (h - 1));
    return generate(w, h, [&](int x, int y) {
        return lerp(static_cast<double>(x + y) / maxDist, start, end);
    });
}

} // namespace tessera
