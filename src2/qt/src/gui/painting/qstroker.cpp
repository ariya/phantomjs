/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "private/qstroker_p.h"
#include "private/qbezier_p.h"
#include "private/qmath_p.h"
#include "qline.h"
#include "qtransform.h"
#include <qmath.h>

QT_BEGIN_NAMESPACE

// #define QPP_STROKE_DEBUG

class QSubpathForwardIterator
{
public:
    QSubpathForwardIterator(const QDataBuffer<QStrokerOps::Element> *path)
        : m_path(path), m_pos(0) { }
    inline int position() const { return m_pos; }
    inline bool hasNext() const { return m_pos < m_path->size(); }
    inline QStrokerOps::Element next() { Q_ASSERT(hasNext()); return m_path->at(m_pos++); }

private:
    const QDataBuffer<QStrokerOps::Element> *m_path;
    int m_pos;
};

class QSubpathBackwardIterator
{
public:
    QSubpathBackwardIterator(const QDataBuffer<QStrokerOps::Element> *path)
        : m_path(path), m_pos(path->size() - 1) { }

    inline int position() const { return m_pos; }

    inline bool hasNext() const { return m_pos >= 0; }

    inline QStrokerOps::Element next()
    {
        Q_ASSERT(hasNext());

        QStrokerOps::Element ce = m_path->at(m_pos);   // current element

        if (m_pos == m_path->size() - 1) {
            --m_pos;
            ce.type = QPainterPath::MoveToElement;
            return ce;
        }

        const QStrokerOps::Element &pe = m_path->at(m_pos + 1); // previous element

        switch (pe.type) {
        case QPainterPath::LineToElement:
            ce.type = QPainterPath::LineToElement;
            break;
        case QPainterPath::CurveToDataElement:
            // First control point?
            if (ce.type == QPainterPath::CurveToElement) {
                ce.type = QPainterPath::CurveToDataElement;
            } else { // Second control point then
                ce.type = QPainterPath::CurveToElement;
            }
            break;
        case QPainterPath::CurveToElement:
            ce.type = QPainterPath::CurveToDataElement;
            break;
        default:
            qWarning("QSubpathReverseIterator::next: Case %d unhandled", ce.type);
            break;
        }
        --m_pos;

        return ce;
    }

private:
    const QDataBuffer<QStrokerOps::Element> *m_path;
    int m_pos;
};

class QSubpathFlatIterator
{
public:
    QSubpathFlatIterator(const QDataBuffer<QStrokerOps::Element> *path, qreal threshold)
        : m_path(path), m_pos(0), m_curve_index(-1), m_curve_threshold(threshold) { }

    inline bool hasNext() const { return m_curve_index >= 0 || m_pos < m_path->size(); }

    QStrokerOps::Element next()
    {
        Q_ASSERT(hasNext());

        if (m_curve_index >= 0) {
            QStrokerOps::Element e = { QPainterPath::LineToElement,
                                       qt_real_to_fixed(m_curve.at(m_curve_index).x()),
                                       qt_real_to_fixed(m_curve.at(m_curve_index).y())
                                       };
            ++m_curve_index;
            if (m_curve_index >= m_curve.size())
                m_curve_index = -1;
            return e;
        }

        QStrokerOps::Element e = m_path->at(m_pos);
        if (e.isCurveTo()) {
            Q_ASSERT(m_pos > 0);
            Q_ASSERT(m_pos < m_path->size());

            m_curve = QBezier::fromPoints(QPointF(qt_fixed_to_real(m_path->at(m_pos-1).x),
                                                  qt_fixed_to_real(m_path->at(m_pos-1).y)),
                                          QPointF(qt_fixed_to_real(e.x),
                                                  qt_fixed_to_real(e.y)),
                                          QPointF(qt_fixed_to_real(m_path->at(m_pos+1).x),
                                                  qt_fixed_to_real(m_path->at(m_pos+1).y)),
                                          QPointF(qt_fixed_to_real(m_path->at(m_pos+2).x),
                                                  qt_fixed_to_real(m_path->at(m_pos+2).y))).toPolygon(m_curve_threshold);
            m_curve_index = 1;
            e.type = QPainterPath::LineToElement;
            e.x = m_curve.at(0).x();
            e.y = m_curve.at(0).y();
            m_pos += 2;
        }
        Q_ASSERT(e.isLineTo() || e.isMoveTo());
        ++m_pos;
        return e;
    }

private:
    const QDataBuffer<QStrokerOps::Element> *m_path;
    int m_pos;
    QPolygonF m_curve;
    int m_curve_index;
    qreal m_curve_threshold;
};

template <class Iterator> bool qt_stroke_side(Iterator *it, QStroker *stroker,
                                              bool capFirst, QLineF *startTangent);

/*******************************************************************************
 * QLineF::angle gives us the smalles angle between two lines. Here we
 * want to identify the line's angle direction on the unit circle.
 */
static inline qreal adapted_angle_on_x(const QLineF &line)
{
    qreal angle = line.angle(QLineF(0, 0, 1, 0));
    if (line.dy() > 0)
        angle = 360 - angle;
    return angle;
}

QStrokerOps::QStrokerOps()
    : m_elements(0)
    , m_curveThreshold(qt_real_to_fixed(0.25))
    , m_dashThreshold(qt_real_to_fixed(0.25))
    , m_customData(0)
    , m_moveTo(0)
    , m_lineTo(0)
    , m_cubicTo(0)
{
}

QStrokerOps::~QStrokerOps()
{
}

/*!
    Prepares the stroker. Call this function once before starting a
    stroke by calling moveTo, lineTo or cubicTo.

    The \a customData is passed back through that callback functions
    and can be used by the user to for instance maintain state
    information.
*/
void QStrokerOps::begin(void *customData)
{
    m_customData = customData;
    m_elements.reset();
}


