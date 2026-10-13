// tessera - bitmap font rendering with a built-in 8x8 pixel font.
// Covers printable ASCII (32-126); other characters render as a filled box.
#pragma once

#include "Canvas.hpp"

#include <cstdint>
#include <string_view>

namespace tessera {

// A bitmap font: cell size plus a glyph lookup returning one byte per row,
// MSB = leftmost pixel. glyphs always yields exactly height bytes.
struct Font {
    int width{};
    int height{};
    const std::uint8_t *(*glyphs)(char){}; // char -> pointer to height row bytes
};

// The built-in 8x8 font covering printable ASCII.
Font defaultFont();

// Render a single character to a fresh transparent canvas (width x height).
Canvas renderChar(const Font &font, Color col, char ch);

// Render a string left to right, no wrapping, to a fresh transparent canvas.
Canvas renderText(const Font &font, Color col, std::string_view text);

int textWidth(const Font &font, std::string_view text); // length * cell width
int textHeight(const Font &font);                       // one line == cell height

} // namespace tessera
