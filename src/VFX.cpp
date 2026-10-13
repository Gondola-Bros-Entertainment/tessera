// tessera - VFX implementation.
// Particles and sparks scatter on the golden angle with seeded LCG jitter;
// the ring uses a midpoint-circle outline; effects fade with frame progress.
#include "tessera/VFX.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tessera {
namespace {

constexpr double pi = 3.14159265358979323846;

// ---- deterministic pseudo-random helpers -----------------------------------

// Golden angle for even angular distribution.
constexpr double goldenAngle = 2.399963;

// LCG step for seed variation.
constexpr int lcgStep = 7;

// LCG constants. Arithmetic is unsigned 64-bit, which wraps by definition, then
// masked to 31 bits before normalizing.
constexpr std::uint64_t lcgMul = 1103515245u;
constexpr std::uint64_t lcgInc = 12345u;
constexpr double lcgMod = 2147483648.0;        // 2^31
constexpr std::uint64_t lcgMask = 2147483647u; // 2^31 - 1

// Deterministic fractional value in [0, 1) from a seed. A negative seed converts
// modulo 2^64, which keeps its two's-complement bits.
double lcgFrac(std::int64_t seed) {
    const std::uint64_t hashed = (static_cast<std::uint64_t>(seed) * lcgMul + lcgInc) & lcgMask;
    return static_cast<double>(hashed) / lcgMod;
}

// Round a double to the nearest int (lround: half away from zero).
int iround(double v) { return static_cast<int>(std::lround(v)); }

// ---- ring outline ----------------------------------------------------------

// Plot the eight-way symmetric pixels of a midpoint circle of the given radius.
// Skips non-positive radii.
void circleOutline(Canvas &canvas, int cx, int cy, int radius, Color color) {
    if (radius <= 0)
        return;
    int x = 0, y = radius, d = 1 - radius;
    while (x <= y) {
        setPixel(canvas, cx + x, cy + y, color);
        setPixel(canvas, cx - x, cy + y, color);
        setPixel(canvas, cx + x, cy - y, color);
        setPixel(canvas, cx - x, cy - y, color);
        setPixel(canvas, cx + y, cy + x, color);
        setPixel(canvas, cx - y, cy + x, color);
        setPixel(canvas, cx + y, cy - x, color);
        setPixel(canvas, cx - y, cy - x, color);
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            --y;
        }
        ++x;
    }
}

// Draw a ring of the given thickness as a band of concentric outlines.
void drawRing(Canvas &canvas, int cx, int cy, int radius, int thickness, Color color) {
    // Thickness is split evenly inside and outside the nominal radius.
    constexpr int halfThicknessDivisor = 2;
    const int lo = std::max(0, radius - thickness / halfThicknessDivisor);
    const int hi = radius + thickness / halfThicknessDivisor;
    for (int r = lo; r <= hi; ++r)
        circleOutline(canvas, cx, cy, r, color);
}

// ---- per-frame renderers ---------------------------------------------------

constexpr int particleRadiusDivisor = 16;
constexpr int minParticleRadius = 1;
// Per-particle travel distance spans [distBase, distBase + distJitter] of full radius.
constexpr double distBase = 0.5;
constexpr double distJitter = 0.5;
// Particle radius shrinks by this fraction of itself over the effect's lifetime.
constexpr double particleShrink = 0.5;

Canvas renderExplosionFrame(const ExplosionConfig &config, int frameIdx) {
    const int size = config.size;
    const int center = size / 2;
    const double progress = static_cast<double>(frameIdx) / std::max(1, config.frameCount - 1);
    const double maxRadius = static_cast<double>(center);
    Canvas canvas = newCanvas(size, size, transparent);
    for (int particleIdx = 0; particleIdx < config.particleCount; ++particleIdx) {
        const std::int64_t angleSeed = static_cast<std::int64_t>(particleIdx) * lcgStep + config.seed;
        const double angle = static_cast<double>(angleSeed) * goldenAngle;
        const double dist = progress * maxRadius *
                            (distBase + distJitter * lcgFrac(static_cast<std::int64_t>(particleIdx) + config.seed));
        const int px = center + iround(std::cos(angle) * dist);
        const int py = center + iround(std::sin(angle) * dist);
        const double alpha = std::max(0.0, 1.0 - progress);
        const Color particleColor = scaleAlpha(alpha, config.color);
        const int baseRadius = size / particleRadiusDivisor;
        const int particleRadius = std::max(minParticleRadius, iround(baseRadius * (1.0 - progress * particleShrink)));
        fillCircle(canvas, px, py, particleRadius, particleColor);
    }
    return canvas;
}

Canvas renderRingFrame(const RingConfig &config, int frameIdx) {
    const int size = config.size;
    const int center = size / 2;
    const double progress = static_cast<double>(frameIdx) / std::max(1, config.frameCount - 1);
    const double maxRadius = static_cast<double>(center);
    const int radius = iround(progress * maxRadius);
    const double alpha = std::max(0.0, 1.0 - progress);
    const Color ringCol = scaleAlpha(alpha, config.color);
    Canvas canvas = newCanvas(size, size, transparent);
    drawRing(canvas, center, center, radius, config.thickness, ringCol);
    return canvas;
}

