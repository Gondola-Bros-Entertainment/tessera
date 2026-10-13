#include "support.hpp"

#include <tessera/Text.hpp>

#include <doctest/doctest.h>

using namespace tessera;
using namespace tessera::test;

TEST_CASE("The default font is 8 by 8") {
    const Font font = defaultFont();
    CHECK(font.width == 8);
    CHECK(font.height == 8);
    CHECK(textHeight(font) == 8);
    CHECK(textWidth(font, "abc") == 24);
    CHECK(textWidth(font, "") == 0);
}

TEST_CASE("Characters render in their color, and a space renders nothing") {
    const Font font = defaultFont();
    const Canvas a = renderChar(font, white, 'A');
    CHECK(a.w == 8);
    CHECK(a.h == 8);
    CHECK(opaque(a) > 0);
    CHECK(count(a, [](Color col) { return col.a != 0 && col != white; }) == 0);
    CHECK(opaque(renderChar(font, white, ' ')) == 0);
    CHECK_FALSE(same(renderChar(font, white, 'A'), renderChar(font, white, 'B')));
}

TEST_CASE("Characters outside printable ASCII render as an outlined box") {
    const Font font = defaultFont();
    const Canvas box = renderChar(font, white, '\x01');
    CHECK(opaque(box) == 28); // the 8 by 8 border
    CHECK(opaque(crop(box, 1, 1, 6, 6)) == 0);
}

TEST_CASE("Text renders characters side by side") {
    const Font font = defaultFont();
    const Canvas text = renderText(font, red, "Hi");
    CHECK(text.w == 16);
    CHECK(text.h == 8);
    CHECK(same(crop(text, 0, 0, 8, 8), renderChar(font, red, 'H')));
    CHECK(same(crop(text, 8, 0, 8, 8), renderChar(font, red, 'i')));
}
