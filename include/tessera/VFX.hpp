// tessera - procedural visual-effect generators producing animation frames.
// Each generator returns a sequence of Canvas frames for an animated effect.
// Output is deterministic: all variation comes from seeded LCG helpers.
#pragma once

#include "Canvas.hpp"

#include <vector>

namespace tessera {

// Configuration for explosion effects.
struct ExplosionConfig {
    int size{};          // size of each frame (square)
    int frameCount{};    // number of frames
    int particleCount{}; // number of particles
    Color color{};       // particle color
    int seed{};          // random seed
};

// Configuration for expanding ring effects.
struct RingConfig {
    int size{};       // size of each frame (square)
    int frameCount{}; // number of frames
    Color color{};    // ring color
    int thickness{};  // ring thickness
};

// Configuration for glow/pulse effects.
struct GlowConfig {
    int size{};       // size of each frame (square)
    int frameCount{}; // number of frames per cycle
    Color color{};    // glow color
};

// Configuration for trail effects.
struct TrailConfig {
    int width{};      // width of each frame
    int height{};     // height of each frame
    int frameCount{}; // number of frames
    Color color{};    // trail color
    int dotRadius{};  // trail dot radius
};

// Configuration for directional spark burst effects.
struct SparksConfig {
    int size{};       // size of each frame (square)
    int frameCount{}; // number of frames
    int count{};      // number of sparks
    Color color{};    // spark color
    int seed{};       // random seed
};

// Generators: each returns a list of frames, one fresh Canvas per frame.
std::vector<Canvas> explosionFrames(const ExplosionConfig &config); // radial particle burst
std::vector<Canvas> ringExpandFrames(const RingConfig &config);     // expanding ring
std::vector<Canvas> glowPulseFrames(const GlowConfig &config);      // sine-driven pulse
std::vector<Canvas> trailFrames(const TrailConfig &config);         // dots fading behind
std::vector<Canvas> flashFrames(int count, Color color);            // solid color fading out
std::vector<Canvas> sparksFrames(const SparksConfig &config);       // directional spark burst

} // namespace tessera
