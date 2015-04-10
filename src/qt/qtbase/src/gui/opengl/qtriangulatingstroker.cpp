/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtriangulatingstroker_p.h"
#include <qmath.h>

QT_BEGIN_NAMESPACE

#define CURVE_FLATNESS Q_PI / 8




void QTriangulatingStroker::endCapOrJoinClosed(const qreal *start, const qreal *cur,
                                               bool implicitClose, bool endsAtStart)
{
    if (endsAtStart) {
        join(start + 2);
    } else if (implicitClose) {
        join(start);
        lineTo(start);
        join(start+2);
    } else {
        endCap(cur);
    }
    int count = m_vertices.size();

    // Copy the (x, y) values because QDataBuffer::add(const float& t)
    // may resize the buffer, which will leave t pointing at the
    // previous buffer's memory region if we don't copy first.
    float x = m_vertices.at(count-2);
    float y = m_vertices.at(count-1);
    m_vertices.add(x);
    m_vertices.add(y);
}

static inline void skipDuplicatePoints(const qreal **pts, const qreal *endPts)
{
    while ((*pts + 2) < endPts && float((*pts)[0]) == float((*pts)[2])
           && float((*pts)[1]) == float((*pts)[3]))
    {
        *pts += 2;
    }
}

void QTriangulatingStroker::process(const QVectorPath &path, const QPen &pen, const QRectF &, QPainter::RenderHints hints)
{
    const qreal *pts = path.points();
    const QPainterPath::ElementType *types = path.elements();
    int count = path.elementCount();
    if (count < 2)
        return;

    float realWidth = qpen_widthf(pen);
    if (realWidth == 0)
        realWidth = 1;

    m_width = realWidth / 2;

    bool cosmetic = qt_pen_is_cosmetic(pen, hints);
    if (cosmetic) {
        m_width = m_width * m_inv_scale;
    }

    m_join_style = qpen_joinStyle(pen);
    m_cap_style = qpen_capStyle(pen);
    m_vertices.reset();
    m_miter_limit = pen.miterLimit() * qpen_widthf(pen);

    // The curvyness is based on the notion that I originally wanted
    // roughly one line segment pr 4 pixels. This may seem little, but
    // because we sample at constantly incrementing B(t) E [0<t<1], we
    // will get longer segments where the curvature is small and smaller
    // segments when the curvature is high.
    //
    // To get a rough idea of the length of each curve, I pretend that
    // the curve is a 90 degree arc, whose radius is
    // qMax(curveBounds.width, curveBounds.height). Based on this
    // logic we can estimate the length of the outline edges based on
    // the radius + a pen width and adjusting for scale factors
    // depending on if the pen is cosmetic or not.
    //
    // The curvyness value of PI/14 was based on,
    // arcLength = 2*PI*r/4 = PI*r/2 and splitting length into somewhere
    // between 3 and 8 where 5 seemed to be give pretty good results
    // hence: Q_PI/14. Lower divisors will give more detail at the
    // direct cost of performance.

    // simplfy pens that are thin in device size (2px wide or less)
    if (realWidth < 2.5 && (cosmetic || m_inv_scale == 1)) {
        if (m_cap_style == Qt::RoundCap)
            m_cap_style = Qt::SquareCap;
        if (m_join_style == Qt::RoundJoin)
            m_join_style = Qt::MiterJoin;
        m_curvyness_add = 0.5;
        m_curvyness_mul = CURVE_FLATNESS / m_inv_scale;
        m_roundness = 1;
    } else if (cosmetic) {
        m_curvyness_add = realWidth / 2;
        m_curvyness_mul = CURVE_FLATNESS;
        m_roundness = qMax<int>(4, realWidth * CURVE_FLATNESS);
    } else {
        m_curvyness_add = m_width;
        m_curvyness_mul = CURVE_FLATNESS / m_inv_scale;
        m_roundness = qMax<int>(4, realWidth * m_curvyness_mul);
    }

    // Over this level of segmentation, there doesn't seem to be any
    // benefit, even for huge penWidth
    if (m_roundness > 24)
        m_roundness = 24;

    m_sin_theta = qFastSin(Q_PI / m_roundness);
    m_cos_theta = qFastCos(Q_PI / m_roundness);

    const qreal *endPts = pts + (count<<1);
    const qreal *startPts = 0;

    Qt::PenCapStyle cap = m_cap_style;

    if (!types) {
        skipDuplicatePoints(&pts, endPts);
        if ((pts + 2) == endPts)
            return;

        startPts = pts;

        bool endsAtStart = float(startPts[0]) == float(endPts[-2])
                && float(startPts[1]) == float(endPts[-1]);

        if (endsAtStart || path.hasImplicitClose())
            m_cap_style = Qt::FlatCap;
        moveTo(pts);
        m_cap_style = cap;
        pts += 2;
        skipDuplicatePoints(&pts, endPts);
        lineTo(pts);
        pts += 2;
        skipDuplicatePoints(&pts, endPts);
        while (pts < endPts) {
            join(pts);
            lineTo(pts);
            pts += 2;
            skipDuplicatePoints(&pts, endPts);
        }
        endCapOrJoinClosed(startPts, pts-2, path.hasImplicitClose(), endsAtStart);

    } else {
        bool endsAtStart = false;
        QPainterPath::ElementType previousType = QPainterPath::MoveToElement;
        const qreal *previousPts = pts;
        while (pts < endPts) {
            switch (*types) {
            case QPainterPath::MoveToElement: {
                if (previousType != QPainterPath::MoveToElement)
                    endCapOrJoinClosed(startPts, previousPts, path.hasImplicitClose(), endsAtStart);

                startPts = pts;
                skipDuplicatePoints(&startPts, endPts); // Skip duplicates to find correct normal.
                if (startPts + 2 >= endPts)
                    return; // Nothing to see here...

                int end = (endPts - pts) / 2;
                int i = 2; // Start looking to ahead since we never have two moveto's in a row
                while (i<end && types[i] != QPainterPath::MoveToElement) {
                    ++i;
                }
                endsAtStart = float(startPts[0]) == float(pts[i*2 - 2])
                        && float(startPts[1]) == float(pts[i*2 - 1]);
                if (endsAtStart || path.hasImplicitClose())
                    m_cap_style = Qt::FlatCap;

                moveTo(startPts);
                m_cap_style = cap;
                previousType = QPainterPath::MoveToElement;
                previousPts = pts;
                pts+=2;
                ++types;
                break; }
            case QPainterPath::LineToElement:
                if (float(m_cx) != float(pts[0]) || float(m_cy) != float(pts[1])) {
                    if (previousType != QPainterPath::MoveToElement)
                        join(pts);
                    lineTo(pts);
                    previousType = QPainterPath::LineToElement;
                    previousPts = pts;
                }
                pts+=2;
                ++types;
                break;
            case QPainterPath::CurveToElement:
                if (float(m_cx) != float(pts[0]) || float(m_cy) != float(pts[1])
                        || float(pts[0]) != float(pts[2]) || float(pts[1]) != float(pts[3])
                        || float(pts[2]) != float(pts[4]) || float(pts[3]) != float(pts[5]))
                {
                    if (float(m_cx) != float(pts[0]) || float(m_cy) != float(pts[1])) {
                        if (previousType != QPainterPath::MoveToElement)
                            join(pts);
                    }
                    cubicTo(pts);
                    previousType = QPainterPath::CurveToElement;
                    previousPts = pts + 4;
                }
                pts+=6;
                types+=3;
                break;
            default:
                Q_ASSERT(false);
                break;
            }
        }

        if (previousType != QPainterPath::MoveToElement)
            endCapOrJoinClosed(startPts, previousPts, path.hasImplicitClose(), endsAtStart);
    }
}