/*!
    Finishes the stroke. Call this function once when an entire
    primitive has been stroked.
*/
void QStrokerOps::end()
{
    if (m_elements.size() > 1)
        processCurrentSubpath();
    m_customData = 0;
}

/*!
    Convenience function that decomposes \a path into begin(),
    moveTo(), lineTo(), curevTo() and end() calls.

    The \a customData parameter is used in the callback functions

    The \a matrix is used to transform the points before input to the
    stroker.

    \sa begin()
*/
void QStrokerOps::strokePath(const QPainterPath &path, void *customData, const QTransform &matrix)
{
    if (path.isEmpty())
        return;

    setCurveThresholdFromTransform(QTransform());
    begin(customData);
    int count = path.elementCount();
    if (matrix.isIdentity()) {
        for (int i=0; i<count; ++i) {
            const QPainterPath::Element &e = path.elementAt(i);
            switch (e.type) {
            case QPainterPath::MoveToElement:
                moveTo(qt_real_to_fixed(e.x), qt_real_to_fixed(e.y));
                break;
            case QPainterPath::LineToElement:
                lineTo(qt_real_to_fixed(e.x), qt_real_to_fixed(e.y));
                break;
            case QPainterPath::CurveToElement:
                {
                    const QPainterPath::Element &cp2 = path.elementAt(++i);
                    const QPainterPath::Element &ep = path.elementAt(++i);
                    cubicTo(qt_real_to_fixed(e.x), qt_real_to_fixed(e.y),
                            qt_real_to_fixed(cp2.x), qt_real_to_fixed(cp2.y),
                            qt_real_to_fixed(ep.x), qt_real_to_fixed(ep.y));
                }
                break;
            default:
                break;
            }
        }
    } else {
        for (int i=0; i<count; ++i) {
            const QPainterPath::Element &e = path.elementAt(i);
            QPointF pt = QPointF(e.x, e.y) * matrix;
            switch (e.type) {
            case QPainterPath::MoveToElement:
                moveTo(qt_real_to_fixed(pt.x()), qt_real_to_fixed(pt.y()));
                break;
            case QPainterPath::LineToElement:
                lineTo(qt_real_to_fixed(pt.x()), qt_real_to_fixed(pt.y()));
                break;
            case QPainterPath::CurveToElement:
                {
                    QPointF cp2 = ((QPointF) path.elementAt(++i)) * matrix;
                    QPointF ep = ((QPointF) path.elementAt(++i)) * matrix;
                    cubicTo(qt_real_to_fixed(pt.x()), qt_real_to_fixed(pt.y()),
                            qt_real_to_fixed(cp2.x()), qt_real_to_fixed(cp2.y()),
                            qt_real_to_fixed(ep.x()), qt_real_to_fixed(ep.y()));
                }
                break;
            default:
                break;
            }
        }
    }
    end();
}

/*!
    Convenience function for stroking a polygon of the \a pointCount
    first points in \a points. If \a implicit_close is set to true a
    line is implictly drawn between the first and last point in the
    polygon. Typically true for polygons and false for polylines.

    The \a matrix is used to transform the points before they enter the
    stroker.

    \sa begin()
*/

void QStrokerOps::strokePolygon(const QPointF *points, int pointCount, bool implicit_close,
                                void *data, const QTransform &matrix)
{
    if (!pointCount)
        return;

    setCurveThresholdFromTransform(QTransform());
    begin(data);
    if (matrix.isIdentity()) {
        moveTo(qt_real_to_fixed(points[0].x()), qt_real_to_fixed(points[0].y()));
        for (int i=1; i<pointCount; ++i)
            lineTo(qt_real_to_fixed(points[i].x()),
                   qt_real_to_fixed(points[i].y()));
        if (implicit_close)
            lineTo(qt_real_to_fixed(points[0].x()), qt_real_to_fixed(points[0].y()));
    } else {
        QPointF start = points[0] * matrix;
        moveTo(qt_real_to_fixed(start.x()), qt_real_to_fixed(start.y()));
        for (int i=1; i<pointCount; ++i) {
            QPointF pt = points[i] * matrix;
            lineTo(qt_real_to_fixed(pt.x()), qt_real_to_fixed(pt.y()));
        }
        if (implicit_close)
            lineTo(qt_real_to_fixed(start.x()), qt_real_to_fixed(start.y()));
    }
    end();
}

/*!
    Convenience function for stroking an ellipse with bounding rect \a
    rect. The \a matrix is used to transform the coordinates before
    they enter the stroker.
*/
void QStrokerOps::strokeEllipse(const QRectF &rect, void *data, const QTransform &matrix)
{
    int count = 0;
    QPointF pts[12];
    QPointF start = qt_curves_for_arc(rect, 0, -360, pts, &count);
    Q_ASSERT(count == 12); // a perfect circle..

    if (!matrix.isIdentity()) {
        start = start * matrix;
        for (int i=0; i<12; ++i) {
            pts[i] = pts[i] * matrix;
        }
    }

    setCurveThresholdFromTransform(QTransform());
    begin(data);
    moveTo(qt_real_to_fixed(start.x()), qt_real_to_fixed(start.y()));
    for (int i=0; i<12; i+=3) {
        cubicTo(qt_real_to_fixed(pts[i].x()), qt_real_to_fixed(pts[i].y()),
                qt_real_to_fixed(pts[i+1].x()), qt_real_to_fixed(pts[i+1].y()),
                qt_real_to_fixed(pts[i+2].x()), qt_real_to_fixed(pts[i+2].y()));
    }
    end();
}


QStroker::QStroker()
    : m_capStyle(SquareJoin), m_joinStyle(FlatJoin),
      m_back1X(0), m_back1Y(0),
      m_back2X(0), m_back2Y(0)
{
    m_strokeWidth = qt_real_to_fixed(1);
    m_miterLimit = qt_real_to_fixed(2);
}

QStroker::~QStroker()
{
}

