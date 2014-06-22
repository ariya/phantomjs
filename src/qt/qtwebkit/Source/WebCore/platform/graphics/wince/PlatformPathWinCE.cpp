/*
 *  Copyright (C) 2007-2009 Torch Mobile, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "PlatformPathWinCE.h"

#include "AffineTransform.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "Path.h"
#include "WinCEGraphicsExtras.h"
#include <wtf/MathExtras.h>
#include <wtf/OwnPtr.h>
#include <wtf/text/WTFString.h>

#include <windows.h>

namespace WebCore {

// Implemented in GraphicsContextWinCE.cpp
void getEllipsePointByAngle(double angle, double a, double b, float& x, float& y);

static void quadCurve(int segments, Vector<PathPoint>& pts, const PathPoint* control)
{
    const float step = 1.0 / segments;
    register float tA = 0.0;
    register float tB = 1.0;

    float c1x = control[0].x();
    float c1y = control[0].y();
    float c2x = control[1].x();
    float c2y = control[1].y();
    float c3x = control[2].x();
    float c3y = control[2].y();

    const int offset = pts.size();
    pts.resize(offset + segments);
    PathPoint pp;
    pp.m_x = c1x;
    pp.m_y = c1y;

    for (int i = 1; i < segments; ++i) {
        tA += step;
        tB -= step;

        const float a = tB * tB;
        const float b = 2.0 * tA * tB;
        const float c = tA * tA;

        pp.m_x = c1x * a + c2x * b + c3x * c;
        pp.m_y = c1y * a + c2y * b + c3y * c;

        pts[offset + i - 1] = pp;
    }

    pp.m_x = c3x;
    pp.m_y = c3y;
    pts[offset + segments - 1] = pp;
}

static inline void bezier(int segments, Vector<PathPoint>& pts, const PathPoint* control)
{
    const float step = 1.0 / segments;
    register float tA = 0.0;
    register float tB = 1.0;

    float c1x = control[0].x();
    float c1y = control[0].y();
    float c2x = control[1].x();
    float c2y = control[1].y();
    float c3x = control[2].x();
    float c3y = control[2].y();
    float c4x = control[3].x();
    float c4y = control[3].y();

    const int offset = pts.size();
    pts.resize(offset + segments);
    PathPoint pp;
    pp.m_x = c1x;
    pp.m_y = c1y;

    for (int i = 1; i < segments; ++i) {
        tA += step;
        tB -= step;
        const float tAsq = tA * tA;
        const float tBsq = tB * tB;

        const float a = tBsq * tB;
        const float b = 3.0 * tA * tBsq;
        const float c = 3.0 * tB * tAsq;
        const float d = tAsq * tA;

        pp.m_x = c1x * a + c2x * b + c3x * c + c4x * d;
        pp.m_y = c1y * a + c2y * b + c3y * c + c4y * d;

        pts[offset + i - 1] = pp;
    }

    pp.m_x = c4x;
    pp.m_y = c4y;
    pts[offset + segments - 1] = pp;
}

static bool containsPoint(const FloatRect& r, const FloatPoint& p)
{
    return p.x() >= r.x() && p.y() >= r.y() && p.x() < r.maxX() && p.y() < r.maxY();
}

static void normalizeAngle(float& angle)
{
    angle = fmod(angle, 2 * piFloat);
    if (angle < 0)
        angle += 2 * piFloat;
    if (angle < 0.00001f)
        angle = 0;
}

static void transformArcPoint(float& x, float& y, const FloatPoint& c)
{
    x += c.x();
    y += c.y();
}

static void inflateRectToContainPoint(FloatRect& r, float x, float y)
{
    if (r.isEmpty()) {
        r.setX(x);
        r.setY(y);
        r.setSize(FloatSize(1, 1));
        return;
    }
    if (x < r.x()) {
        r.setWidth(r.maxX() - x);
        r.setX(x);
    } else {
        float w = x - r.x() + 1;
        if (w > r.width())
            r.setWidth(w);
    }
    if (y < r.y()) {
        r.setHeight(r.maxY() - y);
        r.setY(y);
    } else {
        float h =  y - r.y() + 1;
        if (h > r.height())
            r.setHeight(h);
    }
}

// return 0-based value: 0 - first Quadrant ( 0 - 90 degree)
static inline int quadrant(const PathPoint& point, const PathPoint& origin)
{
    return point.m_x < origin.m_x ?
        (point.m_y < origin.m_y ? 2 : 1)
        : (point.m_y < origin.m_y ? 3 : 0);
}

static inline bool isQuadrantOnLeft(int q) { return q == 1 || q == 2; }
static inline bool isQuadrantOnRight(int q) { return q == 0 || q == 3; }
static inline bool isQuadrantOnTop(int q) { return q == 2 || q == 3; }
static inline bool isQuadrantOnBottom(int q) { return q == 0 || q == 1; }

static inline int nextQuadrant(int q) { return q == 3 ? 0 : q + 1; }
static inline int quadrantDiff(int q1, int q2)
{
    int d = q1 - q2;
    while (d < 0)
        d += 4;
    return d;
}

struct PathVector {
    float m_x;
    float m_y;

    PathVector() : m_x(0), m_y(0) {}
    PathVector(float x, float y) : m_x(x), m_y(y) {}
    double angle() const { return atan2(m_y, m_x); }
    operator double () const { return angle(); }
    double length() const { return _hypot(m_x, m_y); }
};

PathVector operator-(const PathPoint& p1, const PathPoint& p2)
{
    return PathVector(p1.m_x - p2.m_x, p1.m_y - p2.m_y);
}

static void addArcPoint(PathPolygon& poly, const PathPoint& center, const PathPoint& radius, double angle)
{
    PathPoint p;
    getEllipsePointByAngle(angle, radius.m_x, radius.m_y, p.m_x, p.m_y);
    transformArcPoint(p.m_x, p.m_y, center);
    if (poly.isEmpty() || poly.last() != p)
        poly.append(p);
}

static void addArcPoints(PathPolygon& poly, const PlatformPathElement::ArcTo& data)
{
    const PathPoint& startPoint = poly.last();
    double curAngle = startPoint - data.m_center;
    double endAngle = data.m_end - data.m_center;
    double angleStep = 2. / std::max(data.m_radius.m_x, data.m_radius.m_y);
    if (data.m_clockwise) {
        if (endAngle <= curAngle || startPoint == data.m_end)
            endAngle += 2 * piDouble;
    } else {
        angleStep = -angleStep;
        if (endAngle >= curAngle || startPoint == data.m_end)
            endAngle -= 2 * piDouble;
    }

    for (curAngle += angleStep; data.m_clockwise ? curAngle < endAngle : curAngle > endAngle; curAngle += angleStep)
        addArcPoint(poly, data.m_center, data.m_radius, curAngle);

    if (poly.isEmpty() || poly.last() != data.m_end)
        poly.append(data.m_end);
}

static void drawPolygons(HDC dc, const Vector<PathPolygon>& polygons, bool fill, const AffineTransform* transformation)
{
    for (Vector<PathPolygon>::const_iterator i = polygons.begin(); i != polygons.end(); ++i) {
        int npoints = i->size();
        if (!npoints)
            continue;

        POINT* winPoints = 0;
        if (fill) {
            if (npoints > 2)
                winPoints = new POINT[npoints + 1];
        } else
            winPoints = new POINT[npoints];

        if (winPoints) {
            if (transformation) {
                for (int i2 = 0; i2 < npoints; ++i2) {
                    FloatPoint trPoint = transformation->mapPoint(i->at(i2));
                    winPoints[i2].x = stableRound(trPoint.x());
                    winPoints[i2].y = stableRound(trPoint.y());
                }
            } else {
                for (int i2 = 0; i2 < npoints; ++i2) {
                    winPoints[i2].x = stableRound(i->at(i2).x());
                    winPoints[i2].y = stableRound(i->at(i2).y());
                }
            }

            if (fill && winPoints[npoints - 1] != winPoints[0]) {
                winPoints[npoints].x = winPoints[0].x;
                winPoints[npoints].y = winPoints[0].y;
                ++npoints;
            }

            if (fill)
                ::Polygon(dc, winPoints, npoints);
            else
                ::Polyline(dc, winPoints, npoints);
            delete[] winPoints;
        }
    }
}


int PlatformPathElement::numControlPoints() const
{
    switch (m_type) {
    case PathMoveTo:
    case PathLineTo:
        return 1;
    case PathQuadCurveTo:
    case PathArcTo:
        return 2;
    case PathBezierCurveTo:
        return 3;
    default:
        ASSERT(m_type == PathCloseSubpath);
        return 0;
    }
}

int PlatformPathElement::numPoints() const
{
    switch (m_type) {
    case PathMoveTo:
    case PathLineTo:
    case PathArcTo:
        return 1;
    case PathQuadCurveTo:
        return 2;
    case PathBezierCurveTo:
        return 3;
    default:
        ASSERT(m_type == PathCloseSubpath);
        return 0;
    }
}

void PathPolygon::move(const FloatSize& offset)
{
    for (Vector<PathPoint>::iterator i = begin(); i < end(); ++i)
        i->move(offset);
}

void PathPolygon::transform(const AffineTransform& t)
{
    for (Vector<PathPoint>::iterator i = begin(); i < end(); ++i)
        *i = t.mapPoint(*i);
}

bool PathPolygon::contains(const FloatPoint& point) const
{
    if (size() < 3)
        return false;

    // Test intersections between the polygon and the vertical line: x = point.x()

    int intersected = 0;
    const PathPoint* point1 = &last();
    Vector<PathPoint>::const_iterator last = end();
    // wasNegative: -1 means unknown, 0 means false, 1 means true.
    int wasNegative = -1;
    for (Vector<PathPoint>::const_iterator i = begin(); i != last; ++i) {
        const PathPoint& point2 = *i;
        if (point1->x() != point.x()) {
            if (point2.x() == point.x()) {
                // We are getting on the vertical line
                wasNegative = point1->x() < point.x() ? 1 : 0;
            } else if (point2.x() < point.x() != point1->x() < point.x()) {
                float y = (point2.y() - point1->y()) / (point2.x() - point1->x()) * (point.x() - point1->x()) + point1->y();
                if (y >= point.y())
                    ++intersected;
            }
        } else {
            // We were on the vertical line

            // handle special case
            if (point1->y() == point.y())
                return true;

            if (point1->y() > point.y()) {
                if (point2.x() == point.x()) {
                    // see if the point is on this segment
                    if (point2.y() <= point.y())
                        return true;

                    // We are still on the line
                } else {
                    // We are leaving the line now.
                    // We have to get back to see which side we come from. If we come from
                    // the same side we are leaving, no intersection should be counted
                    if (wasNegative < 0) {
                        Vector<PathPoint>::const_iterator jLast = i;
                        Vector<PathPoint>::const_iterator j = i;
                        do {
                            if (j == begin())
                                j = last;
                            else
                                --j;
                            if (j->x() != point.x()) {
                                if (j->x() > point.x())
                                    wasNegative = 0;
                                else
                                    wasNegative = 1;
                                break;
                            }
                        } while (j != jLast);

                        if (wasNegative < 0)
                            return false;
                    }
                    if (wasNegative ? point2.x() > point.x() : point2.x() < point.x())
                        ++intersected;
                }
            } else if (point2.x() == point.x() && point2.y() >= point.y())
                return true;
        }
        point1 = &point2;
    }

    return intersected & 1;
}

void PlatformPathElement::move(const FloatSize& offset)
{
    int n = numControlPoints();
    for (int i = 0; i < n; ++i)
        m_data.m_points[i].move(offset);
}

void PlatformPathElement::transform(const AffineTransform& t)
{
    int n = numControlPoints();
    for (int i = 0; i < n; ++i) {
        FloatPoint p = t.mapPoint(m_data.m_points[i]);
        m_data.m_points[i].set(p.x(), p.y());
    }
}

void PlatformPathElement::inflateRectToContainMe(FloatRect& r, const FloatPoint& lastPoint) const
{
    if (m_type == PathArcTo) {
        const ArcTo& data = m_data.m_arcToData;
        PathPoint startPoint;
        startPoint = lastPoint;
        PathPoint endPoint = data.m_end;
        if (!data.m_clockwise)
            std::swap(startPoint, endPoint);

        int q0 = quadrant(startPoint, data.m_center);
        int q1 = quadrant(endPoint, data.m_center);
        bool containsExtremes[4] = { false }; // bottom, left, top, right
        static const PathPoint extremeVectors[4] = { { 0, 1 }, { -1, 0 }, { 0, -1 }, { 1, 0 } };
        if (q0 == q1) {
            if (startPoint.m_x == endPoint.m_x || isQuadrantOnBottom(q0) != startPoint.m_x > endPoint.m_x) {
                for (int i = 0; i < 4; ++i)
                    containsExtremes[i] = true;
            }
        } else {
            int extreme = q0;
            int diff = quadrantDiff(q1, q0);
            for (int i = 0; i < diff; ++i) {
                containsExtremes[extreme] = true;
                extreme = nextQuadrant(extreme);
            }
        }

        inflateRectToContainPoint(r, startPoint.m_x, startPoint.m_y);
        inflateRectToContainPoint(r, endPoint.m_x, endPoint.m_y);
        for (int i = 0; i < 4; ++i) {
            if (containsExtremes[i])
                inflateRectToContainPoint(r, data.m_center.m_x + data.m_radius.m_x * extremeVectors[i].m_x, data.m_center.m_y + data.m_radius.m_y * extremeVectors[i].m_y);
        }
    } else {
        int n = numPoints();
        for (int i = 0; i < n; ++i)
            inflateRectToContainPoint(r, m_data.m_points[i].m_x, m_data.m_points[i].m_y);
    }
}

PathElementType PlatformPathElement::type() const
{
    switch (m_type) {
    case PathMoveTo:
        return PathElementMoveToPoint;
    case PathLineTo:
        return PathElementAddLineToPoint;
    case PathArcTo:
        // FIXME: there's no arcTo type for PathElement
        return PathElementAddLineToPoint;
        // return PathElementAddQuadCurveToPoint;
    case PathQuadCurveTo:
        return PathElementAddQuadCurveToPoint;
    case PathBezierCurveTo:
        return PathElementAddCurveToPoint;
    default:
        ASSERT(m_type == PathCloseSubpath);
        return PathElementCloseSubpath;
    }
}

PlatformPath::PlatformPath()
    : m_penLifted(true)
{
    m_currentPoint.clear();
}

void PlatformPath::ensureSubpath()
{
    if (m_penLifted) {
        m_penLifted = false;
        m_subpaths.append(PathPolygon());
        m_subpaths.last().append(m_currentPoint);
    } else
        ASSERT(!m_subpaths.isEmpty());
}

void PlatformPath::addToSubpath(const PlatformPathElement& e)
{
    if (e.platformType() == PlatformPathElement::PathMoveTo) {
        m_penLifted = true;
        m_currentPoint = e.pointAt(0);
    } else if (e.platformType() == PlatformPathElement::PathCloseSubpath) {
        m_penLifted = true;
        if (!m_subpaths.isEmpty()) {
            if (m_currentPoint != m_subpaths.last()[0]) {
                // According to W3C, we have to draw a line from current point to the initial point
                m_subpaths.last().append(m_subpaths.last()[0]);
                m_currentPoint = m_subpaths.last()[0];
            }
        } else
            m_currentPoint.clear();
    } else {
        ensureSubpath();
        switch (e.platformType()) {
        case PlatformPathElement::PathLineTo:
            m_subpaths.last().append(e.pointAt(0));
            break;
        case PlatformPathElement::PathArcTo:
            addArcPoints(m_subpaths.last(), e.arcTo());
            break;
        case PlatformPathElement::PathQuadCurveTo:
            {
                PathPoint control[] = {
                    m_currentPoint,
                    e.pointAt(0),
                    e.pointAt(1),
                };
                // FIXME: magic number?
                quadCurve(50, m_subpaths.last(), control);
            }
            break;
        case PlatformPathElement::PathBezierCurveTo:
            {
                PathPoint control[] = {
                    m_currentPoint,
                    e.pointAt(0),
                    e.pointAt(1),
                    e.pointAt(2),
                };
                // FIXME: magic number?
                bezier(100, m_subpaths.last(), control);
            }
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }
        m_currentPoint = m_subpaths.last().last();
    }
}

void PlatformPath::append(const PlatformPathElement& e)
{
    e.inflateRectToContainMe(m_boundingRect, lastPoint());
    addToSubpath(e);
    m_elements.append(e);
}

void PlatformPath::append(const PlatformPath& p)
{
    const PlatformPathElements& e = p.elements();
    for (PlatformPathElements::const_iterator it(e.begin()); it != e.end(); ++it) {
        addToSubpath(*it);
        it->inflateRectToContainMe(m_boundingRect, lastPoint());
        m_elements.append(*it);
    }
}

void PlatformPath::clear()
{
    m_elements.clear();
    m_boundingRect = FloatRect();
    m_subpaths.clear();
    m_currentPoint.clear();
    m_penLifted = true;
}

void PlatformPath::strokePath(HDC dc, const AffineTransform* transformation) const
{
    drawPolygons(dc, m_subpaths, false, transformation);
}

void PlatformPath::fillPath(HDC dc, const AffineTransform* transformation) const
{
    HGDIOBJ oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
    drawPolygons(dc, m_subpaths, true, transformation);
    SelectObject(dc, oldPen);
}

void PlatformPath::translate(const FloatSize& size)
{
    for (PlatformPathElements::iterator it(m_elements.begin()); it != m_elements.end(); ++it)
        it->move(size);

    m_boundingRect.move(size);
    for (Vector<PathPolygon>::iterator it = m_subpaths.begin(); it != m_subpaths.end(); ++it)
        it->move(size);
}

void PlatformPath::transform(const AffineTransform& t)
{
    for (PlatformPathElements::iterator it(m_elements.begin()); it != m_elements.end(); ++it)
        it->transform(t);

    m_boundingRect = t.mapRect(m_boundingRect);
    for (Vector<PathPolygon>::iterator it = m_subpaths.begin(); it != m_subpaths.end(); ++it)
        it->transform(t);
}

bool PlatformPath::contains(const FloatPoint& point, WindRule rule) const
{
    // optimization: check the bounding rect first
    if (!containsPoint(m_boundingRect, point))
        return false;

    for (Vector<PathPolygon>::const_iterator i = m_subpaths.begin(); i != m_subpaths.end(); ++i) {
        if (i->contains(point))
            return true;
    }

    return false;
}

void PlatformPath::moveTo(const FloatPoint& point)
{
    PlatformPathElement::MoveTo data = { { point.x(), point.y() } };
    PlatformPathElement pe(data);
    append(pe);
}

void PlatformPath::addLineTo(const FloatPoint& point)
{
    PlatformPathElement::LineTo data = { { point.x(), point.y() } };
    PlatformPathElement pe(data);
    append(pe);
}

void PlatformPath::addQuadCurveTo(const FloatPoint& cp, const FloatPoint& p)
{
    PlatformPathElement::QuadCurveTo data = { { cp.x(), cp.y() }, { p.x(), p.y() } };
    PlatformPathElement pe(data);
    append(pe);
}

void PlatformPath::addBezierCurveTo(const FloatPoint& cp1, const FloatPoint& cp2, const FloatPoint& p)
{
    PlatformPathElement::BezierCurveTo data = { { cp1.x(), cp1.y() }, { cp2.x(), cp2.y() }, { p.x(), p.y() } };
    PlatformPathElement pe(data);
    append(pe);
}

void PlatformPath::addArcTo(const FloatPoint& fp1, const FloatPoint& fp2, float radius)
{
    const PathPoint& p0 = m_currentPoint;
    PathPoint p1;
    p1 = fp1;
    PathPoint p2;
    p2 = fp2;
    if (!radius || p0 == p1 || p1 == p2) {
        addLineTo(p1);
        return;
    }

    PathVector v01 = p0 - p1;
    PathVector v21 = p2 - p1;

    // sin(A - B) = sin(A) * cos(B) - sin(B) * cos(A)
    double cross = v01.m_x * v21.m_y - v01.m_y * v21.m_x;

    if (fabs(cross) < 1E-10) {
        // on one line
        addLineTo(p1);
        return;
    }

    double d01 = v01.length();
    double d21 = v21.length();
    double angle = (piDouble - fabs(asin(cross / (d01 * d21)))) * 0.5;
    double span = radius * tan(angle);
    double rate = span / d01;
    PathPoint startPoint;
    startPoint.m_x = p1.m_x + v01.m_x * rate;
    startPoint.m_y = p1.m_y + v01.m_y * rate;

    addLineTo(startPoint);

    PathPoint endPoint;
    rate = span / d21;
    endPoint.m_x = p1.m_x + v21.m_x * rate;
    endPoint.m_y = p1.m_y + v21.m_y * rate;

    PathPoint midPoint;
    midPoint.m_x = (startPoint.m_x + endPoint.m_x) * 0.5;
    midPoint.m_y = (startPoint.m_y + endPoint.m_y) * 0.5;

    PathVector vm1 = midPoint - p1;
    double dm1 = vm1.length();
    double d = _hypot(radius, span);

    PathPoint centerPoint;
    rate = d / dm1;
    centerPoint.m_x = p1.m_x + vm1.m_x * rate;
    centerPoint.m_y = p1.m_y + vm1.m_y * rate;

    PlatformPathElement::ArcTo data = {
        endPoint,
        centerPoint,
        { radius, radius },
        cross < 0
    };
    PlatformPathElement pe(data);
    append(pe);
}

void PlatformPath::closeSubpath()
{
    PlatformPathElement pe;
    append(pe);
}

// add a circular arc centred at p with radius r from start angle sar (radians) to end angle ear
void PlatformPath::addEllipse(const FloatPoint& p, float a, float b, float sar, float ear, bool anticlockwise)
{
    float startX, startY, endX, endY;

    normalizeAngle(sar);
    normalizeAngle(ear);

    getEllipsePointByAngle(sar, a, b, startX, startY);
    getEllipsePointByAngle(ear, a, b, endX, endY);

    transformArcPoint(startX, startY, p);
    transformArcPoint(endX, endY, p);

    FloatPoint start(startX, startY);
    moveTo(start);

    PlatformPathElement::ArcTo data = { { endX, endY }, { p.x(), p.y() },  { a, b }, !anticlockwise };
    PlatformPathElement pe(data);
    append(pe);
}


void PlatformPath::addRect(const FloatRect& r)
{
    moveTo(r.location());

    float right = r.maxX() - 1;
    float bottom = r.maxY() - 1;
    addLineTo(FloatPoint(right, r.y()));
    addLineTo(FloatPoint(right, bottom));
    addLineTo(FloatPoint(r.x(), bottom));
    addLineTo(r.location());
}

void PlatformPath::addEllipse(const FloatRect& r)
{
    FloatSize radius(r.width() * 0.5, r.height() * 0.5);
    addEllipse(r.location() + radius, radius.width(), radius.height(), 0, 0, true);
}

void PlatformPath::apply(void* info, PathApplierFunction function) const
{
    PathElement pelement;
    FloatPoint points[3];
    pelement.points = points;

    for (PlatformPathElements::const_iterator it(m_elements.begin()); it != m_elements.end(); ++it) {
        pelement.type = it->type();
        int n = it->numPoints();
        for (int i = 0; i < n; ++i)
            points[i] = it->pointAt(i);
        function(info, &pelement);
    }
}

} // namespace Webcore