void QTriangulatingStroker::moveTo(const qreal *pts)
{
    m_cx = pts[0];
    m_cy = pts[1];

    float x2 = pts[2];
    float y2 = pts[3];
    normalVector(m_cx, m_cy, x2, y2, &m_nvx, &m_nvy);


    // To acheive jumps we insert zero-area tringles. This is done by
    // adding two identical points in both the end of previous strip
    // and beginning of next strip
    bool invisibleJump = m_vertices.size();

    switch (m_cap_style) {
    case Qt::FlatCap:
        if (invisibleJump) {
            m_vertices.add(m_cx + m_nvx);
            m_vertices.add(m_cy + m_nvy);
        }
        break;
    case Qt::SquareCap: {
        float sx = m_cx - m_nvy;
        float sy = m_cy + m_nvx;
        if (invisibleJump) {
            m_vertices.add(sx + m_nvx);
            m_vertices.add(sy + m_nvy);
        }
        emitLineSegment(sx, sy, m_nvx, m_nvy);
        break; }
    case Qt::RoundCap: {
        QVarLengthArray<float> points;
        arcPoints(m_cx, m_cy, m_cx + m_nvx, m_cy + m_nvy, m_cx - m_nvx, m_cy - m_nvy, points);
        m_vertices.resize(m_vertices.size() + points.size() + 2 * int(invisibleJump));
        int count = m_vertices.size();
        int front = 0;
        int end = points.size() / 2;
        while (front != end) {
            m_vertices.at(--count) = points[2 * end - 1];
            m_vertices.at(--count) = points[2 * end - 2];
            --end;
            if (front == end)
                break;
            m_vertices.at(--count) = points[2 * front + 1];
            m_vertices.at(--count) = points[2 * front + 0];
            ++front;
        }

        if (invisibleJump) {
            m_vertices.at(count - 1) = m_vertices.at(count + 1);
            m_vertices.at(count - 2) = m_vertices.at(count + 0);
        }
        break; }
    default: break; // ssssh gcc...
    }
    emitLineSegment(m_cx, m_cy, m_nvx, m_nvy);
}

