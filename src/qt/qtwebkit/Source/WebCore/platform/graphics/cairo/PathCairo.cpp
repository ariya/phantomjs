/*
    Copyright (C) 2007 Krzysztof Kowalczyk <kkowalczyk@gmail.com>
    Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005, 2006 Rob Buis <buis@kde.org>
                  2005, 2007 Apple Inc. All Rights reserved.
                  2007 Alp Toker <alp@atoker.com>
                  2008 Dirk Schulze <krit@webkit.org>
                  2011 Igalia S.L.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    aint with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "Path.h"

#include "AffineTransform.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "OwnPtrCairo.h"
#include "PlatformPathCairo.h"
#include "StrokeStyleApplier.h"
#include <cairo.h>
#include <math.h>
#include <wtf/MathExtras.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

Path::Path()
    : m_path(0)
{
}

Path::~Path()
{
    if (m_path)
        delete m_path;
}

Path::Path(const Path& other)
    : m_path(0)
{
    if (other.isNull())
        return;

    cairo_t* cr = ensurePlatformPath()->context();
    OwnPtr<cairo_path_t> pathCopy = adoptPtr(cairo_copy_path(other.platformPath()->context()));
    cairo_append_path(cr, pathCopy.get());
}

PlatformPathPtr Path::ensurePlatformPath()
{
    if (!m_path)
        m_path = new CairoPath();
    return m_path;
}

Path& Path::operator=(const Path& other)
{
    if (&other == this)
        return *this;

    if (other.isNull()) {
        if (m_path) {
            delete m_path;
            m_path = 0;
        }
    } else {
        clear();
        cairo_t* cr = ensurePlatformPath()->context();
        OwnPtr<cairo_path_t> pathCopy = adoptPtr(cairo_copy_path(other.platformPath()->context()));
        cairo_append_path(cr, pathCopy.get());
    }

    return *this;
}

void Path::clear()
{
    if (isNull())
        return;

    cairo_t* cr = platformPath()->context();
    cairo_new_path(cr);
}

bool Path::isEmpty() const
{
    return isNull() || !cairo_has_current_point(platformPath()->context());
}

bool Path::hasCurrentPoint() const
{
    return !isEmpty();
}

FloatPoint Path::currentPoint() const 
{
    if (isNull())
        return FloatPoint();

    // FIXME: Is this the correct way?
    double x;
    double y;
    cairo_get_current_point(platformPath()->context(), &x, &y);
    return FloatPoint(x, y);
}

void Path::translate(const FloatSize& p)
{
    cairo_t* cr = ensurePlatformPath()->context();
    cairo_translate(cr, -p.width(), -p.height());
}

void Path::moveTo(const FloatPoint& p)
{
    cairo_t* cr = ensurePlatformPath()->context();
    cairo_move_to(cr, p.x(), p.y());
}

void Path::addLineTo(const FloatPoint& p)
{
    cairo_t* cr = ensurePlatformPath()->context();
    cairo_line_to(cr, p.x(), p.y());
}

void Path::addRect(const FloatRect& rect)
{
    cairo_t* cr = ensurePlatformPath()->context();
    cairo_rectangle(cr, rect.x(), rect.y(), rect.width(), rect.height());
}

/*
 * inspired by libsvg-cairo
 */
void Path::addQuadCurveTo(const FloatPoint& controlPoint, const FloatPoint& point)
{
    cairo_t* cr = ensurePlatformPath()->context();
    double x, y;
    double x1 = controlPoint.x();
    double y1 = controlPoint.y();
    double x2 = point.x();
    double y2 = point.y();
    cairo_get_current_point(cr, &x, &y);
    cairo_curve_to(cr,
                   x  + 2.0 / 3.0 * (x1 - x),  y  + 2.0 / 3.0 * (y1 - y),
                   x2 + 2.0 / 3.0 * (x1 - x2), y2 + 2.0 / 3.0 * (y1 - y2),
                   x2, y2);
}

void Path::addBezierCurveTo(const FloatPoint& controlPoint1, const FloatPoint& controlPoint2, const FloatPoint& controlPoint3)
{
    cairo_t* cr = ensurePlatformPath()->context();
    cairo_curve_to(cr, controlPoint1.x(), controlPoint1.y(),
                   controlPoint2.x(), controlPoint2.y(),
                   controlPoint3.x(), controlPoint3.y());
}

