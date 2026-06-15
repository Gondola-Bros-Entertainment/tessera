// tessera - Palette implementation.
// Median cut splits the widest color bucket along its widest channel axis until
// the target count is reached; quantization picks the nearest color by squared
// RGB distance. Sorts use stable_sort so ties preserve input order.
#include "tessera/Palette.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace tessera {
namespace {

constexpr std::uint8_t maxAlpha       = 255;
constexpr std::uint8_t minOpaqueAlpha = 1;
constexpr int          channelMax     = 255;
constexpr int          medianDivisor  = 2;

using Bucket = std::vector<Color>;

// Squared Euclidean distance between two colors in RGB space (alpha ignored).
int colorDist(Color a, Color b) {
    const int dr = static_cast<int>(a.r) - static_cast<int>(b.r);
    const int dg = static_cast<int>(a.g) - static_cast<int>(b.g);
    const int db = static_cast<int>(a.b) - static_cast<int>(b.b);
    return dr * dr + dg * dg + db * db;
}

// Largest spread across the R, G, B channels of a bucket. Empty -> 0.
int bucketRange(const Bucket& colors) {
    if (colors.empty()) return 0;
    int minR = colors[0].r, maxR = colors[0].r;
    int minG = colors[0].g, maxG = colors[0].g;
    int minB = colors[0].b, maxB = colors[0].b;
    for (const Color c : colors) {
        minR = std::min(minR, static_cast<int>(c.r));
        maxR = std::max(maxR, static_cast<int>(c.r));
        minG = std::min(minG, static_cast<int>(c.g));
        maxG = std::max(maxG, static_cast<int>(c.g));
        minB = std::min(minB, static_cast<int>(c.b));
        maxB = std::max(maxB, static_cast<int>(c.b));
    }
    return std::max(maxR - minR, std::max(maxG - minG, maxB - minB));
}

// Split a bucket in two: sort along the widest channel, cut at the midpoint.
std::pair<Bucket, Bucket> splitBucket(const Bucket& colors) {
    if (colors.empty()) return { Bucket{}, Bucket{} };
    int minR = colors[0].r, maxR = colors[0].r;
    int minG = colors[0].g, maxG = colors[0].g;
    int minB = colors[0].b, maxB = colors[0].b;
    for (const Color c : colors) {
        minR = std::min(minR, static_cast<int>(c.r));
        maxR = std::max(maxR, static_cast<int>(c.r));
        minG = std::min(minG, static_cast<int>(c.g));
        maxG = std::max(maxG, static_cast<int>(c.g));
        minB = std::min(minB, static_cast<int>(c.b));
        maxB = std::max(maxB, static_cast<int>(c.b));
    }
    const int rangeR = maxR - minR;
    const int rangeG = maxG - minG;
    const int rangeB = maxB - minB;

    Bucket sorted = colors;
    if (rangeR >= rangeG && rangeR >= rangeB)
        std::stable_sort(sorted.begin(), sorted.end(),
                         [](Color a, Color b) { return a.r < b.r; });
    else if (rangeG >= rangeB)
        std::stable_sort(sorted.begin(), sorted.end(),
                         [](Color a, Color b) { return a.g < b.g; });
    else
        std::stable_sort(sorted.begin(), sorted.end(),
                         [](Color a, Color b) { return a.b < b.b; });

    const std::size_t mid = sorted.size() / medianDivisor;
    return { Bucket(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(mid)),
             Bucket(sorted.begin() + static_cast<std::ptrdiff_t>(mid), sorted.end()) };
}

// Select the bucket with the largest range; the rest follow in descending-range
// order, ties preserving input order.
bool findWidestBucket(const std::vector<Bucket>& buckets, Bucket& chosen,
                      std::vector<Bucket>& rest) {
    if (buckets.empty()) return false;
    std::vector<std::pair<int, Bucket>> scored;
    scored.reserve(buckets.size());
    for (const Bucket& b : buckets) scored.emplace_back(bucketRange(b), b);
    std::stable_sort(scored.begin(), scored.end(),
                     [](const auto& a, const auto& b) { return a.first > b.first; });
    chosen = std::move(scored.front().second);
    rest.clear();
    rest.reserve(scored.size() - 1);
    for (std::size_t i = 1; i < scored.size(); ++i) rest.push_back(std::move(scored[i].second));
    return true;
}

// Repeatedly split buckets until reaching targetCount. New halves are prepended
// ahead of the remaining buckets so output ordering is preserved.
std::vector<Bucket> medianCut(int targetCount, std::vector<Bucket> buckets) {
    while (static_cast<int>(buckets.size()) < targetCount) {
        Bucket chosen;
        std::vector<Bucket> rest;
        if (!findWidestBucket(buckets, chosen, rest)) break;
        auto [left, right] = splitBucket(chosen);
        // The widest bucket only fails to split into two non-empty halves when
        // it (hence every bucket) is a singleton: no further progress possible.
        if (left.empty() || right.empty()) break;
        std::vector<Bucket> next;
        next.reserve(rest.size() + 2);
        next.push_back(std::move(left));
        next.push_back(std::move(right));
        for (Bucket& b : rest) next.push_back(std::move(b));
        buckets = std::move(next);
    }
    return buckets;
}

// Average a bucket's RGB; alpha forced opaque. Empty -> transparent.
Color bucketAverage(const Bucket& colors) {
    if (colors.empty()) return transparent;
    const int n = static_cast<int>(colors.size());
    int totalR = 0, totalG = 0, totalB = 0;
    for (const Color c : colors) {
        totalR += c.r;
        totalG += c.g;
        totalB += c.b;
    }
    return { static_cast<std::uint8_t>(totalR / n), static_cast<std::uint8_t>(totalG / n),
             static_cast<std::uint8_t>(totalB / n), static_cast<std::uint8_t>(channelMax) };
}

// Nearest palette color by squared RGB distance; ties keep the earlier color.
Color findNearest(const Palette& pal, Color target) {
    Color best = pal.front();
    int bestDist = colorDist(target, best);
    for (std::size_t i = 1; i < pal.size(); ++i) {
        const int d = colorDist(target, pal[i]);
        if (d < bestDist) {
            bestDist = d;
            best = pal[i];
        }
    }
    return best;
}

} // namespace