// Glow radius spans [glowRadiusBase, glowRadiusBase + glowRadiusRange] of center.
constexpr double fullTurn = 2.0;  // multiplier on pi for one full sine cycle
constexpr double halfRange = 2.0; // maps sine output [-1, 1] into [0, 1]
constexpr double glowRadiusBase = 0.3;
constexpr double glowRadiusRange = 0.7;

Canvas renderGlowFrame(const GlowConfig &config, int frameIdx) {
    const int size = config.size;
    const int center = size / 2;
    // A pulse is a seamless loop, so the last frame must not duplicate frame 0;
    // divide by frameCount (not frameCount - 1, which one-shot effects use).
    const double cyclePos = static_cast<double>(frameIdx) / std::max(1, config.frameCount);
    const double intensity = (std::sin(cyclePos * fullTurn * pi) + 1.0) / halfRange;
    const int radius = iround(static_cast<double>(center) * (glowRadiusBase + intensity * glowRadiusRange));
    const Color glowCol = scaleAlpha(intensity, config.color);
    Canvas canvas = newCanvas(size, size, transparent);
    fillCircle(canvas, center, center, radius, glowCol);
    return canvas;
}

constexpr int trailDotCount = 5;
constexpr double trailOffsetFrac = 0.1;

Canvas renderTrailFrame(const TrailConfig &config, int frameIdx) {
    const int w = config.width;
    const int h = config.height;
    const double progress = static_cast<double>(frameIdx) / std::max(1, config.frameCount - 1);
    const int dotX = iround(progress * static_cast<double>(w - 1));
    const int dotY = h / 2;
    Canvas canvas = newCanvas(w, h, transparent);
    for (int dotIdx = 0; dotIdx < trailDotCount; ++dotIdx) {
        const double offset = static_cast<double>(dotIdx) * (static_cast<double>(w) * trailOffsetFrac);
        const int dx = dotX - iround(offset);
        const double alpha = std::max(0.0, 1.0 - static_cast<double>(dotIdx) / trailDotCount);
        const Color dotColor = scaleAlpha(alpha, config.color);
        if (dx >= 0)
            fillCircle(canvas, dx, dotY, config.dotRadius, dotColor);
    }
    return canvas;
}

constexpr int sparkRadius = 1;
constexpr double sparkSpeedBase = 0.5; // spark speed is sparkSpeedBase + lcgFrac jitter
constexpr int sparkSpeedSeedMul = 3;   // decorrelates the speed seed from the angle seed

Canvas renderSparksFrame(const SparksConfig &config, int frameIdx) {
    const int size = config.size;
    const int center = size / 2;
    const double progress = static_cast<double>(frameIdx) / std::max(1, config.frameCount - 1);
    Canvas canvas = newCanvas(size, size, transparent);
    for (int sparkIdx = 0; sparkIdx < config.count; ++sparkIdx) {
        const std::int64_t angleSeed = static_cast<std::int64_t>(sparkIdx) * lcgStep + config.seed;
        const double angle = static_cast<double>(angleSeed) * goldenAngle;
        const double speed = sparkSpeedBase + lcgFrac(static_cast<std::int64_t>(sparkIdx) +
                                                      static_cast<std::int64_t>(config.seed) * sparkSpeedSeedMul);
        const double dist = progress * static_cast<double>(center) * speed;
        const int px = center + iround(std::cos(angle) * dist);
        const int py = center + iround(std::sin(angle) * dist);
        const double alpha = std::max(0.0, 1.0 - progress);
        const Color sparkColor = scaleAlpha(alpha, config.color);
        fillCircle(canvas, px, py, sparkRadius, sparkColor);
    }
    return canvas;
}

} // namespace

std::vector<Canvas> explosionFrames(const ExplosionConfig &config) {
    std::vector<Canvas> frames;
    for (int frameIdx = 0; frameIdx < config.frameCount; ++frameIdx)
        frames.push_back(renderExplosionFrame(config, frameIdx));
    return frames;
}

std::vector<Canvas> ringExpandFrames(const RingConfig &config) {
    std::vector<Canvas> frames;
    for (int frameIdx = 0; frameIdx < config.frameCount; ++frameIdx)
        frames.push_back(renderRingFrame(config, frameIdx));
    return frames;
}

std::vector<Canvas> glowPulseFrames(const GlowConfig &config) {
    std::vector<Canvas> frames;
    for (int frameIdx = 0; frameIdx < config.frameCount; ++frameIdx)
        frames.push_back(renderGlowFrame(config, frameIdx));
    return frames;
}

std::vector<Canvas> trailFrames(const TrailConfig &config) {
    std::vector<Canvas> frames;
    for (int frameIdx = 0; frameIdx < config.frameCount; ++frameIdx)
        frames.push_back(renderTrailFrame(config, frameIdx));
    return frames;
}

std::vector<Canvas> flashFrames(int count, Color color) {
    constexpr int flashSize = 32;
    std::vector<Canvas> frames;
    for (int i = 0; i < count; ++i) {
        const double progress = static_cast<double>(i) / std::max(1, count - 1);
        const Color faded = scaleAlpha(1.0 - progress, color);
        frames.push_back(newCanvas(flashSize, flashSize, faded));
    }
    return frames;
}

std::vector<Canvas> sparksFrames(const SparksConfig &config) {
    std::vector<Canvas> frames;
    for (int frameIdx = 0; frameIdx < config.frameCount; ++frameIdx)
        frames.push_back(renderSparksFrame(config, frameIdx));
    return frames;
}

} // namespace tessera