void QTriangulatingStroker::cubicTo(const qreal *pts)
{
    const QPointF *p = (const QPointF *) pts;
    QBezier bezier = QBezier::fromPoints(*(p - 1), p[0], p[1], p[2]);

    QRectF bounds = bezier.bounds();
    float rad = qMax(bounds.width(), bounds.height());
    int threshold = qMin<float>(64, (rad + m_curvyness_add) * m_curvyness_mul);
    if (threshold < 4)
        threshold = 4;
    qreal threshold_minus_1 = threshold - 1;
    float vx, vy;

    float cx = m_cx, cy = m_cy;
    float x, y;

    for (int i=1; i<threshold; ++i) {
        qreal t = qreal(i) / threshold_minus_1;
        QPointF p = bezier.pointAt(t);
        x = p.x();
        y = p.y();

        normalVector(cx, cy, x, y, &vx, &vy);

        emitLineSegment(x, y, vx, vy);

        cx = x;
        cy = y;
    }

    m_cx = cx;
    m_cy = cy;

    m_nvx = vx;
    m_nvy = vy;
}

void QTriangulatingStroker::join(const qreal *pts)
{
    // Creates a join to the next segment (m_cx, m_cy) -> (pts[0], pts[1])
    normalVector(m_cx, m_cy, pts[0], pts[1], &m_nvx, &m_nvy);

    switch (m_join_style) {
    case Qt::BevelJoin:
        break;
    case Qt::SvgMiterJoin:
    case Qt::MiterJoin: {
        // Find out on which side the join should be.
        int count = m_vertices.size();
        float prevNvx = m_vertices.at(count - 2) - m_cx;
        float prevNvy = m_vertices.at(count - 1) - m_cy;
        float xprod = prevNvx * m_nvy - prevNvy * m_nvx;
        float px, py, qx, qy;

        // If the segments are parallel, use bevel join.
        if (qFuzzyIsNull(xprod))
            break;

        // Find the corners of the previous and next segment to join.
        if (xprod < 0) {
            px = m_vertices.at(count - 2);
            py = m_vertices.at(count - 1);
            qx = m_cx - m_nvx;
            qy = m_cy - m_nvy;
        } else {
            px = m_vertices.at(count - 4);
            py = m_vertices.at(count - 3);
            qx = m_cx + m_nvx;
            qy = m_cy + m_nvy;
        }

        // Find intersection point.
        float pu = px * prevNvx + py * prevNvy;
        float qv = qx * m_nvx + qy * m_nvy;
        float ix = (m_nvy * pu - prevNvy * qv) / xprod;
        float iy = (prevNvx * qv - m_nvx * pu) / xprod;

        // Check that the distance to the intersection point is less than the miter limit.
        if ((ix - px) * (ix - px) + (iy - py) * (iy - py) <= m_miter_limit * m_miter_limit) {
            m_vertices.add(ix);
            m_vertices.add(iy);
            m_vertices.add(ix);
            m_vertices.add(iy);
        }
        // else
        // Do a plain bevel join if the miter limit is exceeded or if
        // the lines are parallel. This is not what the raster
        // engine's stroker does, but it is both faster and similar to
        // what some other graphics API's do.

        break; }
    case Qt::RoundJoin: {
        QVarLengthArray<float> points;
        int count = m_vertices.size();
        float prevNvx = m_vertices.at(count - 2) - m_cx;
        float prevNvy = m_vertices.at(count - 1) - m_cy;
        if (m_nvx * prevNvy - m_nvy * prevNvx < 0) {
            arcPoints(0, 0, m_nvx, m_nvy, -prevNvx, -prevNvy, points);
            for (int i = points.size() / 2; i > 0; --i)
                emitLineSegment(m_cx, m_cy, points[2 * i - 2], points[2 * i - 1]);
        } else {
            arcPoints(0, 0, -prevNvx, -prevNvy, m_nvx, m_nvy, points);
            for (int i = 0; i < points.size() / 2; ++i)
                emitLineSegment(m_cx, m_cy, points[2 * i + 0], points[2 * i + 1]);
        }
        break; }
    default: break; // gcc warn--
    }

    emitLineSegment(m_cx, m_cy, m_nvx, m_nvy);
}

