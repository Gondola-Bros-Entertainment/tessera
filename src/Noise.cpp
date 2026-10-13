// tessera - Noise implementation: value/Perlin samplers and fBm/turbulence.
// Lattice hashing runs in unsigned 64-bit arithmetic, which wraps by definition,
// and keeps the low 31 bits of each LCG step before normalizing.
#include "tessera/Noise.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tessera {
namespace {

constexpr std::uint64_t lcgMultiplier = 1103515245u;
constexpr std::uint64_t lcgIncrement = 12345u;
constexpr std::uint64_t lcgBitmask = 2147483647u; // 2^31 - 1
constexpr double lcgMaxF = 2147483647.0;
constexpr std::uint64_t latticeHashX = 374761393u;
constexpr std::uint64_t latticeHashY = 668265263u;
constexpr std::int64_t octaveSeed = 1337;
constexpr double persistence = 0.5;
constexpr double lacunarity = 2.0;
constexpr double invSqrt2 = 0.7071067811865476;
constexpr std::uint64_t gradientMask = 7u; // select one of 8 gradient vectors
constexpr double channelMaxF = 255.0;      // [0,1] sample -> 8-bit channel

std::uint64_t lcgHash(std::uint64_t s) {
    const auto step = [](std::uint64_t v) { return (v * lcgMultiplier + lcgIncrement) & lcgBitmask; };
    return step(step(step(s)));
}

// Negative seeds and coordinates convert modulo 2^64, which keeps their two's-complement bits.
std::uint64_t latticeHash(std::int64_t seed, int x, int y) {
    return lcgHash(static_cast<std::uint64_t>(seed) + static_cast<std::uint64_t>(x) * latticeHashX +
                   static_cast<std::uint64_t>(y) * latticeHashY);
}

double latticeValue(std::int64_t seed, int x, int y) { return static_cast<double>(latticeHash(seed, x, y)) / lcgMaxF; }

double smoothstep(double t) { return t * t * (3.0 - 2.0 * t); }
double lerpD(double t, double a, double b) { return a + t * (b - a); }

double totalAmplitude(int octaves) {
    return (1.0 - std::pow(persistence, static_cast<double>(octaves))) / (1.0 - persistence);
}

struct Vec2d {
    double x, y;
};
Vec2d gradientVector(std::uint64_t hash) {
    switch (hash & gradientMask) {
    case 0:
        return {1.0, 0.0};
    case 1:
        return {invSqrt2, invSqrt2};
    case 2:
        return {0.0, 1.0};
    case 3:
        return {-invSqrt2, invSqrt2};
    case 4:
        return {-1.0, 0.0};
    case 5:
        return {-invSqrt2, -invSqrt2};
    case 6:
        return {0.0, -1.0};
    default:
        return {invSqrt2, -invSqrt2};
    }
}

double gradDot(std::int64_t seed, int gx, int gy, double dx, double dy) {
    const Vec2d grad = gradientVector(latticeHash(seed, gx, gy));
    return grad.x * dx + grad.y * dy;
}

} // namespace

double valueSample(int seed, double scale, int x, int y) {
    const double fx = x / std::max(1.0, scale);
    const double fy = y / std::max(1.0, scale);
    const int ix = static_cast<int>(std::floor(fx));
    const int iy = static_cast<int>(std::floor(fy));
    const double smoothX = smoothstep(fx - ix);
    const double smoothY = smoothstep(fy - iy);
    const std::int64_t s = seed;
    const double v00 = latticeValue(s, ix, iy);
    const double v10 = latticeValue(s, ix + 1, iy);
    const double v01 = latticeValue(s, ix, iy + 1);
    const double v11 = latticeValue(s, ix + 1, iy + 1);
    return lerpD(smoothY, lerpD(smoothX, v00, v10), lerpD(smoothX, v01, v11));
}

double perlinSample(int seed, double scale, int x, int y) {
    const double fx = x / std::max(1.0, scale);
    const double fy = y / std::max(1.0, scale);
    const int ix = static_cast<int>(std::floor(fx));
    const int iy = static_cast<int>(std::floor(fy));
    const double fracX = fx - ix, fracY = fy - iy;
    const double smoothX = smoothstep(fracX), smoothY = smoothstep(fracY);
    const std::int64_t s = seed;
    const double g00 = gradDot(s, ix, iy, fracX, fracY);
    const double g10 = gradDot(s, ix + 1, iy, fracX - 1.0, fracY);
    const double g01 = gradDot(s, ix, iy + 1, fracX, fracY - 1.0);
    const double g11 = gradDot(s, ix + 1, iy + 1, fracX - 1.0, fracY - 1.0);
    const double raw = lerpD(smoothY, lerpD(smoothX, g00, g10), lerpD(smoothX, g01, g11));
    return (raw + 1.0) / 2.0;
}

double fbmSample(int seed, int octaves, double scale, int x, int y) {
    const int oct = std::max(1, octaves);
    double acc = 0.0, amp = 1.0, freq = 1.0;
    for (int o = 0; o < oct; ++o) {
        acc += valueSample(static_cast<int>(seed + o * octaveSeed), scale / freq, x, y) * amp;
        amp *= persistence;
        freq *= lacunarity;
    }
    return acc / totalAmplitude(oct);
}

double turbulenceSample(int seed, int octaves, double scale, int x, int y) {
    const int oct = std::max(1, octaves);
    double acc = 0.0, amp = 1.0, freq = 1.0;
    for (int o = 0; o < oct; ++o) {
        const double val = perlinSample(static_cast<int>(seed + o * octaveSeed), scale / freq, x, y);
        acc += std::fabs(val * 2.0 - 1.0) * amp;
        amp *= persistence;
        freq *= lacunarity;
    }
    return acc / totalAmplitude(oct);
}

Canvas valueNoiseColor(int w, int h, int seed, double scale, Color a, Color b) {
    return generate(w, h, [&](int x, int y) { return lerp(valueSample(seed, scale, x, y), a, b); });
}

Canvas valueNoise(int w, int h, int seed, double scale) { return valueNoiseColor(w, h, seed, scale, black, white); }

Canvas perlinNoise(int w, int h, int seed, double scale) {
    return generate(w, h, [&](int x, int y) {
        const std::uint8_t g = static_cast<std::uint8_t>(std::lround(perlinSample(seed, scale, x, y) * channelMaxF));
        return Color{g, g, g, 255};
    });
}

Canvas fbm(int w, int h, int seed, int octaves, double scale) {
    return generate(w, h, [&](int x, int y) {
        const std::uint8_t g =
            static_cast<std::uint8_t>(std::lround(fbmSample(seed, octaves, scale, x, y) * channelMaxF));
        return Color{g, g, g, 255};
    });
}

Canvas turbulence(int w, int h, int seed, int octaves, double scale) {
    return generate(w, h, [&](int x, int y) {
        const std::uint8_t g =
            static_cast<std::uint8_t>(std::lround(turbulenceSample(seed, octaves, scale, x, y) * channelMaxF));
        return Color{g, g, g, 255};
    });
}

} // namespace tessera
