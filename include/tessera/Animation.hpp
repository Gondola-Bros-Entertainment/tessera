// tessera - Animation: frame sequencing and cross-fade blending.
#pragma once

#include "Canvas.hpp"

namespace tessera {

// How an animation repeats.
enum class LoopMode {
    Loop,     // loop forever (restart from frame 0)
    Once,     // play once and stop on the last frame
    PingPong, // bounce: 0,1,2,1,0,1,2,...
};

// An animation description.
struct Animation {
    int frameDelay{};                  // ticks per frame (higher = slower)
    int frameCount{};                  // total frame count
    LoopMode loopMode{LoopMode::Loop}; // how to repeat
};

constexpr bool operator==(const Animation &x, const Animation &y) {
    return x.frameDelay == y.frameDelay && x.frameCount == y.frameCount && x.loopMode == y.loopMode;
}
constexpr bool operator!=(const Animation &x, const Animation &y) { return !(x == y); }

// ---- construction ----------------------------------------------------------
Animation animation(int delay, int count, LoopMode mode);
Animation loopAnimation(int delay, int count);
Animation onceAnimation(int delay, int count);
Animation pingPongAnimation(int delay, int count);

// ---- playback --------------------------------------------------------------
int animationFrame(const Animation &a, int tick); // current frame index for a tick
bool animationDone(const Animation &a, int tick); // true once a Once anim has finished

// ---- blending --------------------------------------------------------------
// Cross-fade from a (at t=0) to b (at t=1); t in [0,1]. The output uses a's
// dimensions; out-of-range b pixels are treated as transparent.
Canvas blendFrames(double t, const Canvas &a, const Canvas &b);

} // namespace tessera
