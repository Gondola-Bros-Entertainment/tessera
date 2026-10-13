#include "support.hpp"

#include <tessera/Image.hpp>

#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace tessera;
using namespace tessera::test;

namespace {

std::uint32_t big_endian(const std::vector<std::uint8_t> &bytes, std::size_t at) {
    return (std::uint32_t{bytes[at]} << 24u) | (std::uint32_t{bytes[at + 1]} << 16u) |
           (std::uint32_t{bytes[at + 2]} << 8u) | std::uint32_t{bytes[at + 3]};
}

std::uint32_t little_endian(const std::vector<std::uint8_t> &bytes, std::size_t at) {
    return std::uint32_t{bytes[at]} | (std::uint32_t{bytes[at + 1]} << 8u) | (std::uint32_t{bytes[at + 2]} << 16u) |
           (std::uint32_t{bytes[at + 3]} << 24u);
}

/// The CRC-32 that PNG chunks carry, computed bit by bit as an independent reference.
std::uint32_t crc32(const std::uint8_t *data, std::size_t size) {
    std::uint32_t crc = 0xffffffffu;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1u) != 0 ? (crc >> 1u) ^ 0xedb88320u : crc >> 1u;
        }
    }
    return crc ^ 0xffffffffu;
}

std::uint32_t adler32(const std::vector<std::uint8_t> &data) {
    std::uint32_t a = 1;
    std::uint32_t b = 0;
    for (const std::uint8_t byte : data) {
        a = (a + byte) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16u) | a;
}

/// Decodes a PNG made of stored DEFLATE blocks, checking every chunk's CRC and the zlib checksum.
/// Returns the raw scanlines, each prefixed by its filter byte.
std::vector<std::uint8_t> stored_scanlines(const std::vector<std::uint8_t> &png) {
    std::vector<std::uint8_t> zlib;
    std::size_t at = 8;
    while (at + 12 <= png.size()) {
        const std::uint32_t length = big_endian(png, at);
        const std::string type(png.begin() + static_cast<std::ptrdiff_t>(at + 4),
                               png.begin() + static_cast<std::ptrdiff_t>(at + 8));
        REQUIRE(big_endian(png, at + 8 + length) == crc32(png.data() + at + 4, length + 4));
        if (type == "IDAT") {
            zlib.insert(zlib.end(), png.begin() + static_cast<std::ptrdiff_t>(at + 8),
                        png.begin() + static_cast<std::ptrdiff_t>(at + 8 + length));
        }
        at += 12 + length;
    }
    REQUIRE(zlib.size() >= 6);
    std::vector<std::uint8_t> raw;
    std::size_t block = 2; // after the zlib header
    bool final_block = false;
    while (!final_block) {
        final_block = (zlib[block] & 1u) != 0;
        REQUIRE((zlib[block] >> 1u) == 0); // stored block
        const std::size_t length = std::size_t{zlib[block + 1]} | (std::size_t{zlib[block + 2]} << 8u);
        const std::size_t complement = std::size_t{zlib[block + 3]} | (std::size_t{zlib[block + 4]} << 8u);
        REQUIRE((length ^ 0xffffu) == complement);
        raw.insert(raw.end(), zlib.begin() + static_cast<std::ptrdiff_t>(block + 5),
                   zlib.begin() + static_cast<std::ptrdiff_t>(block + 5 + length));
        block += 5 + length;
    }
    REQUIRE(big_endian(zlib, block) == adler32(raw));
    return raw;
}

} // namespace

TEST_CASE("PNG encoding is a valid file whose pixels decode back exactly") {
    const Canvas c = numbered(5, 3);
    const std::vector<std::uint8_t> png = encodePng(c);
    const std::array<std::uint8_t, 8> signature{137, 80, 78, 71, 13, 10, 26, 10};
    REQUIRE(png.size() > signature.size());
    CHECK(std::equal(signature.begin(), signature.end(), png.begin()));
    CHECK(big_endian(png, 16) == 5u);
    CHECK(big_endian(png, 20) == 3u);

    const std::vector<std::uint8_t> raw = stored_scanlines(png);
    REQUIRE(raw.size() == 3u * (1u + 5u * 4u));
    for (int y = 0; y < 3; ++y) {
        const std::size_t row = static_cast<std::size_t>(y) * 21u;
        CHECK(raw[row] == 0); // no filter
        for (int x = 0; x < 5; ++x) {
            const Color expected = getPixel(c, x, y);
            const std::size_t pixel = row + 1u + static_cast<std::size_t>(x) * 4u;
            CHECK(Color{raw[pixel], raw[pixel + 1], raw[pixel + 2], raw[pixel + 3]} == expected);
        }
    }
}

TEST_CASE("Large PNGs split their pixels across several stored blocks") {
    const Canvas big = newCanvas(200, 120, orange); // 96 KB of scanlines, above one block's 64 KB
    const std::vector<std::uint8_t> raw = stored_scanlines(encodePng(big));
    CHECK(raw.size() == 120u * (1u + 200u * 4u));
}

TEST_CASE("BMP encoding writes a bottom-up BGRA bitmap") {
    Canvas c = newCanvas(2, 2, transparent);
    setPixel(c, 0, 0, Color{10, 20, 30, 40});
    const std::vector<std::uint8_t> bmp = encodeBmp(c);
    REQUIRE(bmp.size() > 54);
    CHECK(bmp[0] == 'B');
    CHECK(bmp[1] == 'M');
    CHECK(little_endian(bmp, 2) == bmp.size());
    CHECK(little_endian(bmp, 18) == 2u);
    CHECK(little_endian(bmp, 22) == 2u);
    // The top-left pixel is the first pixel of the last stored row.
    const std::size_t top_left = little_endian(bmp, 10) + 2u * 4u;
    CHECK(bmp[top_left] == 30);
    CHECK(bmp[top_left + 1] == 20);
    CHECK(bmp[top_left + 2] == 10);
    CHECK(bmp[top_left + 3] == 40);
}

TEST_CASE("Writing images saves the encoded bytes to disk") {
    const Canvas c = numbered(4, 4);
    const auto dir = std::filesystem::temp_directory_path();
    const auto png_path = dir / "tessera_test.png";
    const auto bmp_path = dir / "tessera_test.bmp";
    REQUIRE(writePng(png_path.string(), c));
    REQUIRE(writeBmp(bmp_path.string(), c));
    std::ifstream png_file(png_path, std::ios::binary);
    const std::vector<std::uint8_t> saved{std::istreambuf_iterator<char>(png_file), std::istreambuf_iterator<char>()};
    CHECK(saved == encodePng(c));
    CHECK(std::filesystem::file_size(bmp_path) == encodeBmp(c).size());
    std::filesystem::remove(png_path);
    std::filesystem::remove(bmp_path);
    CHECK_FALSE(writePng((dir / "no-such-directory" / "x.png").string(), c));
}
