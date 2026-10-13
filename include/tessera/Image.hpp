// tessera - image file encoding (BMP, PNG) and writing to disk.
// PNG uses stored (uncompressed) DEFLATE blocks, so there is no zlib dependency.
#pragma once

#include "Canvas.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tessera {

std::vector<std::uint8_t> encodeBmp(const Canvas &c); // 32-bit BGRA, bottom-up, uncompressed
std::vector<std::uint8_t> encodePng(const Canvas &c); // 8-bit RGBA, DEFLATE stored blocks

bool writeBmp(const std::string &path, const Canvas &c);
bool writePng(const std::string &path, const Canvas &c);

} // namespace tessera