Qt::PenCapStyle QStroker::capForJoinMode(LineJoinMode mode)
{
    if (mode == FlatJoin) return Qt::FlatCap;
    else if (mode == SquareJoin) return Qt::SquareCap;
    else return Qt::RoundCap;
}

QStroker::LineJoinMode QStroker::joinModeForCap(Qt::PenCapStyle style)
{
    if (style == Qt::FlatCap) return FlatJoin;
    else if (style == Qt::SquareCap) return SquareJoin;
    else return RoundCap;
}

Qt::PenJoinStyle QStroker::joinForJoinMode(LineJoinMode mode)
{
    if (mode == FlatJoin) return Qt::BevelJoin;
    else if (mode == MiterJoin) return Qt::MiterJoin;
    else if (mode == SvgMiterJoin) return Qt::SvgMiterJoin;
    else return Qt::RoundJoin;
}

QStroker::LineJoinMode QStroker::joinModeForJoin(Qt::PenJoinStyle joinStyle)
{
    if (joinStyle == Qt::BevelJoin) return FlatJoin;
    else if (joinStyle == Qt::MiterJoin) return MiterJoin;
    else if (joinStyle == Qt::SvgMiterJoin) return SvgMiterJoin;
    else return RoundJoin;
}


/*!
    This function is called to stroke the currently built up
    subpath. The subpath is cleared when the function completes.
*/
void QStroker::processCurrentSubpath()
{
    Q_ASSERT(!m_elements.isEmpty());
    Q_ASSERT(m_elements.first().type == QPainterPath::MoveToElement);
    Q_ASSERT(m_elements.size() > 1);

    QSubpathForwardIterator fwit(&m_elements);
    QSubpathBackwardIterator bwit(&m_elements);

    QLineF fwStartTangent, bwStartTangent;

    bool fwclosed = qt_stroke_side(&fwit, this, false, &fwStartTangent);
    bool bwclosed = qt_stroke_side(&bwit, this, !fwclosed, &bwStartTangent);

    if (!bwclosed)
        joinPoints(m_elements.at(0).x, m_elements.at(0).y, fwStartTangent, m_capStyle);
}


