// tessera - higher-level drawing implementation.
#include "tessera/Draw.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace tessera {
namespace {

constexpr int channelMax = 255;
constexpr double pi = 3.14159265358979323846;
constexpr double degToRad = pi / 180.0;

std::uint8_t clampByte(int n) { return static_cast<std::uint8_t>(std::max(0, std::min(channelMax, n))); }

// Walk a Bresenham line, invoking cb at every point.
template <class F> void bresenham(int x0, int y0, int x1, int y1, F &&cb) {
    const int dx = std::abs(x1 - x0);
    const int dy = -std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        cb(x0, y0);
        if (x0 == x1 && y0 == y1)
            break;
        const int e2 = err * 2;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void quadPoints(Canvas &c, int cx, int cy, int x, int y, Color col) {
    setPixel(c, cx + x, cy + y, col);
    setPixel(c, cx - x, cy + y, col);
    setPixel(c, cx + x, cy - y, col);
    setPixel(c, cx - x, cy - y, col);
}

} // namespace

void drawThickLine(Canvas &c, int x0, int y0, int x1, int y1, int thickness, Color col) {
    const int radius = thickness / 2;
    bresenham(x0, y0, x1, y1, [&](int px, int py) { fillCircle(c, px, py, radius, col); });
}

void drawPolygon(Canvas &c, const std::vector<Point> &v, Color col) {
    const std::size_t n = v.size();
    if (n < 2)
        return;
    for (std::size_t i = 0; i < n; ++i) {
        const Point a = v[i], b = v[(i + 1) % n];
        drawLine(c, a.x, a.y, b.x, b.y, col);
    }
}

void fillPolygon(Canvas &c, const std::vector<Point> &v, Color col) {
    const std::size_t n = v.size();
    if (n < 2)
        return;
    int minY = v[0].y, maxY = v[0].y;
    for (const Point &p : v) {
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }
    minY = std::max(0, minY);
    maxY = std::min(c.h - 1, maxY);
    std::vector<int> xs;
    for (int y = minY; y <= maxY; ++y) {
        xs.clear();
        for (std::size_t i = 0; i < n; ++i) {
            const Point a = v[i], b = v[(i + 1) % n];
            if (a.y == b.y)
                continue;
            if (y < std::min(a.y, b.y) || y >= std::max(a.y, b.y))
                continue;
            xs.push_back(a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y));
        }
        std::sort(xs.begin(), xs.end());
        for (std::size_t i = 0; i + 1 < xs.size(); i += 2)
            hLine(c, xs[i], xs[i + 1], y, col);
    }
}

void drawEllipse(Canvas &c, int cx, int cy, int rx, int ry, Color col) {
    if (rx <= 0 || ry <= 0) {
        setPixel(c, cx, cy, col);
        return;
    }
    const long long rxSq = static_cast<long long>(rx) * rx;
    const long long rySq = static_cast<long long>(ry) * ry;
    int x = 0, y = ry;
    long long d1 = rySq - rxSq * ry + rxSq / 4; // region 1: slope > -1
    for (;;) {
        quadPoints(c, cx, cy, x, y, col);
        if (rySq * (2 * x + 1) >= rxSq * (2 * y))
            break;
        const int nx = x + 1;
        if (d1 < 0) {
            d1 += rySq * (2 * nx + 1);
        } else {
            d1 += rySq * (2 * nx + 1) - rxSq * (2 * y - 2);
            --y;
        }
        x = nx;
    }
    long long d2 = rySq * (static_cast<long long>(x) * x + x) + rxSq * static_cast<long long>(y - 1) * (y - 1) -
                   rxSq * rySq + rySq / 4; // region 2: slope < -1
    while (y >= 0) {
        quadPoints(c, cx, cy, x, y, col);
        const int ny = y - 1;
        if (d2 > 0) {
            d2 -= rxSq * (2 * ny + 1);
        } else {
            d2 += rySq * (2 * x + 2) - rxSq * (2 * ny + 1);
            ++x;
        }
        y = ny;
    }
}

