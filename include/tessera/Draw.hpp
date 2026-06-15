// tessera - higher-level drawing: lines, polygons, ellipses, arcs, curves, rounded rects.
#pragma once

#include "Canvas.hpp"

#include <vector>

namespace tessera {

void drawThickLine(Canvas& c, int x0, int y0, int x1, int y1, int thickness, Color col);

void drawPolygon(Canvas& c, const std::vector<Point>& vertices, Color col);  // closed outline
void fillPolygon(Canvas& c, const std::vector<Point>& vertices, Color col);  // scanline fill

void drawEllipse(Canvas& c, int cx, int cy, int rx, int ry, Color col);      // midpoint outline
void fillEllipse(Canvas& c, int cx, int cy, int rx, int ry, Color col);
void drawArc(Canvas& c, int cx, int cy, int rx, int ry, double startDeg, double endDeg, Color col);

void drawBezier(Canvas& c, Point p0, Point ctrl, Point p1, Color col);              // quadratic
void drawCubicBezier(Canvas& c, Point p0, Point c1, Point c2, Point p1, Color col); // cubic
void drawCatmullRom(Canvas& c, const std::vector<Point>& points, Color col);        // spline

void drawAALine(Canvas& c, int x0, int y0, int x1, int y1, Color col);  // Wu's algorithm

void patternFill(Canvas& c, int x, int y, int w, int h, const Canvas& pattern);

void drawRoundRect(Canvas& c, int x, int y, int w, int h, int radius, Color col);
void fillRoundRect(Canvas& c, int x, int y, int w, int h, int radius, Color col);

} // namespace tessera