/*!
    \internal
*/
void QStroker::joinPoints(qfixed focal_x, qfixed focal_y, const QLineF &nextLine, LineJoinMode join)
{
#ifdef QPP_STROKE_DEBUG
    printf(" -----> joinPoints: around=(%.0f, %.0f), next_p1=(%.0f, %.f) next_p2=(%.0f, %.f)\n",
           qt_fixed_to_real(focal_x),
           qt_fixed_to_real(focal_y),
           nextLine.x1(), nextLine.y1(), nextLine.x2(), nextLine.y2());
#endif
    // points connected already, don't join

#if !defined (QFIXED_26_6) && !defined (Q_FIXED_32_32)
    if (qFuzzyCompare(m_back1X, nextLine.x1()) && qFuzzyCompare(m_back1Y, nextLine.y1()))
        return;
#else
    if (m_back1X == qt_real_to_fixed(nextLine.x1())
        && m_back1Y == qt_real_to_fixed(nextLine.y1())) {
        return;
    }
#endif

    if (join == FlatJoin) {
        QLineF prevLine(qt_fixed_to_real(m_back2X), qt_fixed_to_real(m_back2Y),
                        qt_fixed_to_real(m_back1X), qt_fixed_to_real(m_back1Y));
        QPointF isect;
        QLineF::IntersectType type = prevLine.intersect(nextLine, &isect);
        QLineF shortCut(prevLine.p2(), nextLine.p1());
        qreal angle = shortCut.angleTo(prevLine);
        if (type == QLineF::BoundedIntersection || (angle > 90 && !qFuzzyCompare(angle, (qreal)90))) {
            emitLineTo(focal_x, focal_y);
            emitLineTo(qt_real_to_fixed(nextLine.x1()), qt_real_to_fixed(nextLine.y1()));
            return;
        }
        emitLineTo(qt_real_to_fixed(nextLine.x1()),
                   qt_real_to_fixed(nextLine.y1()));

    } else {
        QLineF prevLine(qt_fixed_to_real(m_back2X), qt_fixed_to_real(m_back2Y),
                        qt_fixed_to_real(m_back1X), qt_fixed_to_real(m_back1Y));

        QPointF isect;
        QLineF::IntersectType type = prevLine.intersect(nextLine, &isect);

        if (join == MiterJoin) {
            qreal appliedMiterLimit = qt_fixed_to_real(m_strokeWidth * m_miterLimit);

            // If we are on the inside, do the short cut...
            QLineF shortCut(prevLine.p2(), nextLine.p1());
            qreal angle = shortCut.angleTo(prevLine);
            if (type == QLineF::BoundedIntersection || (angle > 90 && !qFuzzyCompare(angle, (qreal)90))) {
                emitLineTo(focal_x, focal_y);
                emitLineTo(qt_real_to_fixed(nextLine.x1()), qt_real_to_fixed(nextLine.y1()));
                return;
            }
            QLineF miterLine(QPointF(qt_fixed_to_real(m_back1X),
                                     qt_fixed_to_real(m_back1Y)), isect);
            if (type == QLineF::NoIntersection || miterLine.length() > appliedMiterLimit) {
                QLineF l1(prevLine);
                l1.setLength(appliedMiterLimit);
                l1.translate(prevLine.dx(), prevLine.dy());

                QLineF l2(nextLine);
                l2.setLength(appliedMiterLimit);
                l2.translate(-l2.dx(), -l2.dy());

                emitLineTo(qt_real_to_fixed(l1.x2()), qt_real_to_fixed(l1.y2()));
                emitLineTo(qt_real_to_fixed(l2.x1()), qt_real_to_fixed(l2.y1()));
                emitLineTo(qt_real_to_fixed(nextLine.x1()), qt_real_to_fixed(nextLine.y1()));
            } else {
                emitLineTo(qt_real_to_fixed(isect.x()), qt_real_to_fixed(isect.y()));
                emitLineTo(qt_real_to_fixed(nextLine.x1()), qt_real_to_fixed(nextLine.y1()));
            }

        } else if (join == SquareJoin) {
            qfixed offset = m_strokeWidth / 2;

            QLineF l1(prevLine);
            l1.translate(l1.dx(), l1.dy());
            l1.setLength(qt_fixed_to_real(offset));
            QLineF l2(nextLine.p2(), nextLine.p1());
            l2.translate(l2.dx(), l2.dy());
            l2.setLength(qt_fixed_to_real(offset));
            emitLineTo(qt_real_to_fixed(l1.x2()), qt_real_to_fixed(l1.y2()));
            emitLineTo(qt_real_to_fixed(l2.x2()), qt_real_to_fixed(l2.y2()));
            emitLineTo(qt_real_to_fixed(l2.x1()), qt_real_to_fixed(l2.y1()));

        } else if (join == RoundJoin) {
            qfixed offset = m_strokeWidth / 2;

            QLineF shortCut(prevLine.p2(), nextLine.p1());
            qreal angle = shortCut.angleTo(prevLine);
            if (type == QLineF::BoundedIntersection || (angle > 90 && !qFuzzyCompare(angle, (qreal)90))) {
                emitLineTo(focal_x, focal_y);
                emitLineTo(qt_real_to_fixed(nextLine.x1()), qt_real_to_fixed(nextLine.y1()));
                return;
            }
            qreal l1_on_x = adapted_angle_on_x(prevLine);
            qreal l2_on_x = adapted_angle_on_x(nextLine);

            qreal sweepLength = qAbs(l2_on_x - l1_on_x);

            int point_count;
            QPointF curves[15];

            QPointF curve_start =
                qt_curves_for_arc(QRectF(qt_fixed_to_real(focal_x - offset),
                                         qt_fixed_to_real(focal_y - offset),
                                         qt_fixed_to_real(offset * 2),
                                         qt_fixed_to_real(offset * 2)),
                                  l1_on_x + 90, -sweepLength,
                                  curves, &point_count);

//             // line to the beginning of the arc segment, (should not be needed).
//             emitLineTo(qt_real_to_fixed(curve_start.x()), qt_real_to_fixed(curve_start.y()));
            Q_UNUSED(curve_start);

            for (int i=0; i<point_count; i+=3) {
                emitCubicTo(qt_real_to_fixed(curves[i].x()),
                            qt_real_to_fixed(curves[i].y()),
                            qt_real_to_fixed(curves[i+1].x()),
                            qt_real_to_fixed(curves[i+1].y()),
                            qt_real_to_fixed(curves[i+2].x()),
                            qt_real_to_fixed(curves[i+2].y()));
            }

            // line to the end of the arc segment, (should also not be needed).
            emitLineTo(qt_real_to_fixed(nextLine.x1()), qt_real_to_fixed(nextLine.y1()));

        // Same as round join except we know its 180 degrees. Can also optimize this
        // later based on the addEllipse logic
        } else if (join == RoundCap) {
            qfixed offset = m_strokeWidth / 2;

            // first control line
            QLineF l1 = prevLine;
            l1.translate(l1.dx(), l1.dy());
            l1.setLength(QT_PATH_KAPPA * offset);

            // second control line, find through normal between prevLine and focal.
            QLineF l2(qt_fixed_to_real(focal_x), qt_fixed_to_real(focal_y),
                      prevLine.x2(), prevLine.y2());
            l2.translate(-l2.dy(), l2.dx());
            l2.setLength(QT_PATH_KAPPA * offset);

            emitCubicTo(qt_real_to_fixed(l1.x2()),
                        qt_real_to_fixed(l1.y2()),
                        qt_real_to_fixed(l2.x2()),
                        qt_real_to_fixed(l2.y2()),
                        qt_real_to_fixed(l2.x1()),
                        qt_real_to_fixed(l2.y1()));

            // move so that it matches
            l2 = QLineF(l2.x1(), l2.y1(), l2.x1()-l2.dx(), l2.y1()-l2.dy());

            // last line is parallel to l1 so just shift it down.
            l1.translate(nextLine.x1() - l1.x1(), nextLine.y1() - l1.y1());

            emitCubicTo(qt_real_to_fixed(l2.x2()),
                        qt_real_to_fixed(l2.y2()),
                        qt_real_to_fixed(l1.x2()),
                        qt_real_to_fixed(l1.y2()),
                        qt_real_to_fixed(l1.x1()),
                        qt_real_to_fixed(l1.y1()));
        } else if (join == SvgMiterJoin) {
            QLineF shortCut(prevLine.p2(), nextLine.p1());
            qreal angle = shortCut.angleTo(prevLine);
            if (type == QLineF::BoundedIntersection || (angle > 90 && !qFuzzyCompare(angle, (qreal)90))) {
                emitLineTo(focal_x, focal_y);
                emitLineTo(qt_real_to_fixed(nextLine.x1()), qt_real_to_fixed(nextLine.y1()));
                return;
            }
            QLineF miterLine(QPointF(qt_fixed_to_real(focal_x),
                                     qt_fixed_to_real(focal_y)), isect);
            if (type == QLineF::NoIntersection || miterLine.length() > qt_fixed_to_real(m_strokeWidth * m_miterLimit) / 2) {
                emitLineTo(qt_real_to_fixed(nextLine.x1()),
                           qt_real_to_fixed(nextLine.y1()));
            } else {
                emitLineTo(qt_real_to_fixed(isect.x()), qt_real_to_fixed(isect.y()));
                emitLineTo(qt_real_to_fixed(nextLine.x1()), qt_real_to_fixed(nextLine.y1()));
            }
        } else {
            Q_ASSERT(!"QStroker::joinPoints(), bad join style...");
        }
    }
}


