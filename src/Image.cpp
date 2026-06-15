// tessera - BMP and PNG encoders plus file writers.
// PNG uses stored (uncompressed) DEFLATE blocks, so there is no zlib dependency.
#include "tessera/Image.hpp"

#include <algorithm>
#include <array>
#include <fstream>

namespace tessera {
namespace {

using Bytes = std::vector<std::uint8_t>;

constexpr std::uint32_t byteMask  = 0xFFu;
constexpr std::uint32_t word16Mask = 0xFFFFu;

void put16le(Bytes& o, std::uint32_t v) {
    o.push_back(static_cast<std::uint8_t>(v & byteMask));
    o.push_back(static_cast<std::uint8_t>((v >> 8) & byteMask));
}
void put32le(Bytes& o, std::uint32_t v) {
    o.push_back(static_cast<std::uint8_t>(v & byteMask));
    o.push_back(static_cast<std::uint8_t>((v >> 8) & byteMask));
    o.push_back(static_cast<std::uint8_t>((v >> 16) & byteMask));
    o.push_back(static_cast<std::uint8_t>((v >> 24) & byteMask));
}
void put32be(Bytes& o, std::uint32_t v) {
    o.push_back(static_cast<std::uint8_t>((v >> 24) & byteMask));
    o.push_back(static_cast<std::uint8_t>((v >> 16) & byteMask));
    o.push_back(static_cast<std::uint8_t>((v >> 8) & byteMask));
    o.push_back(static_cast<std::uint8_t>(v & byteMask));
}

// ---- BMP format constants --------------------------------------------------
constexpr int           bmpBytesPerPixel = 4;
constexpr int           bmpRowAlignment  = 4;   // rows padded to a 4-byte boundary
constexpr std::uint32_t bmpFileHeaderSize = 14;
constexpr std::uint32_t bmpDibHeaderSize  = 40;
constexpr std::uint32_t bmpHeaderSize     = bmpFileHeaderSize + bmpDibHeaderSize;
constexpr std::uint32_t bmpPelsPerMeter   = 3780;   // ~96 DPI
constexpr std::uint32_t bmpColorPlanes    = 1;
constexpr std::uint32_t bmpBitsPerPixel   = 32;
constexpr std::uint32_t bmpCompressionRgb = 0;      // BI_RGB

// ---- PNG / zlib format constants -------------------------------------------
constexpr std::array<std::uint8_t, 8> pngSignature{ 137, 80, 78, 71, 13, 10, 26, 10 };
constexpr int           pngTypeLen        = 4;
constexpr int           pngBytesPerPixel  = 4;
constexpr std::uint8_t  pngBitDepth8      = 8;
constexpr std::uint8_t  pngColorTypeRgba  = 6;
constexpr std::uint8_t  pngCompressionDeflate = 0;
constexpr std::uint8_t  pngFilterAdaptive     = 0;
constexpr std::uint8_t  pngInterlaceNone      = 0;
constexpr std::uint8_t  pngFilterNone         = 0;  // per-row filter byte
constexpr std::size_t   pngFilterByteSize     = 1;  // one filter byte prefixes each row

constexpr std::uint32_t crcPolynomial   = 0xEDB88320u;
constexpr std::uint32_t crcInitFinal    = 0xFFFFFFFFu;
constexpr std::uint32_t adlerModulus    = 65521u;
constexpr std::uint32_t adlerInit       = 1u;
constexpr std::size_t   deflateMaxStored = 65535;   // max bytes in one stored block
constexpr std::uint8_t  zlibCmf         = 0x78;     // deflate, 32K window
constexpr std::uint8_t  zlibFlg         = 0x01;     // (cmf<<8 | flg) % 31 == 0
constexpr std::uint8_t  deflateStoredFinal = 0x01;  // BFINAL=1, BTYPE=stored
constexpr std::uint8_t  deflateStoredCont  = 0x00;  // BFINAL=0, BTYPE=stored

std::array<std::uint32_t, 256> makeCrcTable() {
    std::array<std::uint32_t, 256> table{};
    for (std::uint32_t n = 0; n < table.size(); ++n) {
        std::uint32_t c = n;
        for (int k = 0; k < 8; ++k) c = (c & 1u) ? (crcPolynomial ^ (c >> 1)) : (c >> 1);
        table[n] = c;
    }
    return table;
}

std::uint32_t crc32(const Bytes& data) {
    static const std::array<std::uint32_t, 256> table = makeCrcTable();
    std::uint32_t c = crcInitFinal;
    for (std::uint8_t byte : data) c = table[(c ^ byte) & byteMask] ^ (c >> 8);
    return c ^ crcInitFinal;
}

std::uint32_t adler32(const Bytes& data) {
    std::uint32_t a = adlerInit, b = 0;
    for (std::uint8_t byte : data) {
        a = (a + byte) % adlerModulus;
        b = (b + a) % adlerModulus;
    }
    return (b << 16) | a;
}

// Wrap raw bytes in a zlib stream of stored (uncompressed) DEFLATE blocks.
Bytes zlibStore(const Bytes& raw) {
    Bytes out;
    out.push_back(zlibCmf);
    out.push_back(zlibFlg);
    std::size_t off = 0;
    do {
        const std::size_t n = std::min(deflateMaxStored, raw.size() - off);
        const bool last = off + n >= raw.size();
        out.push_back(last ? deflateStoredFinal : deflateStoredCont);
        put16le(out, static_cast<std::uint32_t>(n));
        put16le(out, static_cast<std::uint32_t>(~n & word16Mask));
        out.insert(out.end(), raw.begin() + static_cast<std::ptrdiff_t>(off),
                   raw.begin() + static_cast<std::ptrdiff_t>(off + n));
        off += n;
    } while (off < raw.size());
    put32be(out, adler32(raw));
    return out;
}

void pngChunk(Bytes& out, const char* type, const Bytes& body) {
    put32be(out, static_cast<std::uint32_t>(body.size()));
    Bytes typed;
    typed.reserve(static_cast<std::size_t>(pngTypeLen) + body.size());
    for (int i = 0; i < pngTypeLen; ++i) typed.push_back(static_cast<std::uint8_t>(type[i]));
    typed.insert(typed.end(), body.begin(), body.end());
    out.insert(out.end(), typed.begin(), typed.end());
    put32be(out, crc32(typed));
}

bool writeFile(const std::string& path, const Bytes& bytes) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(file);
}

} // namespace

