/*
 * Copyright (C) 2006 Zack Rusin   <zack@kde.org>
 *               2006 Rob Buis     <buis@kde.org>
 *               2009, 2010 Dirk Schulze <krit@webkit.org>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Path.h"

#include "AffineTransform.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "PlatformString.h"
#include "StrokeStyleApplier.h"
#include <QPainterPath>
#include <QTransform>
#include <QString>
#include <wtf/MathExtras.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

Path::Path()
{
}

Path::~Path()
{
}

Path::Path(const Path& other)
    : m_path(other.m_path)
{
}

Path& Path::operator=(const Path& other)
{
    m_path = other.m_path;
    return *this;
}

static inline bool areCollinear(const QPointF& a, const QPointF& b, const QPointF& c)
{
    // Solved from comparing the slopes of a to b and b to c: (ay-by)/(ax-bx) == (cy-by)/(cx-bx)
    return qFuzzyCompare((c.y() - b.y()) * (a.x() - b.x()), (a.y() - b.y()) * (c.x() - b.x()));
}

static inline bool withinRange(qreal p, qreal a, qreal b)
{
    return (p >= a && p <= b) || (p >= b && p <= a);
}

// Check whether a point is on the border
static bool isPointOnPathBorder(const QPolygonF& border, const QPointF& p)
{
    // null border doesn't contain points
    if (border.isEmpty())
        return false;

    QPointF p1 = border.at(0);
    QPointF p2;

    for (int i = 1; i < border.size(); ++i) {
        p2 = border.at(i);
        if (areCollinear(p, p1, p2)
                // Once we know that the points are collinear we
                // only need to check one of the coordinates
                && (qAbs(p2.x() - p1.x()) > qAbs(p2.y() - p1.y()) ?
                        withinRange(p.x(), p1.x(), p2.x()) :
                        withinRange(p.y(), p1.y(), p2.y()))) {
            return true;
        }
        p1 = p2;
    }
    return false;
}

bool Path::contains(const FloatPoint& point, WindRule rule) const
{
    Qt::FillRule savedRule = m_path.fillRule();
    const_cast<QPainterPath*>(&m_path)->setFillRule(rule == RULE_EVENODD ? Qt::OddEvenFill : Qt::WindingFill);

    bool contains = m_path.contains(point);
    
    if (!contains) {
        // check whether the point is on the border
        contains = isPointOnPathBorder(m_path.toFillPolygon(), point);
    }

    const_cast<QPainterPath*>(&m_path)->setFillRule(savedRule);
    return contains;
}

static GraphicsContext* scratchContext()
{
    static QImage image(1, 1, QImage::Format_ARGB32_Premultiplied);
    static QPainter painter(&image);
    static GraphicsContext* context = new GraphicsContext(&painter);
    return context;
}

bool Path::strokeContains(StrokeStyleApplier* applier, const FloatPoint& point) const
{
    ASSERT(applier);

    QPainterPathStroker stroke;
    GraphicsContext* context = scratchContext();
    applier->strokeStyle(context);

    QPen pen = context->platformContext()->pen();
    stroke.setWidth(pen.widthF());
    stroke.setCapStyle(pen.capStyle());
    stroke.setJoinStyle(pen.joinStyle());
    stroke.setMiterLimit(pen.miterLimit());
    stroke.setDashPattern(pen.dashPattern());
    stroke.setDashOffset(pen.dashOffset());

    return stroke.createStroke(m_path).contains(point);
}

void Path::translate(const FloatSize& size)
{
    QTransform matrix;
    matrix.translate(size.width(), size.height());
    m_path = m_path * matrix;
}

FloatRect Path::boundingRect() const
{
    return m_path.boundingRect();
}

FloatRect Path::strokeBoundingRect(StrokeStyleApplier* applier) const
{
    GraphicsContext* context = scratchContext();
    QPainterPathStroker stroke;
    if (applier) {
        applier->strokeStyle(context);

        QPen pen = context->platformContext()->pen();
        stroke.setWidth(pen.widthF());
        stroke.setCapStyle(pen.capStyle());
        stroke.setJoinStyle(pen.joinStyle());
        stroke.setMiterLimit(pen.miterLimit());
        stroke.setDashPattern(pen.dashPattern());
        stroke.setDashOffset(pen.dashOffset());
    }
    return stroke.createStroke(m_path).boundingRect();
}

void Path::moveTo(const FloatPoint& point)
{
    m_path.moveTo(point);
}

void Path::addLineTo(const FloatPoint& p)
{
    m_path.lineTo(p);
}

void Path::addQuadCurveTo(const FloatPoint& cp, const FloatPoint& p)
{
    m_path.quadTo(cp, p);
}

void Path::addBezierCurveTo(const FloatPoint& cp1, const FloatPoint& cp2, const FloatPoint& p)
{
    m_path.cubicTo(cp1, cp2, p);
}

void Path::addArcTo(const FloatPoint& p1, const FloatPoint& p2, float radius)
{
    FloatPoint p0(m_path.currentPosition());

    FloatPoint p1p0((p0.x() - p1.x()), (p0.y() - p1.y()));
    FloatPoint p1p2((p2.x() - p1.x()), (p2.y() - p1.y()));
    float p1p0_length = sqrtf(p1p0.x() * p1p0.x() + p1p0.y() * p1p0.y());
    float p1p2_length = sqrtf(p1p2.x() * p1p2.x() + p1p2.y() * p1p2.y());

    double cos_phi = (p1p0.x() * p1p2.x() + p1p0.y() * p1p2.y()) / (p1p0_length * p1p2_length);

    // The points p0, p1, and p2 are on the same straight line (HTML5, 4.8.11.1.8)
    // We could have used areCollinear() here, but since we're reusing
    // the variables computed above later on we keep this logic.
    if (qFuzzyCompare(qAbs(cos_phi), 1.0)) {
        m_path.lineTo(p1);
        return;
    }

    float tangent = radius / tan(acos(cos_phi) / 2);
    float factor_p1p0 = tangent / p1p0_length;
    FloatPoint t_p1p0((p1.x() + factor_p1p0 * p1p0.x()), (p1.y() + factor_p1p0 * p1p0.y()));

    FloatPoint orth_p1p0(p1p0.y(), -p1p0.x());
    float orth_p1p0_length = sqrt(orth_p1p0.x() * orth_p1p0.x() + orth_p1p0.y() * orth_p1p0.y());
    float factor_ra = radius / orth_p1p0_length;

    // angle between orth_p1p0 and p1p2 to get the right vector orthographic to p1p0
    double cos_alpha = (orth_p1p0.x() * p1p2.x() + orth_p1p0.y() * p1p2.y()) / (orth_p1p0_length * p1p2_length);
    if (cos_alpha < 0.f)
        orth_p1p0 = FloatPoint(-orth_p1p0.x(), -orth_p1p0.y());

    FloatPoint p((t_p1p0.x() + factor_ra * orth_p1p0.x()), (t_p1p0.y() + factor_ra * orth_p1p0.y()));

    // calculate angles for addArc
    orth_p1p0 = FloatPoint(-orth_p1p0.x(), -orth_p1p0.y());
    float sa = acos(orth_p1p0.x() / orth_p1p0_length);
    if (orth_p1p0.y() < 0.f)
        sa = 2 * piDouble - sa;

    // anticlockwise logic
    bool anticlockwise = false;

    float factor_p1p2 = tangent / p1p2_length;
    FloatPoint t_p1p2((p1.x() + factor_p1p2 * p1p2.x()), (p1.y() + factor_p1p2 * p1p2.y()));
    FloatPoint orth_p1p2((t_p1p2.x() - p.x()), (t_p1p2.y() - p.y()));
    float orth_p1p2_length = sqrtf(orth_p1p2.x() * orth_p1p2.x() + orth_p1p2.y() * orth_p1p2.y());
    float ea = acos(orth_p1p2.x() / orth_p1p2_length);
    if (orth_p1p2.y() < 0)
        ea = 2 * piDouble - ea;
    if ((sa > ea) && ((sa - ea) < piDouble))
        anticlockwise = true;
    if ((sa < ea) && ((ea - sa) > piDouble))
        anticlockwise = true;

    m_path.lineTo(t_p1p0);

    addArc(p, radius, sa, ea, anticlockwise);
}

void Path::closeSubpath()
{
    m_path.closeSubpath();
}

void Path::addArc(const FloatPoint& p, float r, float sar, float ear, bool anticlockwise)
{
    qreal xc = p.x();
    qreal yc = p.y();
    qreal radius = r;


    //### HACK
    // In Qt we don't switch the coordinate system for degrees
    // and still use the 0,0 as bottom left for degrees so we need
    // to switch
    sar = -sar;
    ear = -ear;
    anticlockwise = !anticlockwise;
    //end hack

    float sa = rad2deg(sar);
    float ea = rad2deg(ear);

    double span = 0;

    double xs = xc - radius;
    double ys = yc - radius;
    double width  = radius*2;
    double height = radius*2;

    if ((!anticlockwise && (ea - sa >= 360)) || (anticlockwise && (sa - ea >= 360))) {
        // If the anticlockwise argument is false and endAngle-startAngle is equal to or greater than 2*PI, or, if the
        // anticlockwise argument is true and startAngle-endAngle is equal to or greater than 2*PI, then the arc is the whole
        // circumference of this circle.
        span = 360;

        if (anticlockwise)
            span = -span;
    } else {
        if (!anticlockwise && (ea < sa))
            span += 360;
        else if (anticlockwise && (sa < ea))
            span -= 360;

        // this is also due to switched coordinate system
        // we would end up with a 0 span instead of 360
        if (!(qFuzzyCompare(span + (ea - sa) + 1, 1.0)
            && qFuzzyCompare(qAbs(span), 360.0))) {
            // mod 360
            span += (ea - sa) - (static_cast<int>((ea - sa) / 360)) * 360;
        }
    }

    // If the path is empty, move to where the arc will start to avoid painting a line from (0,0)
    // NOTE: QPainterPath::isEmpty() won't work here since it ignores a lone MoveToElement
    if (!m_path.elementCount())
        m_path.arcMoveTo(xs, ys, width, height, sa);
    else if (!radius) {
        m_path.lineTo(xc, yc);
        return;
    }

    m_path.arcTo(xs, ys, width, height, sa, span);

}

void Path::addRect(const FloatRect& r)
{
    m_path.addRect(r.x(), r.y(), r.width(), r.height());
}

void Path::addEllipse(const FloatRect& r)
{
    m_path.addEllipse(r.x(), r.y(), r.width(), r.height());
}

void Path::clear()
{
    if (!m_path.elementCount())
        return;
    m_path = QPainterPath();
}

bool Path::isEmpty() const
{
    // Don't use QPainterPath::isEmpty(), as that also returns true if there's only
    // one initial MoveTo element in the path.
    return !m_path.elementCount();
}

bool Path::hasCurrentPoint() const
{
    return !isEmpty();
}

FloatPoint Path::currentPoint() const 
{
    return m_path.currentPosition();
}

void Path::apply(void* info, PathApplierFunction function) const
{
    PathElement pelement;
    FloatPoint points[3];
    pelement.points = points;
    for (int i = 0; i < m_path.elementCount(); ++i) {
        const QPainterPath::Element& cur = m_path.elementAt(i);

        switch (cur.type) {
            case QPainterPath::MoveToElement:
                pelement.type = PathElementMoveToPoint;
                pelement.points[0] = QPointF(cur);
                function(info, &pelement);
                break;
            case QPainterPath::LineToElement:
                pelement.type = PathElementAddLineToPoint;
                pelement.points[0] = QPointF(cur);
                function(info, &pelement);
                break;
            case QPainterPath::CurveToElement:
            {
                const QPainterPath::Element& c1 = m_path.elementAt(i + 1);
                const QPainterPath::Element& c2 = m_path.elementAt(i + 2);

                Q_ASSERT(c1.type == QPainterPath::CurveToDataElement);
                Q_ASSERT(c2.type == QPainterPath::CurveToDataElement);

                pelement.type = PathElementAddCurveToPoint;
                pelement.points[0] = QPointF(cur);
                pelement.points[1] = QPointF(c1);
                pelement.points[2] = QPointF(c2);
                function(info, &pelement);

                i += 2;
                break;
            }
            case QPainterPath::CurveToDataElement:
                Q_ASSERT(false);
        }
    }
}

void Path::transform(const AffineTransform& transform)
{
    QTransform qTransform(transform);
    m_path = qTransform.map(m_path);
}

float Path::length() const
{
    return m_path.length();
}

FloatPoint Path::pointAtLength(float length, bool& ok) const
{
    ok = (length >= 0 && length <= m_path.length());

    qreal percent = m_path.percentAtLength(length);
    QPointF point = m_path.pointAtPercent(percent);

    return point;
}

float Path::normalAngleAtLength(float length, bool& ok) const
{
    ok = (length >= 0 && length <= m_path.length());

    qreal percent = m_path.percentAtLength(length);
    qreal angle = m_path.angleAtPercent(percent);

    // Normalize angle value.
    // QPainterPath returns angle values with the origo being at the top left corner.
    // In case of moveTo(0, 0) and addLineTo(0, 10) the angle is 270,
    // while the caller expects it to be 90.
    // Normalize the value by mirroring it to the x-axis.
    // For more info look at pathLengthApplierFunction().
    if (angle > 0)
        angle = 360 - angle;
    return angle;
}

}

// vim: ts=4 sw=4 et
