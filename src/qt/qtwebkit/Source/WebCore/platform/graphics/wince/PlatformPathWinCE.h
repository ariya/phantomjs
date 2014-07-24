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

#ifndef PlatformPathWinCE_h
#define PlatformPathWinCE_h

#include "FloatPoint.h"
#include "FloatRect.h"
#include "Path.h"
#include <wtf/Vector.h>

namespace WebCore {

    class GraphicsContext;

    struct PathPoint {
        float m_x;
        float m_y;
        const float& x() const { return m_x; }
        const float& y() const { return m_y; }
        void set(float x, float y)
        {
            m_x = x;
            m_y = y;
        };
        operator FloatPoint() const { return FloatPoint(m_x, m_y); }
        void move(const FloatSize& offset)
        {
            m_x += offset.width();
            m_y += offset.height();
        }
        PathPoint& operator=(const FloatPoint& p)
        {
            m_x = p.x();
            m_y = p.y();
            return *this;
        }
        void clear() { m_x = m_y = 0; }
    };

    struct PathPolygon: public Vector<PathPoint> {
        void move(const FloatSize& offset);
        void transform(const AffineTransform& t);
        bool contains(const FloatPoint& point) const;
    };

    class PlatformPathElement {
    public:
        enum PlaformPathElementType {
            PathMoveTo,
            PathLineTo,
            PathArcTo,
            PathQuadCurveTo,
            PathBezierCurveTo,
            PathCloseSubpath,
        };

        struct MoveTo {
            PathPoint m_end;
        };

        struct LineTo {
            PathPoint m_end;
        };

        struct ArcTo {
            PathPoint m_end;
            PathPoint m_center;
            PathPoint m_radius;
            bool m_clockwise;
        };

        struct QuadCurveTo {
            PathPoint m_point0;
            PathPoint m_point1;
        };

        struct BezierCurveTo {
            PathPoint m_point0;
            PathPoint m_point1;
            PathPoint m_point2;
        };

        PlatformPathElement(): m_type(PathCloseSubpath) { m_data.m_points[0].set(0, 0);    }
        PlatformPathElement(const MoveTo& data): m_type(PathMoveTo) { m_data.m_moveToData = data; }
        PlatformPathElement(const LineTo& data): m_type(PathLineTo) { m_data.m_lineToData = data; }
        PlatformPathElement(const ArcTo& data): m_type(PathArcTo) { m_data.m_arcToData = data; }
        PlatformPathElement(const QuadCurveTo& data): m_type(PathQuadCurveTo) { m_data.m_quadCurveToData = data; }
        PlatformPathElement(const BezierCurveTo& data): m_type(PathBezierCurveTo) { m_data.m_bezierCurveToData = data; }

        const MoveTo& moveTo() const { return m_data.m_moveToData; }
        const LineTo& lineTo() const { return m_data.m_lineToData; }
        const ArcTo& arcTo() const { return m_data.m_arcToData; }
        const QuadCurveTo& quadCurveTo() const { return m_data.m_quadCurveToData; }
        const BezierCurveTo& bezierCurveTo() const { return m_data.m_bezierCurveToData; }
        const PathPoint& lastPoint() const
        {
            int n = numPoints();
            return n > 1 ? m_data.m_points[n - 1] : m_data.m_points[0];
        }
        const PathPoint& pointAt(int index) const { return m_data.m_points[index]; }
        int numPoints() const;
        int numControlPoints() const;
        void move(const FloatSize& offset);
        void transform(const AffineTransform& t);
        PathElementType type() const;
        PlaformPathElementType platformType() const { return m_type; }
        void inflateRectToContainMe(FloatRect& r, const FloatPoint& lastPoint) const;

    private:
        PlaformPathElementType m_type;
        union {
            MoveTo m_moveToData;
            LineTo m_lineToData;
            ArcTo m_arcToData;
            QuadCurveTo m_quadCurveToData;
            BezierCurveTo m_bezierCurveToData;
            PathPoint m_points[4];
        } m_data;
    };

    typedef Vector<PlatformPathElement> PlatformPathElements;

    class PlatformPath {
    public:
        PlatformPath();
        const PlatformPathElements& elements() const { return m_elements; }
        void append(const PlatformPathElement& e);
        void append(const PlatformPath& p);
        void clear();
        bool isEmpty() const { return m_elements.isEmpty(); }

        void strokePath(HDC, const AffineTransform* tr) const;
        void fillPath(HDC, const AffineTransform* tr) const;
        FloatPoint lastPoint() const { return m_elements.isEmpty() ? FloatPoint(0, 0) : m_elements.last().lastPoint(); }

        const FloatRect& boundingRect() const { return m_boundingRect; }
        bool contains(const FloatPoint& point, WindRule rule) const;
        void translate(const FloatSize& size);
        void transform(const AffineTransform& t);

        void moveTo(const FloatPoint&);
        void addLineTo(const FloatPoint&);
        void addQuadCurveTo(const FloatPoint& controlPoint, const FloatPoint& point);
        void addBezierCurveTo(const FloatPoint& controlPoint1, const FloatPoint& controlPoint2, const FloatPoint&);
        void addArcTo(const FloatPoint&, const FloatPoint&, float radius);
        void closeSubpath();
        void addEllipse(const FloatPoint& p, float a, float b, float sar, float ear, bool anticlockwise);
        void addRect(const FloatRect& r);
        void addEllipse(const FloatRect& r);
        void apply(void* info, PathApplierFunction function) const;

    private:
        void ensureSubpath();
        void addToSubpath(const PlatformPathElement& e);

        PlatformPathElements m_elements;
        FloatRect m_boundingRect;
        Vector<PathPolygon> m_subpaths;
        PathPoint m_currentPoint;
        bool m_penLifted;
    };

}

#endif // PlatformPathWinCE_h