/*
   Strokes a subpath side using the \a it as source. Results are put into
   \a stroke. The function returns true if the subpath side was closed.
   If \a capFirst is true, we will use capPoints instead of joinPoints to
   connect the first segment, other segments will be joined using joinPoints.
   This is to put capping in order...
*/
template <class Iterator> bool qt_stroke_side(Iterator *it,
                                              QStroker *stroker,
                                              bool capFirst,
                                              QLineF *startTangent)
{
    // Used in CurveToElement section below.
    const int MAX_OFFSET = 16;
    QBezier offsetCurves[MAX_OFFSET];

    Q_ASSERT(it->hasNext()); // The initaial move to
    QStrokerOps::Element first_element = it->next();
    Q_ASSERT(first_element.isMoveTo());

    qfixed2d start = first_element;

#ifdef QPP_STROKE_DEBUG
    qDebug(" -> (side) [%.2f, %.2f], startPos=%d",
           qt_fixed_to_real(start.x),
           qt_fixed_to_real(start.y));
#endif

    qfixed2d prev = start;

    bool first = true;

    qfixed offset = stroker->strokeWidth() / 2;

    while (it->hasNext()) {
        QStrokerOps::Element e = it->next();

        // LineToElement
        if (e.isLineTo()) {
#ifdef QPP_STROKE_DEBUG
            qDebug("\n ---> (side) lineto [%.2f, %.2f]", e.x, e.y);
#endif
            QLineF line(qt_fixed_to_real(prev.x), qt_fixed_to_real(prev.y),
                        qt_fixed_to_real(e.x), qt_fixed_to_real(e.y));
            if (line.p1() != line.p2()) {
                QLineF normal = line.normalVector();
                normal.setLength(offset);
                line.translate(normal.dx(), normal.dy());

                // If we are starting a new subpath, move to correct starting point.
                if (first) {
                    if (capFirst)
                        stroker->joinPoints(prev.x, prev.y, line, stroker->capStyleMode());
                    else
                        stroker->emitMoveTo(qt_real_to_fixed(line.x1()), qt_real_to_fixed(line.y1()));
                    *startTangent = line;
                    first = false;
                } else {
                    stroker->joinPoints(prev.x, prev.y, line, stroker->joinStyleMode());
                }

                // Add the stroke for this line.
                stroker->emitLineTo(qt_real_to_fixed(line.x2()),
                                    qt_real_to_fixed(line.y2()));
                prev = e;
            }

        // CurveToElement
        } else if (e.isCurveTo()) {
            QStrokerOps::Element cp2 = it->next(); // control point 2
            QStrokerOps::Element ep = it->next();  // end point

#ifdef QPP_STROKE_DEBUG
            qDebug("\n ---> (side) cubicTo [%.2f, %.2f]",
                   qt_fixed_to_real(ep.x),
                   qt_fixed_to_real(ep.y));
#endif

            QBezier bezier =
                QBezier::fromPoints(QPointF(qt_fixed_to_real(prev.x), qt_fixed_to_real(prev.y)),
                                    QPointF(qt_fixed_to_real(e.x), qt_fixed_to_real(e.y)),
                                    QPointF(qt_fixed_to_real(cp2.x), qt_fixed_to_real(cp2.y)),
                                    QPointF(qt_fixed_to_real(ep.x), qt_fixed_to_real(ep.y)));

            int count = bezier.shifted(offsetCurves,
                                       MAX_OFFSET,
                                       offset,
                                       stroker->curveThreshold());

            if (count) {
                // If we are starting a new subpath, move to correct starting point
                QLineF tangent = bezier.startTangent();
                tangent.translate(offsetCurves[0].pt1() - bezier.pt1());
                if (first) {
                    QPointF pt = offsetCurves[0].pt1();
                    if (capFirst) {
                        stroker->joinPoints(prev.x, prev.y,
                                            tangent,
                                            stroker->capStyleMode());
                    } else {
                        stroker->emitMoveTo(qt_real_to_fixed(pt.x()),
                                            qt_real_to_fixed(pt.y()));
                    }
                    *startTangent = tangent;
                    first = false;
                } else {
                    stroker->joinPoints(prev.x, prev.y,
                                        tangent,
                                        stroker->joinStyleMode());
                }

                // Add these beziers
                for (int i=0; i<count; ++i) {
                    QPointF cp1 = offsetCurves[i].pt2();
                    QPointF cp2 = offsetCurves[i].pt3();
                    QPointF ep = offsetCurves[i].pt4();
                    stroker->emitCubicTo(qt_real_to_fixed(cp1.x()), qt_real_to_fixed(cp1.y()),
                                         qt_real_to_fixed(cp2.x()), qt_real_to_fixed(cp2.y()),
                                         qt_real_to_fixed(ep.x()), qt_real_to_fixed(ep.y()));
                }
            }

            prev = ep;
        }
    }

    if (start == prev) {
        // closed subpath, join first and last point
#ifdef QPP_STROKE_DEBUG
        qDebug("\n ---> (side) closed subpath");
#endif
        // don't join empty subpaths
        if (!first)
            stroker->joinPoints(prev.x, prev.y, *startTangent, stroker->joinStyleMode());
        return true;
    } else {
#ifdef QPP_STROKE_DEBUG
        qDebug("\n ---> (side) open subpath");
#endif
        return false;
    }
}

/*!
    \internal

    For a given angle in the range [0 .. 90], finds the corresponding parameter t
    of the prototype cubic bezier arc segment
    b = fromPoints(QPointF(1, 0), QPointF(1, KAPPA), QPointF(KAPPA, 1), QPointF(0, 1));

    From the bezier equation:
    b.pointAt(t).x() = (1-t)^3 + t*(1-t)^2 + t^2*(1-t)*KAPPA
    b.pointAt(t).y() = t*(1-t)^2 * KAPPA + t^2*(1-t) + t^3

    Third degree coefficients:
    b.pointAt(t).x() = at^3 + bt^2 + ct + d
    where a = 2-3*KAPPA, b = 3*(KAPPA-1), c = 0, d = 1

    b.pointAt(t).y() = at^3 + bt^2 + ct + d
    where a = 3*KAPPA-2, b = 6*KAPPA+3, c = 3*KAPPA, d = 0

    Newton's method to find the zero of a function:
    given a function f(x) and initial guess x_0
    x_1 = f(x_0) / f'(x_0)
    x_2 = f(x_1) / f'(x_1)
    etc...
*/

