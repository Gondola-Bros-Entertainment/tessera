# tessera

A pure C++20 raster engine. Build images from code: draw primitives, noise,
gradients, blend modes, transforms, sprites, tilemaps, and procedural effects.
No image files, no GPU, no dependencies beyond the standard library.

A Canvas is plain data: `{ int w, h; std::vector<uint8_t> rgba }`, RGBA and
row-major, so it uploads straight to SDL (`SDL_PIXELFORMAT_RGBA32`) or any
other backend. Generators return a new Canvas; draw calls take a `Canvas&` and
mutate it. Free functions, no classes.

## Build

    cmake -S . -B build
    cmake --build build

Link the `tessera::tessera` target from your CMake project.

## Example

    tessera::Canvas c = tessera::radialGradient(256, 256, 128, 128, 128,
                                                {60, 40, 120, 255}, tessera::black);
    tessera::fillCircle(c, 128, 128, 40, {180, 160, 255, 255});
    tessera::writePng("tessera.png", c);

## License

BSD-3-Clause. See [LICENSE](LICENSE).
