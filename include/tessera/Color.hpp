// tessera - RGBA color, color math, and HSL/HSV conversions.
#pragma once

#include <cstdint>

namespace tessera {

// An RGBA color, 8 bits per channel. Byte order R,G,B,A matches
// SDL_PIXELFORMAT_RGBA32, so a Canvas uploads to a texture with no conversion.
struct Color {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
    std::uint8_t a{255};
};

constexpr bool operator==(Color x, Color y) { return x.r == y.r && x.g == y.g && x.b == y.b && x.a == y.a; }
constexpr bool operator!=(Color x, Color y) { return !(x == y); }

// Hue/saturation/lightness: h in [0,360), s and l in [0,1].
struct HSL {
    double h{}, s{}, l{};
};
// Hue/saturation/value: h in [0,360), s and v in [0,1].
struct HSV {
    double h{}, s{}, v{};
};

// ---- named colors ----------------------------------------------------------
inline constexpr Color transparent{0, 0, 0, 0};
inline constexpr Color black{0, 0, 0, 255};
inline constexpr Color white{255, 255, 255, 255};
inline constexpr Color red{255, 0, 0, 255};
inline constexpr Color green{0, 255, 0, 255};
inline constexpr Color blue{0, 0, 255, 255};
inline constexpr Color yellow{255, 255, 0, 255};
inline constexpr Color cyan{0, 255, 255, 255};
inline constexpr Color magenta{255, 0, 255, 255};
inline constexpr Color orange{255, 165, 0, 255};
inline constexpr Color purple{128, 0, 128, 255};
inline constexpr Color pink{255, 192, 203, 255};
inline constexpr Color gray{128, 128, 128, 255};
inline constexpr Color darkGray{64, 64, 64, 255};
inline constexpr Color lightGray{192, 192, 192, 255};

// ---- color math ------------------------------------------------------------
Color lerp(double t, Color a, Color b);   // blend a -> b; t clamped to [0,1]
Color multiply(Color a, Color b);         // component-wise modulate
Color alphaBlend(Color src, Color dst);   // Porter-Duff "src over dst"
Color scaleAlpha(double factor, Color c); // scale alpha by a factor in [0,1]

constexpr Color withAlpha(std::uint8_t alpha, Color c) { return {c.r, c.g, c.b, alpha}; }

// ---- color space conversions ----------------------------------------------
HSL toHSL(Color c);
Color fromHSL(HSL hsl);
HSV toHSV(Color c);
Color fromHSV(HSV hsv);

// ---- color transforms ------------------------------------------------------
Color tintColor(Color tint, Color c); // multiply RGB, keep target alpha
Color invertColor(Color c);
Color grayscaleColor(Color c);               // BT.709 luminance
Color brightenColor(double amount, Color c); // amount in [-1,1]
Color contrastColor(double amount, Color c); // amount in [-1,1]
Color saturateColor(double amount, Color c); // amount in [-1,1]
Color shiftHueColor(double degrees, Color c);

} // namespace tessera