void fillEllipse(Canvas &c, int cx, int cy, int rx, int ry, Color col) {
    if (rx <= 0 || ry <= 0) {
        setPixel(c, cx, cy, col);
        return;
    }
    const double rxF = rx, ryF = ry, rySq = ryF * ryF;
    for (int dy = -ry; dy <= ry; ++dy) {
        const double dyF = dy;
        const int half = static_cast<int>(std::lround(rxF * std::sqrt(std::max(0.0, 1.0 - dyF * dyF / rySq))));
        hLine(c, cx - half, cx + half, cy + dy, col);
    }
}

void drawArc(Canvas &c, int cx, int cy, int rx, int ry, double startDeg, double endDeg, Color col) {
    constexpr int minArcSteps = 16;
    constexpr int arcStepsPerRadius = 4;
    const int steps = std::max(minArcSteps, std::max(rx, ry) * arcStepsPerRadius);
    const double start = startDeg * degToRad;
    const double step = (endDeg * degToRad - start) / steps;
    int prevX = 0, prevY = 0;
    for (int i = 0; i <= steps; ++i) {
        const double a = start + i * step;
        const int px = cx + static_cast<int>(std::lround(rx * std::cos(a)));
        const int py = cy + static_cast<int>(std::lround(ry * std::sin(a)));
        if (i > 0)
            drawLine(c, prevX, prevY, px, py, col);
        prevX = px;
        prevY = py;
    }
}

void drawBezier(Canvas &c, Point p0, Point ctrl, Point p1, Color col) {
    constexpr int steps = 32;
    int prevX = 0, prevY = 0;
    for (int i = 0; i <= steps; ++i) {
        const double t = static_cast<double>(i) / steps, u = 1.0 - t;
        const int px = static_cast<int>(std::lround(u * u * p0.x + 2.0 * u * t * ctrl.x + t * t * p1.x));
        const int py = static_cast<int>(std::lround(u * u * p0.y + 2.0 * u * t * ctrl.y + t * t * p1.y));
        if (i > 0)
            drawLine(c, prevX, prevY, px, py, col);
        prevX = px;
        prevY = py;
    }
}

void drawCubicBezier(Canvas &c, Point p0, Point c1, Point c2, Point p1, Color col) {
    constexpr int steps = 48;
    int prevX = 0, prevY = 0;
    for (int i = 0; i <= steps; ++i) {
        const double t = static_cast<double>(i) / steps, u = 1.0 - t;
        const double u2 = u * u, u3 = u2 * u, t2 = t * t, t3 = t2 * t;
        const int px = static_cast<int>(std::lround(u3 * p0.x + 3.0 * u2 * t * c1.x + 3.0 * u * t2 * c2.x + t3 * p1.x));
        const int py = static_cast<int>(std::lround(u3 * p0.y + 3.0 * u2 * t * c1.y + 3.0 * u * t2 * c2.y + t3 * p1.y));
        if (i > 0)
            drawLine(c, prevX, prevY, px, py, col);
        prevX = px;
        prevY = py;
    }
}

void drawCatmullRom(Canvas &c, const std::vector<Point> &pts, Color col) {
    const std::size_t n = pts.size();
    if (n < 2)
        return;
    if (n == 2) {
        drawLine(c, pts[0].x, pts[0].y, pts[1].x, pts[1].y, col);
        return;
    }
    if (n == 3) {
        drawLine(c, pts[0].x, pts[0].y, pts[1].x, pts[1].y, col);
        drawLine(c, pts[1].x, pts[1].y, pts[2].x, pts[2].y, col);
        return;
    }
    constexpr int steps = 16;
    constexpr double tension = 0.5;
    int prevX = 0, prevY = 0;
    bool first = true;
    for (std::size_t s = 0; s + 3 < n; ++s) {
        const Point p0 = pts[s], p1 = pts[s + 1], p2 = pts[s + 2], p3 = pts[s + 3];
        for (int i = 0; i <= steps; ++i) {
            const double t = static_cast<double>(i) / steps, t2 = t * t, t3 = t2 * t;
            const int px = static_cast<int>(std::lround(tension * (2.0 * p1.x + (p2.x - p0.x) * t +
                                                                   (2.0 * p0.x - 5.0 * p1.x + 4.0 * p2.x - p3.x) * t2 +
                                                                   (-p0.x + 3.0 * p1.x - 3.0 * p2.x + p3.x) * t3)));
            const int py = static_cast<int>(std::lround(tension * (2.0 * p1.y + (p2.y - p0.y) * t +
                                                                   (2.0 * p0.y - 5.0 * p1.y + 4.0 * p2.y - p3.y) * t2 +
                                                                   (-p0.y + 3.0 * p1.y - 3.0 * p2.y + p3.y) * t3)));
            if (!first)
                drawLine(c, prevX, prevY, px, py, col);
            prevX = px;
            prevY = py;
            first = false;
        }
    }
}

