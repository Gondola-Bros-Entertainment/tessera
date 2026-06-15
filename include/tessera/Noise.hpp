// tessera - deterministic procedural noise.
// Samplers return a value in [0,1] at a point; the canvas generators fill a
// grayscale (or two-color) Canvas. Seed-driven and reproducible.
#pragma once

#include "Canvas.hpp"

namespace tessera {

// ---- point samplers (value in [0,1]) ---------------------------------------
double valueSample(int seed, double scale, int x, int y);
double perlinSample(int seed, double scale, int x, int y);
double fbmSample(int seed, int octaves, double scale, int x, int y);
double turbulenceSample(int seed, int octaves, double scale, int x, int y);

// ---- canvas generators -----------------------------------------------------
Canvas valueNoise(int w, int h, int seed, double scale);
Canvas valueNoiseColor(int w, int h, int seed, double scale, Color a, Color b);
Canvas perlinNoise(int w, int h, int seed, double scale);
Canvas fbm(int w, int h, int seed, int octaves, double scale);
Canvas turbulence(int w, int h, int seed, int octaves, double scale);

} // namespace tessera
