#include "support.hpp"

#include <tessera/Vector.hpp>

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numbers>
#include <vector>

using namespace tessera;
using namespace tessera::test;

namespace {

constexpr Color clear{0, 0, 0, 0};
constexpr Color rose{200, 40, 60, 255};

// Total alpha in whole pixels: a fully covered pixel counts 1.
double coverage(const Canvas &c) {
    double sum = 0.0;
    for (std::size_t i = 3; i < c.rgba.size(); i += 4)
        sum += c.rgba[i] / 255.0;
    return sum;
}

// Alpha summed down column x, in pixels.
double columnCoverage(const Canvas &c, int x) {
    double sum = 0.0;
    for (int y = 0; y < c.h; ++y)
        sum += getPixel(c, x, y).a / 255.0;
    return sum;
}

// Alpha summed along row y from column `from`, in pixels.
double rowCoverage(const Canvas &c, int y, int from) {
    double sum = 0.0;
    for (int x = from; x < c.w; ++x)
        sum += getPixel(c, x, y).a / 255.0;
    return sum;
}

Drawing single(double size, Shape shape, Paint fill = rose) {
    return Drawing{size, size, {Layer{std::move(shape), fill, {}, 0.0}}};
}

// Largest channel difference between two canvases of one size.
int largestDifference(const Canvas &a, const Canvas &b) {
    int largest = 0;
    for (std::size_t i = 0; i < a.rgba.size(); ++i)
        largest = std::max(largest, std::abs(a.rgba[i] - b.rgba[i]));
    return largest;
}

} // namespace

TEST_CASE("A circle covers pi r squared at any radius") {
    for (const double radius : {3.0, 10.0, 40.0}) {
        const double size = 2.0 * radius + 8.0;
        const int pixels = static_cast<int>(size);
        const Canvas c = rasterize(single(size, Circle{{size / 2.0, size / 2.0}, radius}), pixels, pixels);
        const double area = std::numbers::pi * radius * radius;
        CHECK(coverage(c) == doctest::Approx(area).epsilon(0.01));
    }
}

TEST_CASE("Inside is the fill color, far outside is clear, and edges blend only alpha") {
    const Canvas c = rasterize(single(30.0, Circle{{15.0, 15.0}, 10.0}), 30, 30);
    CHECK(getPixel(c, 15, 15) == rose);
    CHECK(getPixel(c, 0, 0) == transparent);
    int partial = 0;
    for (int y = 0; y < c.h; ++y)
        for (int x = 0; x < c.w; ++x) {
            const Color px = getPixel(c, x, y);
            if (px.a == 0 || px.a == 255)
                continue;
            ++partial;
            CHECK(withAlpha(255, px) == rose);
        }
    CHECK(partial > 0);
}

TEST_CASE("Doubling the canvas quadruples the coverage of every shape") {
    const Drawing drawing{40.0,
                          40.0,
                          {Layer{Ellipse{{12.0, 12.0}, {9.0, 5.0}, 0.4}, rose, {}, 0.0},
                           Layer{RoundRect{{28.0, 14.0}, {7.0, 4.0}, 2.0, 0.7}, rose, {}, 0.0},
                           Layer{Capsule{{6.0, 32.0}, {34.0, 28.0}, 3.0}, rose, {}, 0.0}}};
    const double small = coverage(rasterize(drawing, 40, 40));
    const double large = coverage(rasterize(drawing, 80, 80));
    CHECK(large == doctest::Approx(4.0 * small).epsilon(0.01));
}

TEST_CASE("Gradients take their end colors past their ends") {
    const Paint linear = LinearGradient{{20.0, 5.0}, {80.0, 5.0}, red, blue};
    const Canvas strip = rasterize(
        Drawing{100.0, 10.0, {Layer{RoundRect{{50.0, 5.0}, {50.0, 5.0}, 0.0, 0.0}, linear, {}, 0.0}}}, 100, 10);
    CHECK(getPixel(strip, 5, 5) == red);
    CHECK(getPixel(strip, 95, 5) == blue);
    // Pixel 49's center is 29.5 of the gradient's 60.
    CHECK(near(getPixel(strip, 49, 5), lerp(29.5 / 60.0, red, blue)));

    const Paint radial = RadialGradient{{50.5, 50.5}, 30.0, white, black};
    const Canvas disc = rasterize(single(101.0, Circle{{50.5, 50.5}, 45.0}, radial), 101, 101);
    CHECK(getPixel(disc, 50, 50) == white);
    CHECK(getPixel(disc, 50 + 40, 50) == black);
}

TEST_CASE("Strokes are as wide as requested and centered on the edge") {
    const Drawing outlined{60.0, 60.0, {Layer{Circle{{30.0, 30.0}, 20.0}, clear, white, 4.0}}};
    const Canvas c = rasterize(outlined, 60, 60);
    CHECK(rowCoverage(c, 30, 30) == doctest::Approx(4.0).epsilon(0.03));
    CHECK(getPixel(c, 50, 30).a == 255);
    CHECK(getPixel(c, 30, 30).a == 0);

    const Canvas ring = rasterize(single(60.0, Ring{{30.0, 30.0}, 20.0, 6.0}), 60, 60);
    CHECK(rowCoverage(ring, 30, 30) == doctest::Approx(6.0).epsilon(0.03));
}