Palette grayscale4() {
    return { { 0, 0, 0, maxAlpha },
             { 85, 85, 85, maxAlpha },
             { 170, 170, 170, maxAlpha },
             { 255, 255, 255, maxAlpha } };
}

Palette grayscale8() {
    return { { 0, 0, 0, maxAlpha },
             { 36, 36, 36, maxAlpha },
             { 73, 73, 73, maxAlpha },
             { 109, 109, 109, maxAlpha },
             { 146, 146, 146, maxAlpha },
             { 182, 182, 182, maxAlpha },
             { 219, 219, 219, maxAlpha },
             { 255, 255, 255, maxAlpha } };
}

Palette gameboy() {
    return { { 15, 56, 15, maxAlpha },    // darkest
             { 48, 98, 48, maxAlpha },
             { 139, 172, 15, maxAlpha },
             { 155, 188, 15, maxAlpha } }; // lightest
}

Palette nes() {
    return { { 0, 0, 0, maxAlpha },       // black
             { 252, 252, 252, maxAlpha }, // white
             { 188, 0, 0, maxAlpha },     // red
             { 0, 120, 0, maxAlpha },     // green
             { 0, 0, 168, maxAlpha },     // blue
             { 248, 184, 0, maxAlpha },   // yellow
             { 0, 168, 168, maxAlpha },   // cyan
             { 148, 0, 132, maxAlpha },   // magenta
             { 228, 92, 16, maxAlpha },   // orange
             { 124, 8, 76, maxAlpha },    // purple
             { 252, 160, 68, maxAlpha },  // peach
             { 168, 204, 0, maxAlpha },   // lime
             { 88, 216, 84, maxAlpha },   // bright green
             { 104, 136, 252, maxAlpha }, // light blue
             { 164, 164, 164, maxAlpha }, // gray
             { 216, 120, 168, maxAlpha } }; // pink
}

Palette fromColors(std::vector<Color> colors) { return colors; }

int paletteSize(const Palette& pal) { return static_cast<int>(pal.size()); }

Color paletteColor(const Palette& pal, int idx) {
    if (pal.empty()) return transparent;
    if (idx < 0) return pal.front();
    if (idx >= static_cast<int>(pal.size())) return pal.back();
    return pal[static_cast<std::size_t>(idx)];
}

Color paletteSwap(const Palette& src, const Palette& dst, Color color) {
    const std::size_t n = std::min(src.size(), dst.size());
    for (std::size_t i = 0; i < n; ++i)
        if (color == src[i]) return dst[i];
    return color;
}

Palette paletteLerp(double t, const Palette& a, const Palette& b) {
    const std::size_t n = std::max(a.size(), b.size());
    Palette out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        const Color ca = i < a.size() ? a[i] : transparent;
        const Color cb = i < b.size() ? b[i] : transparent;
        out.push_back(lerp(t, ca, cb));
    }
    return out;
}

Palette extractPalette(int targetCount, const std::vector<Color>& colors) {
    if (targetCount <= 0) return Palette{};
    Bucket opaque;
    opaque.reserve(colors.size());
    for (const Color c : colors)
        if (c.a >= minOpaqueAlpha) opaque.push_back(c);

    std::vector<Bucket> initial;
    if (!opaque.empty()) initial.push_back(std::move(opaque));

    const std::vector<Bucket> buckets = medianCut(targetCount, std::move(initial));
    Palette result;
    result.reserve(buckets.size());
    for (const Bucket& b : buckets) result.push_back(bucketAverage(b));
    return result;
}

Color quantizeColor(const Palette& pal, Color c) {
    if (pal.empty()) return c;
    return findNearest(pal, c);
}

} // namespace tessera