void QTriangulatingStroker::endCap(const qreal *)
{
    switch (m_cap_style) {
    case Qt::FlatCap:
        break;
    case Qt::SquareCap:
        emitLineSegment(m_cx + m_nvy, m_cy - m_nvx, m_nvx, m_nvy);
        break;
    case Qt::RoundCap: {
        QVarLengthArray<float> points;
        int count = m_vertices.size();
        arcPoints(m_cx, m_cy, m_vertices.at(count - 2), m_vertices.at(count - 1), m_vertices.at(count - 4), m_vertices.at(count - 3), points);
        int front = 0;
        int end = points.size() / 2;
        while (front != end) {
            m_vertices.add(points[2 * end - 2]);
            m_vertices.add(points[2 * end - 1]);
            --end;
            if (front == end)
                break;
            m_vertices.add(points[2 * front + 0]);
            m_vertices.add(points[2 * front + 1]);
            ++front;
        }
        break; }
    default: break; // to shut gcc up...
    }
}

void QTriangulatingStroker::arcPoints(float cx, float cy, float fromX, float fromY, float toX, float toY, QVarLengthArray<float> &points)
{
    float dx1 = fromX - cx;
    float dy1 = fromY - cy;
    float dx2 = toX - cx;
    float dy2 = toY - cy;

    // while more than 180 degrees left:
    while (dx1 * dy2 - dx2 * dy1 < 0) {
        float tmpx = dx1 * m_cos_theta - dy1 * m_sin_theta;
        float tmpy = dx1 * m_sin_theta + dy1 * m_cos_theta;
        dx1 = tmpx;
        dy1 = tmpy;
        points.append(cx + dx1);
        points.append(cy + dy1);
    }

    // while more than 90 degrees left:
    while (dx1 * dx2 + dy1 * dy2 < 0) {
        float tmpx = dx1 * m_cos_theta - dy1 * m_sin_theta;
        float tmpy = dx1 * m_sin_theta + dy1 * m_cos_theta;
        dx1 = tmpx;
        dy1 = tmpy;
        points.append(cx + dx1);
        points.append(cy + dy1);
    }

    // while more than 0 degrees left:
    while (dx1 * dy2 - dx2 * dy1 > 0) {
        float tmpx = dx1 * m_cos_theta - dy1 * m_sin_theta;
        float tmpy = dx1 * m_sin_theta + dy1 * m_cos_theta;
        dx1 = tmpx;
        dy1 = tmpy;
        points.append(cx + dx1);
        points.append(cy + dy1);
    }

    // remove last point which was rotated beyond [toX, toY].
    if (!points.isEmpty())
        points.resize(points.size() - 2);
}