TEST_CASE("Later layers cover earlier ones") {
    const Layer under{RoundRect{{10.0, 10.0}, {6.0, 6.0}, 0.0, 0.0}, red, {}, 0.0};
    const Layer top{RoundRect{{12.0, 12.0}, {6.0, 6.0}, 0.0, 0.0}, blue, {}, 0.0};
    CHECK(getPixel(rasterize(Drawing{20.0, 20.0, {under, top}}, 20, 20), 11, 11) == blue);
    CHECK(getPixel(rasterize(Drawing{20.0, 20.0, {top, under}}, 20, 20), 11, 11) == red);
}

TEST_CASE("A quarter turn swaps a shape's extents") {
    const double quarter = std::numbers::pi / 2.0;
    const Canvas turnedRect = rasterize(single(40.0, RoundRect{{20.0, 20.0}, {12.0, 5.0}, 2.0, quarter}), 40, 40);
    const Canvas tallRect = rasterize(single(40.0, RoundRect{{20.0, 20.0}, {5.0, 12.0}, 2.0, 0.0}), 40, 40);
    CHECK(largestDifference(turnedRect, tallRect) <= 1);
    const Canvas turnedEllipse = rasterize(single(40.0, Ellipse{{20.0, 20.0}, {12.0, 5.0}, quarter}), 40, 40);
    const Canvas tallEllipse = rasterize(single(40.0, Ellipse{{20.0, 20.0}, {5.0, 12.0}, 0.0}), 40, 40);
    CHECK(largestDifference(turnedEllipse, tallEllipse) <= 1);
    CHECK(coverage(tallEllipse) == doctest::Approx(std::numbers::pi * 12.0 * 5.0).epsilon(0.01));
}

TEST_CASE("Capsules and curves are as wide as requested") {
    const Canvas capsule =
        rasterize(Drawing{60.0, 40.0, {Layer{Capsule{{10.0, 20.0}, {50.0, 20.0}, 3.0}, rose, {}, 0.0}}}, 60, 40);
    CHECK(columnCoverage(capsule, 30) == doctest::Approx(6.0).epsilon(0.02));
    const Canvas straight = rasterize(
        Drawing{60.0, 40.0, {Layer{Curve{{10.0, 20.0}, {30.0, 20.0}, {50.0, 20.0}, 4.0}, rose, {}, 0.0}}}, 60, 40);
    CHECK(columnCoverage(straight, 30) == doctest::Approx(4.0).epsilon(0.02));
    // At its apex the arch runs level, so a column crosses it at its width.
    const Canvas arch = rasterize(
        Drawing{60.0, 40.0, {Layer{Curve{{10.0, 30.0}, {30.0, 0.0}, {50.0, 30.0}, 4.0}, rose, {}, 0.0}}}, 60, 40);
    CHECK(columnCoverage(arch, 29) == doctest::Approx(4.0).epsilon(0.03));
    CHECK(getPixel(arch, 29, 14).a == 255);
}

TEST_CASE("Polygons cover their area, and rounding grows them by perimeter times rounding") {
    const std::vector<VecD> square{{10.0, 10.0}, {40.0, 10.0}, {40.0, 40.0}, {10.0, 40.0}};
    CHECK(coverage(rasterize(single(50.0, Polygon{square, 0.0}), 50, 50)) == doctest::Approx(900.0).epsilon(0.002));
    const double grown = 900.0 + 120.0 * 3.0 + std::numbers::pi * 9.0;
    CHECK(coverage(rasterize(single(50.0, Polygon{square, 3.0}), 50, 50)) == doctest::Approx(grown).epsilon(0.002));
    // An L of three 10 by 10 squares.
    const std::vector<VecD> ell{{10.0, 10.0}, {20.0, 10.0}, {20.0, 20.0}, {30.0, 20.0}, {30.0, 30.0}, {10.0, 30.0}};
    CHECK(coverage(rasterize(single(40.0, Polygon{ell, 0.0}), 40, 40)) == doctest::Approx(300.0).epsilon(0.002));
    CHECK(coverage(rasterize(single(40.0, Polygon{{{1.0, 1.0}, {5.0, 5.0}}, 2.0}), 40, 40)) == 0.0);
}

TEST_CASE("A drawing without layers or area is clear, at least one pixel in size") {
    const Canvas empty = rasterize(Drawing{10.0, 10.0, {}}, 8, 6);
    CHECK(empty.w == 8);
    CHECK(empty.h == 6);
    CHECK(coverage(empty) == 0.0);
    const Canvas flat = rasterize(single(0.0, Circle{{0.0, 0.0}, 5.0}), 0, -3);
    CHECK(flat.w == 1);
    CHECK(flat.h == 1);
    CHECK(coverage(flat) == 0.0);
}
