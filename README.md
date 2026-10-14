# tessera

A pure C++20 raster engine. Build images from code: draw primitives, noise, gradients, blend modes, transforms, sprites,
tilemaps and procedural effects. No image files, no GPU, no dependencies beyond the standard library.

A `Canvas` is plain data, `{ int w, h; std::vector<std::uint8_t> rgba }`: RGBA, row-major, origin at the top left. It
uploads straight to SDL (`SDL_PIXELFORMAT_RGBA32`) or any other backend. Generators return a new canvas; draw calls take
a `Canvas &` and change it. Everything is a free function over plain structs.

## Example

```cpp
#include <tessera/Canvas.hpp>
#include <tessera/Gradient.hpp>
#include <tessera/Image.hpp>

int main() {
    tessera::Canvas c = tessera::radialGradient(256, 256, 128, 128, 128, {60, 40, 120, 255}, tessera::black);
    tessera::fillCircle(c, 128, 128, 40, {180, 160, 255, 255});
    return tessera::writePng("tessera.png", c) ? 0 : 1;
}
```

## Modules

| Header | Provides |
| --- | --- |
| `Canvas.hpp` | The canvas, pixel access, lines, rectangles, circles, cropping |
| `Color.hpp` | RGBA colors, blending, HSL and HSV, sRGB and linear light, color transforms |
| `Draw.hpp` | Thick and anti-aliased lines, polygons, ellipses, arcs, Bézier and Catmull-Rom curves, rounded rectangles, pattern fills |
| `Gradient.hpp` | Linear, radial and diagonal gradients |
| `Compose.hpp` | Stamping, overlays, blend modes, masks |
| `Transform.hpp` | Flips, rotations, scaling (nearest, bilinear and area average), shears, outlines, drop shadows, edge color spreading |
| `Filter.hpp` | Box and Gaussian blur, sharpen, edge detection, bloom |
| `Adjust.hpp` | Grayscale, tint, brightness, contrast, saturation, hue, posterize, threshold, sepia, color lookup tables |
| `Palette.hpp`, `Dither.hpp` | Palettes, palette swaps, median-cut quantization, ordered dithering |
| `Noise.hpp` | Seeded value, Perlin, fBm and turbulence noise |
| `VFX.hpp` | Explosion, ring, glow, trail, flash and spark animations |
| `Animation.hpp`, `Sprite.hpp` | Frame timing and cross-fades; named multi-frame sprites |
| `Sheet.hpp`, `NineSlice.hpp` | Sprite sheet packing; nine-slice panels |
| `Tilemap.hpp`, `Isometric.hpp` | Square and isometric tilemaps, isometric coordinates and hit tests |
| `Text.hpp` | A built-in 8 by 8 bitmap font |
| `Image.hpp` | PNG and BMP encoding and writing, with no zlib dependency |

Seeded generators repeat exactly for a seed on a given platform. Some use the platform's math library for trigonometry,
so results can differ by a pixel between platforms.

## Use it from CMake

```cmake
include(FetchContent)
FetchContent_Declare(tessera GIT_REPOSITORY https://github.com/Gondola-Bros-Entertainment/tessera.git GIT_TAG main)
FetchContent_MakeAvailable(tessera)
target_link_libraries(my_game PRIVATE tessera::tessera)
```

`add_subdirectory` on a checkout works the same way.

## Build and test

Requires CMake 3.21 or newer and a C++20 compiler. CI builds and tests with GCC and Clang on Linux, MSVC on Windows and
Apple Clang on macOS, and runs the tests under AddressSanitizer and UndefinedBehaviorSanitizer.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

| Option | Default | Effect |
| --- | --- | --- |
| `TESSERA_BUILD_TESTS` | On when tessera is the top-level project | Builds the doctest suite in `tests/` |
| `TESSERA_WARNINGS_AS_ERRORS` | On | Fails the build on any compiler warning |

Formatting follows `.clang-format` (clang-format 22).

## License

BSD-3-Clause. See [LICENSE](LICENSE).