void Path::addArc(const FloatPoint& p, float r, float startAngle, float endAngle, bool anticlockwise)
{
    // http://bugs.webkit.org/show_bug.cgi?id=16449
    // cairo_arc() functions hang or crash when passed inf as radius or start/end angle
    if (!std::isfinite(r) || !std::isfinite(startAngle) || !std::isfinite(endAngle))
        return;

    cairo_t* cr = ensurePlatformPath()->context();
    float sweep = endAngle - startAngle;
    const float twoPI = 2 * piFloat;
    if ((sweep <= -twoPI || sweep >= twoPI)
        && ((anticlockwise && (endAngle < startAngle)) || (!anticlockwise && (startAngle < endAngle)))) {
        if (anticlockwise)
            cairo_arc_negative(cr, p.x(), p.y(), r, startAngle, startAngle - twoPI);
        else
            cairo_arc(cr, p.x(), p.y(), r, startAngle, startAngle + twoPI);
        cairo_new_sub_path(cr);
        cairo_arc(cr, p.x(), p.y(), r, endAngle, endAngle);
    } else {
        if (anticlockwise)
            cairo_arc_negative(cr, p.x(), p.y(), r, startAngle, endAngle);
        else
            cairo_arc(cr, p.x(), p.y(), r, startAngle, endAngle);
    }
}

static inline float areaOfTriangleFormedByPoints(const FloatPoint& p1, const FloatPoint& p2, const FloatPoint& p3)
{
    return p1.x() * (p2.y() - p3.y()) + p2.x() * (p3.y() - p1.y()) + p3.x() * (p1.y() - p2.y());
}