qreal qt_t_for_arc_angle(qreal angle)
{
    if (qFuzzyIsNull(angle))
        return 0;

    if (qFuzzyCompare(angle, qreal(90)))
        return 1;

    qreal radians = Q_PI * angle / 180;
    qreal cosAngle = qCos(radians);
    qreal sinAngle = qSin(radians);

    // initial guess
    qreal tc = angle / 90;
    // do some iterations of newton's method to approximate cosAngle
    // finds the zero of the function b.pointAt(tc).x() - cosAngle
    tc -= ((((2-3*QT_PATH_KAPPA) * tc + 3*(QT_PATH_KAPPA-1)) * tc) * tc + 1 - cosAngle) // value
         / (((6-9*QT_PATH_KAPPA) * tc + 6*(QT_PATH_KAPPA-1)) * tc); // derivative
    tc -= ((((2-3*QT_PATH_KAPPA) * tc + 3*(QT_PATH_KAPPA-1)) * tc) * tc + 1 - cosAngle) // value
         / (((6-9*QT_PATH_KAPPA) * tc + 6*(QT_PATH_KAPPA-1)) * tc); // derivative

    // initial guess
    qreal ts = tc;
    // do some iterations of newton's method to approximate sinAngle
    // finds the zero of the function b.pointAt(tc).y() - sinAngle
    ts -= ((((3*QT_PATH_KAPPA-2) * ts -  6*QT_PATH_KAPPA + 3) * ts + 3*QT_PATH_KAPPA) * ts - sinAngle)
         / (((9*QT_PATH_KAPPA-6) * ts + 12*QT_PATH_KAPPA - 6) * ts + 3*QT_PATH_KAPPA);
    ts -= ((((3*QT_PATH_KAPPA-2) * ts -  6*QT_PATH_KAPPA + 3) * ts + 3*QT_PATH_KAPPA) * ts - sinAngle)
         / (((9*QT_PATH_KAPPA-6) * ts + 12*QT_PATH_KAPPA - 6) * ts + 3*QT_PATH_KAPPA);

    // use the average of the t that best approximates cosAngle
    // and the t that best approximates sinAngle
    qreal t = 0.5 * (tc + ts);

#if 0
    printf("angle: %f, t: %f\n", angle, t);
    qreal a, b, c, d;
    bezierCoefficients(t, a, b, c, d);
    printf("cosAngle: %.10f, value: %.10f\n", cosAngle, a + b + c * QT_PATH_KAPPA);
    printf("sinAngle: %.10f, value: %.10f\n", sinAngle, b * QT_PATH_KAPPA + c + d);
#endif

    return t;
}

Q_GUI_EXPORT void qt_find_ellipse_coords(const QRectF &r, qreal angle, qreal length,
                            QPointF* startPoint, QPointF *endPoint);

