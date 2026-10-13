#include "support.hpp"

#include <tessera/Noise.hpp>
#include <tessera/VFX.hpp>

#include <doctest/doctest.h>

#include <vector>

using namespace tessera;
using namespace tessera::test;

namespace {

bool same_frames(const std::vector<Canvas> &a, const std::vector<Canvas> &b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t frame = 0; frame < a.size(); ++frame) {
        if (!same(a[frame], b[frame])) {
            return false;
        }
    }
    return true;
}

} // namespace

TEST_CASE("Noise samples stay in [0, 1], repeat for a seed and differ between seeds") {
    bool seeds_differ = false;
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            for (const double value : {valueSample(7, 0.1, x, y), perlinSample(7, 0.1, x, y),
                                       fbmSample(7, 4, 0.1, x, y), turbulenceSample(7, 4, 0.1, x, y)}) {
                REQUIRE(value >= 0.0);
                REQUIRE(value <= 1.0);
            }
            REQUIRE(perlinSample(7, 0.1, x, y) == perlinSample(7, 0.1, x, y));
            seeds_differ = seeds_differ || valueSample(7, 0.1, x, y) != valueSample(8, 0.1, x, y);
        }
    }
    CHECK(seeds_differ);
}

TEST_CASE("Noise canvases are grayscale, or blend between two colors") {
    for (const Canvas &c :
         {valueNoise(12, 8, 3, 0.2), perlinNoise(12, 8, 3, 0.2), fbm(12, 8, 3, 4, 0.2), turbulence(12, 8, 3, 4, 0.2)}) {
        CHECK(c.w == 12);
        CHECK(c.h == 8);
        CHECK(count(c, [](Color col) { return col.r != col.g || col.g != col.b; }) == 0);
    }
    CHECK(same(perlinNoise(12, 8, 3, 0.2), perlinNoise(12, 8, 3, 0.2)));
    const Canvas tinted = valueNoiseColor(12, 8, 3, 0.2, black, red);
    CHECK(count(tinted, [](Color col) { return col.g != 0 || col.b != 0; }) == 0);
}

TEST_CASE("Effects produce the requested frames and repeat for a seed") {
    const ExplosionConfig explosion{.size = 24, .frameCount = 8, .particleCount = 12, .color = red, .seed = 5};
    const auto burst = explosionFrames(explosion);
    REQUIRE(burst.size() == 8u);
    CHECK(burst[0].w == 24);
    CHECK(opaque(burst[2]) > 0);
    CHECK(same_frames(burst, explosionFrames(explosion)));

    const auto ring = ringExpandFrames({.size = 20, .frameCount = 5, .color = white, .thickness = 2});
    REQUIRE(ring.size() == 5u);
    CHECK(opaque(ring[2]) > 0);

    const auto glow = glowPulseFrames({.size = 16, .frameCount = 6, .color = cyan});
    CHECK(glow.size() == 6u);

    const auto trail = trailFrames({.width = 30, .height = 10, .frameCount = 4, .color = yellow, .dotRadius = 2});
    REQUIRE(trail.size() == 4u);
    CHECK(trail[0].w == 30);

    const auto flash = flashFrames(4, white);
    REQUIRE(flash.size() == 4u);
    CHECK(getPixel(flash.front(), 0, 0).a > getPixel(flash.back(), 0, 0).a);

    const SparksConfig sparks{.size = 20, .frameCount = 6, .count = 8, .color = orange, .seed = 9};
    CHECK(sparksFrames(sparks).size() == 6u);
    CHECK(same_frames(sparksFrames(sparks), sparksFrames(sparks)));
}
