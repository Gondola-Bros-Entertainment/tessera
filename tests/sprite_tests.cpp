#include "support.hpp"

#include <tessera/Animation.hpp>
#include <tessera/Sprite.hpp>
#include <tessera/Transform.hpp>

#include <doctest/doctest.h>

#include <vector>

using namespace tessera;
using namespace tessera::test;

TEST_CASE("Sprites report their frames and size") {
    const Sprite single = singleFrame("dot", newCanvas(5, 3, red));
    CHECK(single.name == "dot");
    CHECK(spriteWidth(single) == 5);
    CHECK(spriteHeight(single) == 3);
    CHECK(frameCount(single) == 1);
    CHECK(single.originX == 0);

    const Sprite walk = multiFrame("walk", 2, 3, {newCanvas(4, 4, red), newCanvas(4, 4, blue)});
    CHECK(frameCount(walk) == 2);
    CHECK(walk.originY == 3);
    REQUIRE(getFrame(walk, 1).has_value());
    CHECK(getPixel(*getFrame(walk, 1), 0, 0) == blue);
    CHECK_FALSE(getFrame(walk, 2).has_value());
    CHECK_FALSE(getFrame(walk, -1).has_value());

    const Sprite empty = multiFrame("empty", 0, 0, {});
    CHECK(spriteWidth(empty) == 0);
}

TEST_CASE("Mirroring flips every frame and trimming drops transparent borders") {
    const Sprite sprite = multiFrame("pair", 0, 0, {numbered(4, 2), numbered(4, 2)});
    const Sprite mirrored = mirrorSprite(sprite);
    for (int frame = 0; frame < frameCount(mirrored); ++frame) {
        CHECK(same(*getFrame(mirrored, frame), flipH(*getFrame(sprite, frame))));
    }

    Canvas padded = newCanvas(8, 8, transparent);
    fillRect(padded, 2, 3, 3, 2, white);
    const Sprite trimmed = trimSprite(singleFrame("padded", padded));
    CHECK(spriteWidth(trimmed) == 3);
    CHECK(spriteHeight(trimmed) == 2);
}

TEST_CASE("Looping animations wrap around their frames") {
    const Animation loop = loopAnimation(2, 3);
    CHECK(loop == animation(2, 3, LoopMode::Loop));
    CHECK(animationFrame(loop, 0) == 0);
    CHECK(animationFrame(loop, 1) == 0);
    CHECK(animationFrame(loop, 2) == 1);
    CHECK(animationFrame(loop, 5) == 2);
    CHECK(animationFrame(loop, 6) == 0);
    CHECK_FALSE(animationDone(loop, 1000));
}

TEST_CASE("One-shot animations stop on their last frame") {
    const Animation once = onceAnimation(1, 3);
    CHECK(animationFrame(once, 1) == 1);
    CHECK(animationFrame(once, 10) == 2);
    CHECK_FALSE(animationDone(once, 1));
    CHECK(animationDone(once, 10));
}

TEST_CASE("Ping-pong animations bounce between their ends") {
    const Animation bounce = pingPongAnimation(1, 3);
    std::vector<int> frames;
    for (int tick = 0; tick < 7; ++tick) {
        frames.push_back(animationFrame(bounce, tick));
    }
    CHECK(frames == std::vector<int>{0, 1, 2, 1, 0, 1, 2});
}

TEST_CASE("Cross-fades start at the first frame and end at the second") {
    const Canvas a = newCanvas(3, 3, red);
    const Canvas b = newCanvas(3, 3, blue);
    CHECK(same(blendFrames(0.0, a, b), a));
    CHECK(same(blendFrames(1.0, a, b), b));
    const Color middle = getPixel(blendFrames(0.5, a, b), 1, 1);
    CHECK(near(middle, Color{128, 0, 128, 255}));
}