/*!
    \internal

    Creates a number of curves for a given arc definition. The arc is
    defined an arc along the ellipses that fits into \a rect starting
    at \a startAngle and an arc length of \a sweepLength.

    The function has three out parameters. The return value is the
    starting point of the arc. The \a curves array represents the list
    of cubicTo elements up to a maximum of \a point_count. There are of course
    3 points pr curve.
*/
QPointF qt_curves_for_arc(const QRectF &rect, qreal startAngle, qreal sweepLength,
                       QPointF *curves, int *point_count)
{
    Q_ASSERT(point_count);
    Q_ASSERT(curves);

    *point_count = 0;
    if (qt_is_nan(rect.x()) || qt_is_nan(rect.y()) || qt_is_nan(rect.width()) || qt_is_nan(rect.height())
        || qt_is_nan(startAngle) || qt_is_nan(sweepLength)) {
        qWarning("QPainterPath::arcTo: Adding arc where a parameter is NaN, results are undefined");
        return QPointF();
    }

    if (rect.isNull()) {
        return QPointF();
    }

    qreal x = rect.x();
    qreal y = rect.y();

    qreal w = rect.width();
    qreal w2 = rect.width() / 2;
    qreal w2k = w2 * QT_PATH_KAPPA;

    qreal h = rect.height();
    qreal h2 = rect.height() / 2;
    qreal h2k = h2 * QT_PATH_KAPPA;

    QPointF points[16] =
    {
        // start point
        QPointF(x + w, y + h2),

        // 0 -> 270 degrees
        QPointF(x + w, y + h2 + h2k),
        QPointF(x + w2 + w2k, y + h),
        QPointF(x + w2, y + h),

        // 270 -> 180 degrees
        QPointF(x + w2 - w2k, y + h),
        QPointF(x, y + h2 + h2k),
        QPointF(x, y + h2),

        // 180 -> 90 degrees
        QPointF(x, y + h2 - h2k),
        QPointF(x + w2 - w2k, y),
        QPointF(x + w2, y),

        // 90 -> 0 degrees
        QPointF(x + w2 + w2k, y),
        QPointF(x + w, y + h2 - h2k),
        QPointF(x + w, y + h2)
    };

    if (sweepLength > 360) sweepLength = 360;
    else if (sweepLength < -360) sweepLength = -360;

    // Special case fast paths
    if (startAngle == 0.0) {
        if (sweepLength == 360.0) {
            for (int i = 11; i >= 0; --i)
                curves[(*point_count)++] = points[i];
            return points[12];
        } else if (sweepLength == -360.0) {
            for (int i = 1; i <= 12; ++i)
                curves[(*point_count)++] = points[i];
            return points[0];
        }
    }

    int startSegment = int(qFloor(startAngle / 90));
    int endSegment = int(qFloor((startAngle + sweepLength) / 90));

    qreal startT = (startAngle - startSegment * 90) / 90;
    qreal endT = (startAngle + sweepLength - endSegment * 90) / 90;

    int delta = sweepLength > 0 ? 1 : -1;
    if (delta < 0) {
        startT = 1 - startT;
        endT = 1 - endT;
    }

    // avoid empty start segment
    if (qFuzzyIsNull(startT - qreal(1))) {
        startT = 0;
        startSegment += delta;
    }

    // avoid empty end segment
    if (qFuzzyIsNull(endT)) {
        endT = 1;
        endSegment -= delta;
    }

    startT = qt_t_for_arc_angle(startT * 90);
    endT = qt_t_for_arc_angle(endT * 90);

    const bool splitAtStart = !qFuzzyIsNull(startT);
    const bool splitAtEnd = !qFuzzyIsNull(endT - qreal(1));

    const int end = endSegment + delta;

    // empty arc?
    if (startSegment == end) {
        const int quadrant = 3 - ((startSegment % 4) + 4) % 4;
        const int j = 3 * quadrant;
        return delta > 0 ? points[j + 3] : points[j];
    }

    QPointF startPoint, endPoint;
    qt_find_ellipse_coords(rect, startAngle, sweepLength, &startPoint, &endPoint);

    for (int i = startSegment; i != end; i += delta) {
        const int quadrant = 3 - ((i % 4) + 4) % 4;
        const int j = 3 * quadrant;

        QBezier b;
        if (delta > 0)
            b = QBezier::fromPoints(points[j + 3], points[j + 2], points[j + 1], points[j]);
        else
            b = QBezier::fromPoints(points[j], points[j + 1], points[j + 2], points[j + 3]);

        // empty arc?
        if (startSegment == endSegment && qFuzzyCompare(startT, endT))
            return startPoint;

        if (i == startSegment) {
            if (i == endSegment && splitAtEnd)
                b = b.bezierOnInterval(startT, endT);
            else if (splitAtStart)
                b = b.bezierOnInterval(startT, 1);
        } else if (i == endSegment && splitAtEnd) {
            b = b.bezierOnInterval(0, endT);
        }

        // push control points
        curves[(*point_count)++] = b.pt2();
        curves[(*point_count)++] = b.pt3();
        curves[(*point_count)++] = b.pt4();
    }

    Q_ASSERT(*point_count > 0);
    curves[*(point_count)-1] = endPoint;

    return startPoint;
}


static inline void qdashstroker_moveTo(qfixed x, qfixed y, void *data) {
    ((QStroker *) data)->moveTo(x, y);
}

static inline void qdashstroker_lineTo(qfixed x, qfixed y, void *data) {
    ((QStroker *) data)->lineTo(x, y);
}

static inline void qdashstroker_cubicTo(qfixed, qfixed, qfixed, qfixed, qfixed, qfixed, void *) {
    Q_ASSERT(0);
//     ((QStroker *) data)->cubicTo(c1x, c1y, c2x, c2y, ex, ey);
}


/*******************************************************************************
 * QDashStroker members
 */
QDashStroker::QDashStroker(QStroker *stroker)
    : m_stroker(stroker), m_dashOffset(0), m_stroke_width(1), m_miter_limit(1)
{
    if (m_stroker) {
        setMoveToHook(qdashstroker_moveTo);
        setLineToHook(qdashstroker_lineTo);
        setCubicToHook(qdashstroker_cubicTo);
    }
}

QVector<qfixed> QDashStroker::patternForStyle(Qt::PenStyle style)
{
    const qfixed space = 2;
    const qfixed dot = 1;
    const qfixed dash = 4;

    QVector<qfixed> pattern;

    switch (style) {
    case Qt::DashLine:
        pattern << dash << space;
        break;
    case Qt::DotLine:
        pattern << dot << space;
        break;
    case Qt::DashDotLine:
        pattern << dash << space << dot << space;
        break;
    case Qt::DashDotDotLine:
        pattern << dash << space << dot << space << dot << space;
        break;
    default:
        break;
    }

    return pattern;
}

static inline bool lineRectIntersectsRect(qfixed2d p1, qfixed2d p2, const qfixed2d &tl, const qfixed2d &br)
{
    return ((p1.x > tl.x || p2.x > tl.x) && (p1.x < br.x || p2.x < br.x)
        && (p1.y > tl.y || p2.y > tl.y) && (p1.y < br.y || p2.y < br.y));
}

// If the line intersects the rectangle, this function will return true.
static bool lineIntersectsRect(qfixed2d p1, qfixed2d p2, const qfixed2d &tl, const qfixed2d &br)
{
    if (!lineRectIntersectsRect(p1, p2, tl, br))
        return false;
    if (p1.x == p2.x || p1.y == p2.y)
        return true;

    if (p1.y > p2.y)
        qSwap(p1, p2); // make p1 above p2
    qfixed2d u;
    qfixed2d v;
    qfixed2d w = {p2.x - p1.x, p2.y - p1.y};
    if (p1.x < p2.x) {
        // backslash
        u.x = tl.x - p1.x; u.y = br.y - p1.y;
        v.x = br.x - p1.x; v.y = tl.y - p1.y;
    } else {
        // slash
        u.x = tl.x - p1.x; u.y = tl.y - p1.y;
        v.x = br.x - p1.x; v.y = br.y - p1.y;
    }
#if defined(QFIXED_IS_26_6) || defined(QFIXED_IS_16_16)
    qint64 val1 = qint64(u.x) * qint64(w.y) - qint64(u.y) * qint64(w.x);
    qint64 val2 = qint64(v.x) * qint64(w.y) - qint64(v.y) * qint64(w.x);
    return (val1 < 0 && val2 > 0) || (val1 > 0 && val2 < 0);
#elif defined(QFIXED_IS_32_32)
    // Cannot do proper test because it may overflow.
    return true;
#else
    qreal val1 = u.x * w.y - u.y * w.x;
    qreal val2 = v.x * w.y - v.y * w.x;
    return (val1 < 0 && val2 > 0) || (val1 > 0 && val2 < 0);
#endif
}

