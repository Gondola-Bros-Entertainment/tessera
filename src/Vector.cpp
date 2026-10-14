// tessera - Vector implementation: signed distances, paints and the rasterizer.
#include "tessera/Vector.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <variant>
#include <vector>

namespace tessera {
namespace {

constexpr double channelMax = 255.0;
constexpr double half = 0.5;
// Pixel centers lie half a pixel in from their corners.
constexpr double pixelCenter = 0.5;
// Coverage falls from 1 to 0 across one pixel centered on an edge.
constexpr double edgeHalfWidth = 0.5;
// Bisection steps when solving for an ellipse's nearest point; each halves the interval.
constexpr int ellipseBisections = 96;
// Below this a squared length counts as zero.
constexpr double epsilon = 1.0e-12;
constexpr double sqrt3 = 1.7320508075688772;
constexpr double oneThird = 1.0 / 3.0;
constexpr int rgbaChannels = 4;
constexpr double noDistance = std::numeric_limits<double>::infinity();

VecD add(VecD a, VecD b) { return {a.x + b.x, a.y + b.y}; }
VecD sub(VecD a, VecD b) { return {a.x - b.x, a.y - b.y}; }
VecD mul(VecD v, double s) { return {v.x * s, v.y * s}; }
double dot(VecD a, VecD b) { return a.x * b.x + a.y * b.y; }
double length(VecD v) { return std::hypot(v.x, v.y); }
VecD absolute(VecD v) { return {std::abs(v.x), std::abs(v.y)}; }

// p in the frame of a shape turned clockwise by angle about the origin.
VecD unrotate(VecD p, double angle) {
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return {c * p.x + s * p.y, -s * p.x + c * p.y};
}

// ---- signed distances: negative inside, in design units -------------------

double segmentDistance(VecD p, VecD from, VecD to) {
    const VecD pa = sub(p, from);
    const VecD ba = sub(to, from);
    const double baba = dot(ba, ba);
    const double t = baba > epsilon ? std::clamp(dot(pa, ba) / baba, 0.0, 1.0) : 0.0;
    return length(sub(pa, mul(ba, t)));
}

// The root s of (r0 z0 / (s + r0))^2 + (z1 / (s + 1))^2 = 1, found by bisection.
double ellipseRoot(double r0, double z0, double z1, double g) {
    const double n0 = r0 * z0;
    double low = z1 - 1.0;
    double high = g < 0.0 ? 0.0 : std::hypot(n0, z1) - 1.0;
    double s = 0.0;
    for (int i = 0; i < ellipseBisections; ++i) {
        s = (low + high) * half;
        if (s == low || s == high)
            break;
        const double ratio0 = n0 / (s + r0);
        const double ratio1 = z1 / (s + 1.0);
        const double value = ratio0 * ratio0 + ratio1 * ratio1 - 1.0;
        if (value > 0.0)
            low = s;
        else if (value < 0.0)
            high = s;
        else
            break;
    }
    return s;
}

// Unsigned distance from (y0, y1), both at least 0, to the ellipse with radii e0 >= e1 > 0.
double quadrantEllipseDistance(double e0, double e1, double y0, double y1) {
    if (y1 > 0.0) {
        if (y0 <= 0.0)
            return std::abs(y1 - e1);
        const double z0 = y0 / e0;
        const double z1 = y1 / e1;
        const double g = z0 * z0 + z1 * z1 - 1.0;
        if (g == 0.0)
            return 0.0;
        const double r0 = (e0 / e1) * (e0 / e1);
        const double s = ellipseRoot(r0, z0, z1, g);
        return std::hypot(r0 * y0 / (s + r0) - y0, y1 / (s + 1.0) - y1);
    }
    const double numerator = e0 * y0;
    const double denominator = e0 * e0 - e1 * e1;
    if (numerator >= denominator)
        return std::abs(y0 - e0);
    const double ratio = numerator / denominator;
    return std::hypot(e0 * ratio - y0, e1 * std::sqrt(1.0 - ratio * ratio));
}

// band: distances at least this far from the edge only need their sign and a lower bound.
double distance(VecD p, const Circle &s, double) { return length(sub(p, s.center)) - s.radius; }

double distance(VecD p, const Ellipse &s, double band) {
    const double a = std::abs(s.radii.x);
    const double b = std::abs(s.radii.y);
    const VecD q = absolute(unrotate(sub(p, s.center), s.rotation));
    if (a * a < epsilon || b * b < epsilon) {
        // A flat ellipse is the segment along its longer radius.
        return a >= b ? segmentDistance(q, {-a, 0.0}, {a, 0.0}) : segmentDistance(q, {0.0, -b}, {0.0, b});
    }
    const double rho = std::hypot(q.x / a, q.y / b);
    const double sign = rho < 1.0 ? -1.0 : 1.0;
    // Normalizing the axes shrinks distances by at most the shorter radius, which bounds the true distance below.
    const double bound = std::min(a, b) * std::abs(rho - 1.0);
    if (bound >= band)
        return sign * bound;
    return sign * (a >= b ? quadrantEllipseDistance(a, b, q.x, q.y) : quadrantEllipseDistance(b, a, q.y, q.x));
}

double distance(VecD p, const RoundRect &s, double) {
    const VecD extent = absolute(s.halfSize);
    const double corner = std::clamp(s.cornerRadius, 0.0, std::min(extent.x, extent.y));
    const VecD q = add(sub(absolute(unrotate(sub(p, s.center), s.rotation)), extent), {corner, corner});
    const VecD outside{std::max(q.x, 0.0), std::max(q.y, 0.0)};
    return length(outside) + std::min(std::max(q.x, q.y), 0.0) - corner;
}

double distance(VecD p, const Capsule &s, double) { return segmentDistance(p, s.from, s.to) - s.radius; }

double distance(VecD p, const Polygon &s, double) {
    const std::vector<VecD> &v = s.points;
    if (v.size() < 3)
        return noDistance;
    double nearest = dot(sub(p, v.front()), sub(p, v.front()));
    double sign = 1.0;
    for (std::size_t i = 0, j = v.size() - 1; i < v.size(); j = i, ++i) {
        const VecD e = sub(v[j], v[i]);
        const VecD w = sub(p, v[i]);
        const double ee = dot(e, e);
        const double t = ee > epsilon ? std::clamp(dot(w, e) / ee, 0.0, 1.0) : 0.0;
        const VecD gap = sub(w, mul(e, t));
        nearest = std::min(nearest, dot(gap, gap));
        // Crossing edges flip the sign, so it ends negative inside.
        const bool above = p.y >= v[i].y;
        const bool below = p.y < v[j].y;
        const bool left = e.x * w.y > e.y * w.x;
        if ((above && below && left) || (!above && !below && !left))
            sign = -sign;
    }
    return sign * std::sqrt(nearest) - s.rounding;
}

double distance(VecD p, const Curve &s, double) {
    const VecD a = sub(s.control, s.start);
    const VecD b = add(sub(s.start, mul(s.control, 2.0)), s.end);
    const double bb = dot(b, b);
    if (bb < epsilon)
        return segmentDistance(p, s.start, s.end) - s.width * half;
    // The nearest point's parameter solves a cubic, here in depressed form.
    const VecD c = mul(a, 2.0);
    const VecD d = sub(s.start, p);
    const double kx = dot(a, b) / bb;
    const double ky = (2.0 * dot(a, a) + dot(d, b)) / bb * oneThird;
    const double kz = dot(d, a) / bb;
    const double pp = ky - kx * kx;
    const double qq = kx * (2.0 * kx * kx - 3.0 * ky) + kz;
    const double h = qq * qq + 4.0 * pp * pp * pp;
    const auto squaredAt = [&](double t) {
        const VecD offset = add(d, mul(add(c, mul(b, t)), t));
        return dot(offset, offset);
    };
    double nearest = 0.0;
    if (h >= 0.0) {
        const double root = std::sqrt(h);
        const double t = std::clamp(std::cbrt((root - qq) * half) + std::cbrt((-root - qq) * half) - kx, 0.0, 1.0);
        nearest = squaredAt(t);
    } else {
        // Three real roots; the middle one is never the nearest.
        const double z = std::sqrt(-pp);
        const double angle = std::acos(std::clamp(qq / (pp * z * 2.0), -1.0, 1.0)) * oneThird;
        const double m = std::cos(angle);
        const double n = std::sin(angle) * sqrt3;
        nearest = std::min(squaredAt(std::clamp((m + m) * z - kx, 0.0, 1.0)),
                           squaredAt(std::clamp((-n - m) * z - kx, 0.0, 1.0)));
    }
    return std::sqrt(nearest) - s.width * half;
}

double distance(VecD p, const Ring &s, double) {
    return std::abs(length(sub(p, s.center)) - s.radius) - s.width * half;
}

// ---- bounds: a box in design units that contains the shape ----------------

struct Bounds {
    double minX{};
    double minY{};
    double maxX{};
    double maxY{};
};

Bounds around(VecD center, double rx, double ry) {
    return {center.x - rx, center.y - ry, center.x + rx, center.y + ry};
}

Bounds hull(const std::vector<VecD> &points, double grow) {
    Bounds box{noDistance, noDistance, -noDistance, -noDistance};
    for (const VecD point : points) {
        box.minX = std::min(box.minX, point.x - grow);
        box.minY = std::min(box.minY, point.y - grow);
        box.maxX = std::max(box.maxX, point.x + grow);
        box.maxY = std::max(box.maxY, point.y + grow);
    }
    return box;
}

Bounds bounds(const Circle &s) { return around(s.center, std::abs(s.radius), std::abs(s.radius)); }
Bounds bounds(const Ellipse &s) {
    const double c = std::cos(s.rotation);
    const double n = std::sin(s.rotation);
    return around(s.center, std::hypot(s.radii.x * c, s.radii.y * n), std::hypot(s.radii.x * n, s.radii.y * c));
}
Bounds bounds(const RoundRect &s) {
    const double c = std::abs(std::cos(s.rotation));
    const double n = std::abs(std::sin(s.rotation));
    const VecD extent = absolute(s.halfSize);
    return around(s.center, extent.x * c + extent.y * n, extent.x * n + extent.y * c);
}
Bounds bounds(const Capsule &s) { return hull({s.from, s.to}, std::abs(s.radius)); }
Bounds bounds(const Polygon &s) { return s.points.size() < 3 ? Bounds{} : hull(s.points, std::max(0.0, s.rounding)); }
// The curve stays inside the triangle of its three points.
Bounds bounds(const Curve &s) { return hull({s.start, s.control, s.end}, std::abs(s.width) * half); }
Bounds bounds(const Ring &s) {
    const double outer = std::abs(s.radius) + std::abs(s.width) * half;
    return around(s.center, outer, outer);
}

// ---- paints and compositing -----------------------------------------------

// Straight color in linear light, alpha in [0,1].
struct LinearColor {
    double r{};
    double g{};
    double b{};
    double a{};
};

LinearColor linear(Color c) {
    return {srgbToLinear(c.r), srgbToLinear(c.g), srgbToLinear(c.b), static_cast<double>(c.a) / channelMax};
}

Color paintAt(const Color &paint, VecD) { return paint; }
Color paintAt(const LinearGradient &paint, VecD p) {
    const VecD axis = sub(paint.to, paint.from);
    const double span = dot(axis, axis);
    return lerp(span > epsilon ? dot(sub(p, paint.from), axis) / span : 0.0, paint.start, paint.end);
}
Color paintAt(const RadialGradient &paint, VecD p) {
    return lerp(paint.radius > 0.0 ? length(sub(p, paint.center)) / paint.radius : 1.0, paint.inner, paint.outer);
}

double coverage(double pixels) { return std::clamp(edgeHalfWidth - pixels, 0.0, 1.0); }

// Composite c at coverage over one premultiplied pixel.
void over(float *pixel, const LinearColor &c, double cover) {
    const double alpha = c.a * cover;
    const double keep = 1.0 - alpha;
    pixel[0] = static_cast<float>(c.r * alpha + pixel[0] * keep);
    pixel[1] = static_cast<float>(c.g * alpha + pixel[1] * keep);
    pixel[2] = static_cast<float>(c.b * alpha + pixel[2] * keep);
    pixel[3] = static_cast<float>(alpha + pixel[3] * keep);
}

// The pixel span [first, last) that covers design coordinates [low, high] at scale, within [0, limit).
std::pair<int, int> pixelSpan(double low, double high, double scale, int limit) {
    const double edge = static_cast<double>(limit);
    const double first = std::clamp(std::floor(low * scale) - 1.0, 0.0, edge);
    const double last = std::clamp(std::ceil(high * scale) + 1.0, 0.0, edge);
    return {static_cast<int>(first), static_cast<int>(last)};
}

struct Target {
    std::vector<float> &buffer;
    int width{};
    int height{};
    double scaleX{};
    double scaleY{};
    // Pixels per design unit along the axis that converts distances.
    double scale{};
};

template <class S> void paintLayer(const Target &target, const S &shape, const Layer &layer) {
    const double strokeHalf = std::max(0.0, layer.strokeWidth) * half;
    const Bounds box = bounds(shape);
    const auto [x0, x1] = pixelSpan(box.minX - strokeHalf, box.maxX + strokeHalf, target.scaleX, target.width);
    const auto [y0, y1] = pixelSpan(box.minY - strokeHalf, box.maxY + strokeHalf, target.scaleY, target.height);
    const double band = strokeHalf + edgeHalfWidth / target.scale;
    const Color *solid = std::get_if<Color>(&layer.fill);
    const LinearColor solidFill = solid ? linear(*solid) : LinearColor{};
    const LinearColor stroke = linear(layer.stroke);
    for (int y = y0; y < y1; ++y) {
        const std::size_t row = static_cast<std::size_t>(y) * static_cast<std::size_t>(target.width);
        for (int x = x0; x < x1; ++x) {
            const VecD p{(x + pixelCenter) / target.scaleX, (y + pixelCenter) / target.scaleY};
            const double d = distance(p, shape, band);
            float *pixel = &target.buffer[(row + static_cast<std::size_t>(x)) * rgbaChannels];
            const double fill = coverage(d * target.scale);
            if (fill > 0.0) {
                const LinearColor color =
                    solid ? solidFill
                          : std::visit([&](const auto &paint) { return linear(paintAt(paint, p)); }, layer.fill);
                over(pixel, color, fill);
            }
            if (strokeHalf > 0.0) {
                const double outline = coverage((std::abs(d) - strokeHalf) * target.scale);
                if (outline > 0.0)
                    over(pixel, stroke, outline);
            }
        }
    }
}

} // namespace

Canvas rasterize(const Drawing &drawing, int pixelWidth, int pixelHeight) {
    const int w = std::max(1, pixelWidth);
    const int h = std::max(1, pixelHeight);
    Canvas out = newCanvas(w, h, transparent);
    if (!(drawing.width > 0.0) || !(drawing.height > 0.0))
        return out;
    std::vector<float> buffer(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * rgbaChannels, 0.0F);
    const double scaleX = static_cast<double>(w) / drawing.width;
    const double scaleY = static_cast<double>(h) / drawing.height;
    const Target target{buffer, w, h, scaleX, scaleY, std::min(scaleX, scaleY)};
    for (const Layer &layer : drawing.layers)
        std::visit([&](const auto &shape) { paintLayer(target, shape, layer); }, layer.shape);
    for (std::size_t i = 0; i < buffer.size(); i += rgbaChannels) {
        const double alpha = std::min(static_cast<double>(buffer[i + 3]), 1.0);
        const auto alphaByte = static_cast<std::uint8_t>(std::lround(alpha * channelMax));
        // A pixel whose alpha rounds away stays clear, color included.
        if (alphaByte == 0)
            continue;
        out.rgba[i] = linearToSrgb(buffer[i] / alpha);
        out.rgba[i + 1] = linearToSrgb(buffer[i + 1] / alpha);
        out.rgba[i + 2] = linearToSrgb(buffer[i + 2] / alpha);
        out.rgba[i + 3] = alphaByte;
    }
    return out;
}

} // namespace tessera
