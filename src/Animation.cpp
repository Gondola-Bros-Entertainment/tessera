// tessera - Animation implementation.
#include "tessera/Animation.hpp"

#include "tessera/Color.hpp"

namespace tessera {
namespace {

constexpr int pingPongCycleStep = 2; // a full bounce visits count*2-2 frames

// Floor division and modulo (floor toward negative infinity, not the C++
// truncate-toward-zero default), so frame indexing is correct for negative ticks.
int floorDiv(int a, int b) {
    const int q = a / b;
    return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q;
}
int floorMod(int a, int b) {
    const int r = a % b;
    return (r != 0 && ((r < 0) != (b < 0))) ? r + b : r;
}

} // namespace

Animation animation(int delay, int count, LoopMode mode) { return Animation{delay, count, mode}; }

Animation loopAnimation(int delay, int count) { return Animation{delay, count, LoopMode::Loop}; }

Animation onceAnimation(int delay, int count) { return Animation{delay, count, LoopMode::Once}; }

Animation pingPongAnimation(int delay, int count) { return Animation{delay, count, LoopMode::PingPong}; }

int animationFrame(const Animation &a, int tick) {
    if (a.frameCount <= 0)
        return 0;
    if (a.frameDelay <= 0)
        return 0;
    const int rawFrame = floorDiv(tick, a.frameDelay);
    switch (a.loopMode) {
    case LoopMode::Loop:
        return floorMod(rawFrame, a.frameCount);
    case LoopMode::Once:
        return rawFrame < a.frameCount - 1 ? rawFrame : a.frameCount - 1;
    case LoopMode::PingPong: {
        const int cycleLen = a.frameCount * pingPongCycleStep - pingPongCycleStep;
        const int safeCycle = cycleLen < 1 ? 1 : cycleLen;
        const int pos = floorMod(rawFrame, safeCycle);
        return pos < a.frameCount ? pos : safeCycle - pos;
    }
    }
    return 0; // unreachable; satisfies -Wreturn-type
}

bool animationDone(const Animation &a, int tick) {
    if (a.loopMode != LoopMode::Once)
        return false;
    return a.frameDelay > 0 && a.frameCount > 0 && floorDiv(tick, a.frameDelay) >= a.frameCount - 1;
}

Canvas blendFrames(double t, const Canvas &a, const Canvas &b) {
    // Output uses a's dimensions. getPixel returns transparent out of bounds,
    // so off-canvas b pixels are treated as fully transparent.
    return generate(a.w, a.h, [&](int x, int y) { return lerp(t, getPixel(a, x, y), getPixel(b, x, y)); });
}

} // namespace tessera