void QDashStroker::processCurrentSubpath()
{
    int dashCount = qMin(m_dashPattern.size(), 32);
    qfixed dashes[32];

    if (m_stroker) {
        m_customData = m_stroker;
        m_stroke_width = m_stroker->strokeWidth();
        m_miter_limit = m_stroker->miterLimit();
    }

    qreal longestLength = 0;
    qreal sumLength = 0;
    for (int i=0; i<dashCount; ++i) {
        dashes[i] = qMax(m_dashPattern.at(i), qreal(0)) * m_stroke_width;
        sumLength += dashes[i];
        if (dashes[i] > longestLength)
            longestLength = dashes[i];
    }

    if (qFuzzyIsNull(sumLength))
        return;

    qreal invSumLength = qreal(1) / sumLength;

    Q_ASSERT(dashCount > 0);

    dashCount = dashCount & -2; // Round down to even number

    int idash = 0; // Index to current dash
    qreal pos = 0; // The position on the curve, 0 <= pos <= path.length
    qreal elen = 0; // element length
    qreal doffset = m_dashOffset * m_stroke_width;

    // make sure doffset is in range [0..sumLength)
    doffset -= qFloor(doffset * invSumLength) * sumLength;

    while (doffset >= dashes[idash]) {
        doffset -= dashes[idash];
        if (++idash >= dashCount)
            idash = 0;
    }

    qreal estart = 0; // The elements starting position
    qreal estop = 0; // The element stop position

    QLineF cline;

    QPainterPath dashPath;

    QSubpathFlatIterator it(&m_elements, m_dashThreshold);
    qfixed2d prev = it.next();

    bool clipping = !m_clip_rect.isEmpty();
    qfixed2d move_to_pos = prev;
    qfixed2d line_to_pos;

    // Pad to avoid clipping the borders of thick pens.
    qfixed padding = qt_real_to_fixed(qMax(m_stroke_width, m_miter_limit) * longestLength);
    qfixed2d clip_tl = { qt_real_to_fixed(m_clip_rect.left()) - padding,
                         qt_real_to_fixed(m_clip_rect.top()) - padding };
    qfixed2d clip_br = { qt_real_to_fixed(m_clip_rect.right()) + padding ,
                         qt_real_to_fixed(m_clip_rect.bottom()) + padding };

    bool hasMoveTo = false;
    while (it.hasNext()) {
        QStrokerOps::Element e = it.next();

        Q_ASSERT(e.isLineTo());
        cline = QLineF(qt_fixed_to_real(prev.x),
                       qt_fixed_to_real(prev.y),
                       qt_fixed_to_real(e.x),
                       qt_fixed_to_real(e.y));
        elen = cline.length();

        estop = estart + elen;

        bool done = pos >= estop;

        if (clipping) {
            // Check if the entire line can be clipped away.
            if (!lineIntersectsRect(prev, e, clip_tl, clip_br)) {
                // Cut away full dash sequences.
                elen -= qFloor(elen * invSumLength) * sumLength;
                // Update dash offset.
                while (!done) {
                    qreal dpos = pos + dashes[idash] - doffset - estart;

                    Q_ASSERT(dpos >= 0);

                    if (dpos > elen) { // dash extends this line
                        doffset = dashes[idash] - (dpos - elen); // subtract the part already used
                        pos = estop; // move pos to next path element
                        done = true;
                    } else { // Dash is on this line
                        pos = dpos + estart;
                        done = pos >= estop;
                        if (++idash >= dashCount)
                            idash = 0;
                        doffset = 0; // full segment so no offset on next.
                    }
                }
                hasMoveTo = false;
                move_to_pos = e;
            }
        }

        // Dash away...
        while (!done) {
            QPointF p2;

            bool has_offset = doffset > 0;
            bool evenDash = (idash & 1) == 0;
            qreal dpos = pos + dashes[idash] - doffset - estart;

            Q_ASSERT(dpos >= 0);

            if (dpos > elen) { // dash extends this line
                doffset = dashes[idash] - (dpos - elen); // subtract the part already used
                pos = estop; // move pos to next path element
                done = true;
                p2 = cline.p2();
            } else { // Dash is on this line
                p2 = cline.pointAt(dpos/elen);
                pos = dpos + estart;
                done = pos >= estop;
                if (++idash >= dashCount)
                    idash = 0;
                doffset = 0; // full segment so no offset on next.
            }

            if (evenDash) {
                line_to_pos.x = qt_real_to_fixed(p2.x());
                line_to_pos.y = qt_real_to_fixed(p2.y());

                if (!clipping
                    || lineRectIntersectsRect(move_to_pos, line_to_pos, clip_tl, clip_br))
                {
                    // If we have an offset, we're continuing a dash
                    // from a previous element and should only
                    // continue the current dash, without starting a
                    // new subpath.
                    if (!has_offset || !hasMoveTo) {
                        emitMoveTo(move_to_pos.x, move_to_pos.y);
                        hasMoveTo = true;
                    }

                    emitLineTo(line_to_pos.x, line_to_pos.y);
                } else {
                    hasMoveTo = false;
                }
                move_to_pos = line_to_pos;
            } else {
                move_to_pos.x = qt_real_to_fixed(p2.x());
                move_to_pos.y = qt_real_to_fixed(p2.y());
            }
        }

        // Shuffle to the next cycle...
        estart = estop;
        prev = e;
    }

}

QT_END_NAMESPACE