Bytes encodeBmp(const Canvas& c) {
    const int rowSize = c.w * bmpBytesPerPixel;
    const int paddedRowSize = ((rowSize + bmpRowAlignment - 1) / bmpRowAlignment) * bmpRowAlignment;  // 32bpp rows are already aligned
    const std::uint32_t imageSize = static_cast<std::uint32_t>(paddedRowSize * c.h);
    Bytes o;
    o.reserve(bmpHeaderSize + imageSize);
    o.push_back('B');
    o.push_back('M');
    put32le(o, bmpHeaderSize + imageSize);
    put16le(o, 0);  // reserved
    put16le(o, 0);  // reserved
    put32le(o, bmpHeaderSize);
    put32le(o, bmpDibHeaderSize);
    put32le(o, static_cast<std::uint32_t>(c.w));
    put32le(o, static_cast<std::uint32_t>(c.h));  // positive height = bottom-up
    put16le(o, bmpColorPlanes);
    put16le(o, bmpBitsPerPixel);
    put32le(o, bmpCompressionRgb);
    put32le(o, imageSize);
    put32le(o, bmpPelsPerMeter);
    put32le(o, bmpPelsPerMeter);
    put32le(o, 0);  // colors in palette
    put32le(o, 0);  // important colors
    for (int y = c.h - 1; y >= 0; --y) {
        for (int x = 0; x < c.w; ++x) {
            const Color px = getPixel(c, x, y);
            o.push_back(px.b);
            o.push_back(px.g);
            o.push_back(px.r);
            o.push_back(px.a);
        }
        for (int p = rowSize; p < paddedRowSize; ++p) o.push_back(0);
    }
    return o;
}

Bytes encodePng(const Canvas& c) {
    Bytes out;
    out.insert(out.end(), pngSignature.begin(), pngSignature.end());

    Bytes ihdr;
    put32be(ihdr, static_cast<std::uint32_t>(c.w));
    put32be(ihdr, static_cast<std::uint32_t>(c.h));
    ihdr.push_back(pngBitDepth8);
    ihdr.push_back(pngColorTypeRgba);
    ihdr.push_back(pngCompressionDeflate);
    ihdr.push_back(pngFilterAdaptive);
    ihdr.push_back(pngInterlaceNone);
    pngChunk(out, "IHDR", ihdr);

    const int rowSize = c.w * pngBytesPerPixel;
    Bytes raw;
    raw.reserve(static_cast<std::size_t>(c.h) * (pngFilterByteSize + static_cast<std::size_t>(rowSize)));
    for (int y = 0; y < c.h; ++y) {
        raw.push_back(pngFilterNone);
        for (int x = 0; x < c.w; ++x) {
            const Color px = getPixel(c, x, y);
            raw.push_back(px.r);
            raw.push_back(px.g);
            raw.push_back(px.b);
            raw.push_back(px.a);
        }
    }
    pngChunk(out, "IDAT", zlibStore(raw));
    pngChunk(out, "IEND", {});
    return out;
}

bool writeBmp(const std::string& path, const Canvas& c) { return writeFile(path, encodeBmp(c)); }
bool writePng(const std::string& path, const Canvas& c) { return writeFile(path, encodePng(c)); }

} // namespace tessera