static void qdashprocessor_moveTo(qreal x, qreal y, void *data)
{
    ((QDashedStrokeProcessor *) data)->addElement(QPainterPath::MoveToElement, x, y);
}

static void qdashprocessor_lineTo(qreal x, qreal y, void *data)
{
    ((QDashedStrokeProcessor *) data)->addElement(QPainterPath::LineToElement, x, y);
}

static void qdashprocessor_cubicTo(qreal, qreal, qreal, qreal, qreal, qreal, void *)
{
    Q_ASSERT(0); // The dasher should not produce curves...
}

QDashedStrokeProcessor::QDashedStrokeProcessor()
    : m_points(0), m_types(0),
      m_dash_stroker(0), m_inv_scale(1)
{
    m_dash_stroker.setMoveToHook(qdashprocessor_moveTo);
    m_dash_stroker.setLineToHook(qdashprocessor_lineTo);
    m_dash_stroker.setCubicToHook(qdashprocessor_cubicTo);
}

void QDashedStrokeProcessor::process(const QVectorPath &path, const QPen &pen, const QRectF &clip, QPainter::RenderHints hints)
{

    const qreal *pts = path.points();
    const QPainterPath::ElementType *types = path.elements();
    int count = path.elementCount();

    bool cosmetic = qt_pen_is_cosmetic(pen, hints);

    m_points.reset();
    m_types.reset();
    m_points.reserve(path.elementCount());
    m_types.reserve(path.elementCount());

    qreal width = qpen_widthf(pen);
    if (width == 0)
        width = 1;

    m_dash_stroker.setDashPattern(pen.dashPattern());
    m_dash_stroker.setStrokeWidth(cosmetic ? width * m_inv_scale : width);
    m_dash_stroker.setDashOffset(pen.dashOffset());
    m_dash_stroker.setMiterLimit(pen.miterLimit());
    m_dash_stroker.setClipRect(clip);

    float curvynessAdd, curvynessMul;

    // simplify pens that are thin in device size (2px wide or less)
    if (width < 2.5 && (cosmetic || m_inv_scale == 1)) {
        curvynessAdd = 0.5;
        curvynessMul = CURVE_FLATNESS / m_inv_scale;
    } else if (cosmetic) {
        curvynessAdd= width / 2;
        curvynessMul= CURVE_FLATNESS;
    } else {
        curvynessAdd = width * m_inv_scale;
        curvynessMul = CURVE_FLATNESS / m_inv_scale;
    }

    if (count < 2)
        return;

    const qreal *endPts = pts + (count<<1);

    m_dash_stroker.begin(this);

    if (!types) {
        m_dash_stroker.moveTo(pts[0], pts[1]);
        pts += 2;
        while (pts < endPts) {
            m_dash_stroker.lineTo(pts[0], pts[1]);
            pts += 2;
        }
    } else {
        while (pts < endPts) {
            switch (*types) {
            case QPainterPath::MoveToElement:
                m_dash_stroker.moveTo(pts[0], pts[1]);
                pts += 2;
                ++types;
                break;
            case QPainterPath::LineToElement:
                m_dash_stroker.lineTo(pts[0], pts[1]);
                pts += 2;
                ++types;
                break;
            case QPainterPath::CurveToElement: {
                QBezier b = QBezier::fromPoints(*(((const QPointF *) pts) - 1),
                                                *(((const QPointF *) pts)),
                                                *(((const QPointF *) pts) + 1),
                                                *(((const QPointF *) pts) + 2));
                QRectF bounds = b.bounds();
                float rad = qMax(bounds.width(), bounds.height());
                int threshold = qMin<float>(64, (rad + curvynessAdd) * curvynessMul);
                if (threshold < 4)
                    threshold = 4;

                qreal threshold_minus_1 = threshold - 1;
                for (int i=0; i<threshold; ++i) {
                    QPointF pt = b.pointAt(i / threshold_minus_1);
                    m_dash_stroker.lineTo(pt.x(), pt.y());
                }
                pts += 6;
                types += 3;
                break; }
            default: break;
            }
        }
    }

    m_dash_stroker.end();
}

QT_END_NAMESPACE