void Path::addArcTo(const FloatPoint& p1, const FloatPoint& p2, float radius)
{
    // FIXME: Why do we return if the path is empty? Can't a path start with an arc?
    if (isEmpty())
        return;

    cairo_t* cr = platformPath()->context();

    double x0, y0;
    cairo_get_current_point(cr, &x0, &y0);
    FloatPoint p0(x0, y0);

    // Draw only a straight line to p1 if any of the points are equal or the radius is zero
    // or the points are collinear (triangle that the points form has area of zero value).
    if ((p1.x() == p0.x() && p1.y() == p0.y()) || (p1.x() == p2.x() && p1.y() == p2.y()) || !radius
        || !areaOfTriangleFormedByPoints(p0, p1, p2)) {
        cairo_line_to(cr, p1.x(), p1.y());
        return;
    }

    FloatPoint p1p0((p0.x() - p1.x()),(p0.y() - p1.y()));
    FloatPoint p1p2((p2.x() - p1.x()),(p2.y() - p1.y()));
    float p1p0_length = sqrtf(p1p0.x() * p1p0.x() + p1p0.y() * p1p0.y());
    float p1p2_length = sqrtf(p1p2.x() * p1p2.x() + p1p2.y() * p1p2.y());

    double cos_phi = (p1p0.x() * p1p2.x() + p1p0.y() * p1p2.y()) / (p1p0_length * p1p2_length);
    // all points on a line logic
    if (cos_phi == -1) {
        cairo_line_to(cr, p1.x(), p1.y());
        return;
    }
    if (cos_phi == 1) {
        // add infinite far away point
        unsigned int max_length = 65535;
        double factor_max = max_length / p1p0_length;
        FloatPoint ep((p0.x() + factor_max * p1p0.x()), (p0.y() + factor_max * p1p0.y()));
        cairo_line_to(cr, ep.x(), ep.y());
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
    FloatPoint orth_p1p2((t_p1p2.x() - p.x()),(t_p1p2.y() - p.y()));
    float orth_p1p2_length = sqrtf(orth_p1p2.x() * orth_p1p2.x() + orth_p1p2.y() * orth_p1p2.y());
    float ea = acos(orth_p1p2.x() / orth_p1p2_length);
    if (orth_p1p2.y() < 0)
        ea = 2 * piDouble - ea;
    if ((sa > ea) && ((sa - ea) < piDouble))
        anticlockwise = true;
    if ((sa < ea) && ((ea - sa) > piDouble))
        anticlockwise = true;

    cairo_line_to(cr, t_p1p0.x(), t_p1p0.y());

    addArc(p, radius, sa, ea, anticlockwise);
}

void Path::addEllipse(const FloatRect& rect)
{
    cairo_t* cr = ensurePlatformPath()->context();
    cairo_save(cr);
    float yRadius = .5 * rect.height();
    float xRadius = .5 * rect.width();
    cairo_translate(cr, rect.x() + xRadius, rect.y() + yRadius);
    cairo_scale(cr, xRadius, yRadius);
    cairo_arc(cr, 0., 0., 1., 0., 2 * piDouble);
    cairo_restore(cr);
}

void Path::closeSubpath()
{
    cairo_t* cr = ensurePlatformPath()->context();
    cairo_close_path(cr);
}

FloatRect Path::boundingRect() const
{
    // Should this be isEmpty() or can an empty path have a non-zero origin?
    if (isNull())
        return FloatRect();

    cairo_t* cr = platformPath()->context();
    double x0, x1, y0, y1;
    cairo_path_extents(cr, &x0, &y0, &x1, &y1);
    return FloatRect(x0, y0, x1 - x0, y1 - y0);
}

FloatRect Path::strokeBoundingRect(StrokeStyleApplier* applier) const
{
    // Should this be isEmpty() or can an empty path have a non-zero origin?
    if (isNull())
        return FloatRect();

    cairo_t* cr = platformPath()->context();
    if (applier) {
        GraphicsContext gc(cr);
        applier->strokeStyle(&gc);
    }

    double x0, x1, y0, y1;
    cairo_stroke_extents(cr, &x0, &y0, &x1, &y1);
    return FloatRect(x0, y0, x1 - x0, y1 - y0);
}

bool Path::contains(const FloatPoint& point, WindRule rule) const
{
    if (isNull() || !std::isfinite(point.x()) || !std::isfinite(point.y()))
        return false;
    cairo_t* cr = platformPath()->context();
    cairo_fill_rule_t cur = cairo_get_fill_rule(cr);
    cairo_set_fill_rule(cr, rule == RULE_EVENODD ? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
    bool contains = cairo_in_fill(cr, point.x(), point.y());
    cairo_set_fill_rule(cr, cur);
    return contains;
}

bool Path::strokeContains(StrokeStyleApplier* applier, const FloatPoint& point) const
{
    if (isNull())
        return false;

    ASSERT(applier);
    cairo_t* cr = platformPath()->context();
    GraphicsContext gc(cr);
    applier->strokeStyle(&gc);

    return cairo_in_stroke(cr, point.x(), point.y());
}

void Path::apply(void* info, PathApplierFunction function) const
{
    if (isNull())
        return;

    cairo_t* cr = platformPath()->context();
    OwnPtr<cairo_path_t> pathCopy = adoptPtr(cairo_copy_path(cr));
    cairo_path_data_t* data;
    PathElement pelement;
    FloatPoint points[3];
    pelement.points = points;

    for (int i = 0; i < pathCopy->num_data; i += pathCopy->data[i].header.length) {
        data = &pathCopy->data[i];
        switch (data->header.type) {
        case CAIRO_PATH_MOVE_TO:
            pelement.type = PathElementMoveToPoint;
            pelement.points[0] = FloatPoint(data[1].point.x,data[1].point.y);
            function(info, &pelement);
            break;
        case CAIRO_PATH_LINE_TO:
            pelement.type = PathElementAddLineToPoint;
            pelement.points[0] = FloatPoint(data[1].point.x,data[1].point.y);
            function(info, &pelement);
            break;
        case CAIRO_PATH_CURVE_TO:
            pelement.type = PathElementAddCurveToPoint;
            pelement.points[0] = FloatPoint(data[1].point.x,data[1].point.y);
            pelement.points[1] = FloatPoint(data[2].point.x,data[2].point.y);
            pelement.points[2] = FloatPoint(data[3].point.x,data[3].point.y);
            function(info, &pelement);
            break;
        case CAIRO_PATH_CLOSE_PATH:
            pelement.type = PathElementCloseSubpath;
            function(info, &pelement);
            break;
        }
    }
}

void Path::transform(const AffineTransform& trans)
{
    cairo_t* cr = ensurePlatformPath()->context();
    cairo_matrix_t c_matrix = cairo_matrix_t(trans);
    cairo_matrix_invert(&c_matrix);
    cairo_transform(cr, &c_matrix);
}

} // namespace WebCore
