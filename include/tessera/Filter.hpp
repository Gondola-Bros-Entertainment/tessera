// tessera - convolution image filters: blur, sharpen, edge detection, bloom.
#pragma once

#include "Canvas.hpp"

namespace tessera {

// Separable box blur, window 2*radius+1 (horizontal then vertical pass).
// radius <= 0 returns a copy of the source unchanged.
Canvas boxBlur(int radius, const Canvas &src);

// Approximate Gaussian blur: three box-blur passes (central limit theorem).
// radius <= 0 returns a copy of the source unchanged.
Canvas gaussianBlur(int radius, const Canvas &src);

// Unsharp mask: result = clamp(2*original - blurred); alpha preserved.
Canvas sharpen(const Canvas &src);

// Sobel edge detection: gradient magnitude as grayscale; alpha preserved.
Canvas edgeDetect(const Canvas &src);

// Bloom: extract pixels above threshold luminance, blur them, blend additively.
// threshold is in [0,1], mapped to [0,255]; alpha preserved.
Canvas bloom(int radius, double threshold, const Canvas &src);

} // namespace tessera