void drawAALine(Canvas &c, int x0, int y0, int x1, int y1, Color col) {
    const bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
    int ax = 0, ay = 0, bx = 0, by = 0;
    if (steep) {
        const bool swap = y0 > y1;
        const int sx = swap ? x1 : x0, sy = swap ? y1 : y0;
        const int ex = swap ? x0 : x1, ey = swap ? y0 : y1;
        ax = sy;
        ay = sx;
        bx = ey;
        by = ex;
    } else {
        const bool swap = x0 > x1;
        const int sx = swap ? x1 : x0, sy = swap ? y1 : y0;
        const int ex = swap ? x0 : x1, ey = swap ? y0 : y1;
        ax = sx;
        ay = sy;
        bx = ex;
        by = ey;
    }
    const int dx = bx - ax, dy = by - ay;
    const double gradient = dx == 0 ? 1.0 : static_cast<double>(dy) / dx;
    const auto plot = [&](int major, int minor, double intensity) {
        if (intensity <= 0.0)
            return;
        const int px = steep ? minor : major;
        const int py = steep ? major : minor;
        blendPixel(c, px, py, withAlpha(clampByte(static_cast<int>(std::lround(col.a * intensity))), col));
    };
    double intery = ay;
    for (int x = ax; x <= bx; ++x) {
        const int iy = static_cast<int>(std::floor(intery));
        const double frac = intery - iy;
        plot(x, iy, 1.0 - frac);
        plot(x, iy + 1, frac);
        intery += gradient;
    }
}

void patternFill(Canvas &c, int rx, int ry, int rw, int rh, const Canvas &pattern) {
    if (rw <= 0 || rh <= 0)
        return;
    const int pw = std::max(1, pattern.w), ph = std::max(1, pattern.h);
    for (int row = 0; row < rh; ++row)
        for (int col = 0; col < rw; ++col) {
            const Color px = getPixel(pattern, col % pw, row % ph);
            if (px.a > 0)
                setPixel(c, rx + col, ry + row, px);
        }
}

void drawRoundRect(Canvas &c, int x, int y, int w, int h, int radius, Color col) {
    if (w <= 0 || h <= 0)
        return;
    const int r = std::min(radius, std::min(w / 2, h / 2));
    const int x2 = x + w - 1, y2 = y + h - 1;
    drawLine(c, x + r, y, x2 - r, y, col);
    drawLine(c, x + r, y2, x2 - r, y2, col);
    drawLine(c, x, y + r, x, y2 - r, col);
    drawLine(c, x2, y + r, x2, y2 - r, col);
    drawArc(c, x + r, y + r, r, r, 180.0, 270.0, col);
    drawArc(c, x2 - r, y + r, r, r, 270.0, 360.0, col);
    drawArc(c, x + r, y2 - r, r, r, 90.0, 180.0, col);
    drawArc(c, x2 - r, y2 - r, r, r, 0.0, 90.0, col);
}

void fillRoundRect(Canvas &c, int x, int y, int w, int h, int radius, Color col) {
    if (w <= 0 || h <= 0)
        return;
    const int r = std::min(radius, std::min(w / 2, h / 2));
    fillRect(c, x + r, y, w - 2 * r, h, col);
    fillRect(c, x, y + r, r, h - 2 * r, col);
    fillRect(c, x + w - r, y + r, r, h - 2 * r, col);
    fillCircle(c, x + r, y + r, r, col);
    fillCircle(c, x + w - r - 1, y + r, r, col);
    fillCircle(c, x + r, y + h - r - 1, r, col);
    fillCircle(c, x + w - r - 1, y + h - r - 1, r, col);
}

} // namespace tessera
