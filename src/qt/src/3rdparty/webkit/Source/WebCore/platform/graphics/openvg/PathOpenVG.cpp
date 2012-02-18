/*
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "Path.h"

#include "AffineTransform.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "NotImplemented.h"
#include "PainterOpenVG.h"
#include "PlatformPathOpenVG.h"
#include "PlatformString.h"
#include "StrokeStyleApplier.h"
#include "VGUtils.h"

#include <openvg.h>
#include <wtf/MathExtras.h>

#define WEBKIT_VG_PATH_CAPABILITIES VG_PATH_CAPABILITY_ALL

#define FUZZY_COMPARE(number, reference, delta) \
    (number >= (reference - delta) && number <= (reference + delta))

namespace WebCore {

PlatformPathOpenVG::PlatformPathOpenVG()
    : SharedResourceOpenVG()
{
    createPath();
}

PlatformPathOpenVG::PlatformPathOpenVG(const PlatformPathOpenVG& other)
    : SharedResourceOpenVG()
    , m_currentPoint(other.m_currentPoint)
    , m_subpathStartPoint(other.m_subpathStartPoint)
{
    createPath();
    // makeCompatibleContextCurrent() is called by createPath(), so not necessary here.
    vgAppendPath(m_vgPath, other.m_vgPath);
    ASSERT_VG_NO_ERROR();
}

PlatformPathOpenVG& PlatformPathOpenVG::operator=(const PlatformPathOpenVG& other)
{
    if (&other != this) {
        clear();
        // makeCompatibleContextCurrent() is called by clear(), so not necessary here.
        vgAppendPath(m_vgPath, other.m_vgPath);
        ASSERT_VG_NO_ERROR();
    }
    return *this;
}

PlatformPathOpenVG::~PlatformPathOpenVG()
{
    makeCompatibleContextCurrent();

    vgDestroyPath(m_vgPath);
    ASSERT_VG_NO_ERROR();
}

void PlatformPathOpenVG::clear()
{
    makeCompatibleContextCurrent();

    vgClearPath(m_vgPath, WEBKIT_VG_PATH_CAPABILITIES);
    ASSERT_VG_NO_ERROR();

    m_subpathStartPoint.setX(0);
    m_subpathStartPoint.setY(0);
    m_currentPoint = m_subpathStartPoint;
}

void PlatformPathOpenVG::createPath()
{
    makeSharedContextCurrent();

    m_vgPath = vgCreatePath(
        VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
        1.0 /* scale */, 0.0 /* bias */,
        0 /* expected number of segments */,
        0 /* expected number of total coordinates */,
        WEBKIT_VG_PATH_CAPABILITIES);
    ASSERT_VG_NO_ERROR();
}


Path::Path()
{
    m_path = new PlatformPathOpenVG();
}

Path::~Path()
{
    delete m_path;
}

Path::Path(const Path& other)
{
    m_path = new PlatformPathOpenVG(*(other.m_path));
}

Path& Path::operator=(const Path& other)
{
    *m_path = *(other.m_path);
    return *this;
}

FloatPoint Path::currentPoint() const 
{
    // FIXME: is this the way to return the current point of the subpath?
    return m_currentPoint;
}


bool Path::contains(const FloatPoint& point, WindRule rule) const
{
    notImplemented();

    // OpenVG has no path-contains function, so for now we approximate by
    // using the bounding rect of the path.
    return boundingRect().contains(point);
}

bool Path::strokeContains(StrokeStyleApplier* applier, const FloatPoint& point) const
{
    notImplemented();

    // OpenVG has no path-contains function, so for now we approximate by
    // using the stroke bounding rect of the path.
    return (const_cast<Path*>(this))->strokeBoundingRect().contains(point);
}

void Path::translate(const FloatSize& size)
{
    AffineTransform transformation;
    transformation.translate(size.width(), size.height());
    transform(transformation);
}

FloatRect Path::boundingRect() const
{
    VGfloat minX;
    VGfloat minY;
    VGfloat width;
    VGfloat height;

    m_path->makeCompatibleContextCurrent();
    vgPathBounds(m_path->vgPath(), &minX, &minY, &width, &height);
    ASSERT_VG_NO_ERROR();

    return FloatRect(FloatPoint(minX, minY), FloatSize(width, height));
}

FloatRect Path::strokeBoundingRect(StrokeStyleApplier* applier) const
{
    notImplemented();

    // vgPathBounds() ignores stroke parameters, and we don't currently have
    // an approximation that takes stroke parameters into account.
    return boundingRect();
}

void Path::moveTo(const FloatPoint& point)
{
    static const VGubyte pathSegments[] = { VG_MOVE_TO_ABS };
    const VGfloat pathData[] = { point.x(), point.y() };

    m_path->makeCompatibleContextCurrent();
    vgAppendPathData(m_path->vgPath(), 1, pathSegments, pathData);
    ASSERT_VG_NO_ERROR();

    m_path->m_currentPoint = m_path->m_subpathStartPoint = point;
}

void Path::addLineTo(const FloatPoint& point)
{
    static const VGubyte pathSegments[] = { VG_LINE_TO_ABS };
    const VGfloat pathData[] = { point.x(), point.y() };

    m_path->makeCompatibleContextCurrent();
    vgAppendPathData(m_path->vgPath(), 1, pathSegments, pathData);
    ASSERT_VG_NO_ERROR();

    m_path->m_currentPoint = point;
}

void Path::addQuadCurveTo(const FloatPoint& controlPoint, const FloatPoint& endPoint)
{
    static const VGubyte pathSegments[] = { VG_QUAD_TO_ABS };
    const VGfloat pathData[] = { controlPoint.x(), controlPoint.y(), endPoint.x(), endPoint.y() };

    m_path->makeCompatibleContextCurrent();
    vgAppendPathData(m_path->vgPath(), 1, pathSegments, pathData);
    ASSERT_VG_NO_ERROR();

    m_path->m_currentPoint = endPoint;
}

void Path::addBezierCurveTo(const FloatPoint& controlPoint1, const FloatPoint& controlPoint2, const FloatPoint& endPoint)
{
    static const VGubyte pathSegments[] = { VG_CUBIC_TO_ABS };
    const VGfloat pathData[] = { controlPoint1.x(), controlPoint1.y(), controlPoint2.x(), controlPoint2.y(), endPoint.x(), endPoint.y() };

    m_path->makeCompatibleContextCurrent();
    vgAppendPathData(m_path->vgPath(), 1, pathSegments, pathData);
    ASSERT_VG_NO_ERROR();

    m_path->m_currentPoint = endPoint;
}

void Path::addArcTo(const FloatPoint& point1, const FloatPoint& point2, float radius)
{
    // See http://philip.html5.org/tests/canvas/suite/tests/spec.html#arcto.

    const FloatPoint& point0 = m_path->m_currentPoint;
    if (!radius || point0 == point1 || point1 == point2) {
        addLineTo(point1);
        return;
    }

    FloatSize v01 = point0 - point1;
    FloatSize v21 = point2 - point1;

    // sin(A - B) = sin(A) * cos(B) - sin(B) * cos(A)
    double cross = v01.width() * v21.height() - v01.height() * v21.width();

    if (fabs(cross) < 1E-10) {
        // on one line
        addLineTo(point1);
        return;
    }

    double d01 = hypot(v01.width(), v01.height());
    double d21 = hypot(v21.width(), v21.height());
    double angle = (piDouble - fabs(asin(cross / (d01 * d21)))) * 0.5;
    double span = radius * tan(angle);
    double rate = span / d01;
    FloatPoint startPoint = FloatPoint(point1.x() + v01.width() * rate,
                                       point1.y() + v01.height() * rate);
    rate = span / d21;
    FloatPoint endPoint = FloatPoint(point1.x() + v21.width() * rate,
                                     point1.y() + v21.height() * rate);

    // Fa: large arc flag, makes the difference between SCWARC_TO and LCWARC_TO
    //     respectively SCCWARC_TO and LCCWARC_TO arcs. We always use small
    //     arcs for arcTo(), as the arc is defined as the "shortest arc" of the
    //     circle specified in HTML 5.

    // Fs: sweep flag, specifying whether the arc is drawn in increasing (true)
    //     or decreasing (0) direction.
    const bool anticlockwise = cross < 0;

    // Translate the large arc and sweep flags into an OpenVG segment command.
    const VGubyte segmentCommand = anticlockwise ? VG_SCCWARC_TO_ABS : VG_SCWARC_TO_ABS;

    const VGubyte pathSegments[] = {
        VG_LINE_TO_ABS,
        segmentCommand
    };
    const VGfloat pathData[] = {
        startPoint.x(), startPoint.y(),
        radius, radius, 0, endPoint.x(), endPoint.y()
    };

    m_path->makeCompatibleContextCurrent();
    vgAppendPathData(m_path->vgPath(), 2, pathSegments, pathData);
    ASSERT_VG_NO_ERROR();

    m_path->m_currentPoint = endPoint;
}

void Path::closeSubpath()
{
    static const VGubyte pathSegments[] = { VG_CLOSE_PATH };
    // pathData must not be 0, but certain compilers also don't create
    // zero-size arrays. So let's use a random aligned value (sizeof(VGfloat)),
    // it won't be accessed anyways as VG_CLOSE_PATH doesn't take coordinates.
    static const VGfloat* pathData = reinterpret_cast<VGfloat*>(sizeof(VGfloat));

    m_path->makeCompatibleContextCurrent();
    vgAppendPathData(m_path->vgPath(), 1, pathSegments, pathData);
    ASSERT_VG_NO_ERROR();

    m_path->m_currentPoint = m_path->m_subpathStartPoint;
}

void Path::addArc(const FloatPoint& center, float radius, float startAngle, float endAngle, bool anticlockwise)
{
    // The OpenVG spec says nothing about inf as radius or start/end angle.
    // WebKit seems to pass those (e.g. https://bugs.webkit.org/show_bug.cgi?id=16449),
    // so abort instead of risking undefined behavior.
    if (!isfinite(radius) || !isfinite(startAngle) || !isfinite(endAngle))
        return;

    // For some reason, the HTML 5 spec defines the angle as going clockwise
    // from the positive X axis instead of going standard anticlockwise.
    // So let's make it a proper angle in order to keep sanity.
    startAngle = fmod((2.0 * piDouble) - startAngle, 2.0 * piDouble);
    endAngle = fmod((2.0 * piDouble) - endAngle, 2.0 * piDouble);

    // Make it so that endAngle > startAngle. fmod() above takes care of
    // keeping the difference below 360 degrees.
    if (endAngle <= startAngle)
        endAngle += 2.0 * piDouble;

    const VGfloat angleDelta = anticlockwise
        ? (endAngle - startAngle)
        : (startAngle - endAngle + (2.0 * piDouble));

    // OpenVG uses endpoint parameterization while this method receives its
    // values in center parameterization. It lacks an ellipse rotation
    // parameter so we use 0 for that, and also the radius is only a single
    // value which makes for rh == rv. In order to convert from endpoint to
    // center parameterization, we use the formulas from the OpenVG/SVG specs:

    // (x,y) = (cos rot, -sin rot; sin rot, -cos rot) * (rh * cos angle, rv * sin angle) + (center.x, center.y)
    // rot is 0, which simplifies this a bit:
    // (x,y) = (1, 0; 0, -1) * (rh * cos angle, rv * sin angle) + (center.x, center.y)
    //       = (1 * rh * cos angle + 0 * rv * sin angle, 0 * rh * cos angle + -1 * rv * sin angle) + (center.x, center.y)
    //       = (rh * cos angle, -rv * sin angle) + (center.x, center.y)
    // (Set angle = {startAngle, endAngle} to retrieve the respective endpoints.)

    const VGfloat startX = radius * cos(startAngle) + center.x();
    const VGfloat startY = -radius * sin(startAngle) + center.y();
    const VGfloat endX = radius * cos(endAngle) + center.x();
    const VGfloat endY = -radius * sin(endAngle) + center.y();

    // Fa: large arc flag, makes the difference between SCWARC_TO and LCWARC_TO
    //     respectively SCCWARC_TO and LCCWARC_TO arcs.
    const bool largeArc = (angleDelta > piDouble);

    // Fs: sweep flag, specifying whether the arc is drawn in increasing (true)
    //     or decreasing (0) direction. No need to calculate this value, as it
    //     we already get it passed as a parameter (Fs == !anticlockwise).

    // Translate the large arc and sweep flags into an OpenVG segment command.
    // As OpenVG thinks of everything upside down, we need to reverse the
    // anticlockwise parameter in order to get the specified rotation.
    const VGubyte segmentCommand = !anticlockwise
        ? (largeArc ? VG_LCCWARC_TO_ABS : VG_SCCWARC_TO_ABS)
        : (largeArc ? VG_LCWARC_TO_ABS : VG_SCWARC_TO_ABS);

    // So now, we've got all the parameters in endpoint parameterization format
    // as OpenVG requires it. Which means we can just pass it like this.
    const VGubyte pathSegments[] = {
        hasCurrentPoint() ? VG_LINE_TO_ABS : VG_MOVE_TO_ABS,
        segmentCommand
    };
    const VGfloat pathData[] = {
        startX, startY,
        radius, radius, 0, endX, endY
    };

    m_path->makeCompatibleContextCurrent();
    vgAppendPathData(m_path->vgPath(), 2, pathSegments, pathData);
    ASSERT_VG_NO_ERROR();

    m_path->m_currentPoint.setX(endX);
    m_path->m_currentPoint.setY(endY);
}

void Path::addRect(const FloatRect& rect)
{
    static const VGubyte pathSegments[] = {
        VG_MOVE_TO_ABS,
        VG_HLINE_TO_REL,
        VG_VLINE_TO_REL,
        VG_HLINE_TO_REL,
        VG_CLOSE_PATH
    };
    const VGfloat pathData[] = {
        rect.x(), rect.y(),
        rect.width(),
        rect.height(),
        -rect.width()
    };

    m_path->makeCompatibleContextCurrent();
    vgAppendPathData(m_path->vgPath(), 5, pathSegments, pathData);
    ASSERT_VG_NO_ERROR();

    m_path->m_currentPoint = m_path->m_subpathStartPoint = rect.location();
}

void Path::addEllipse(const FloatRect& rect)
{
    static const VGubyte pathSegments[] = {
        VG_MOVE_TO_ABS,
        VG_SCCWARC_TO_REL,
        VG_SCCWARC_TO_REL,
        VG_CLOSE_PATH
    };
    const VGfloat pathData[] = {
        rect.x() + rect.width() / 2.0, rect.y(),
        rect.width() / 2.0, rect.height() / 2.0, 0, 0, rect.height(),
        rect.width() / 2.0, rect.height() / 2.0, 0, 0, -rect.height()
    };

    m_path->makeCompatibleContextCurrent();
    vgAppendPathData(m_path->vgPath(), 4, pathSegments, pathData);
    ASSERT_VG_NO_ERROR();
}

void Path::clear()
{
    m_path->clear();
}

bool Path::isEmpty() const
{
    m_path->makeCompatibleContextCurrent();
    return !vgGetParameteri(m_path->vgPath(), VG_PATH_NUM_SEGMENTS);
}

bool Path::hasCurrentPoint() const
{
    m_path->makeCompatibleContextCurrent();
    return vgGetParameteri(m_path->vgPath(), VG_PATH_NUM_SEGMENTS) > 0;
}

void Path::apply(void* info, PathApplierFunction function) const
{
    // OpenVG provides no means to retrieve path segment information.
    // This is *very* unfortunate, we might need to store the segments in
    // memory if we want to implement this function properly.
    // See http://www.khronos.org/message_boards/viewtopic.php?f=6&t=1887
    notImplemented();
}

void Path::transform(const AffineTransform& transformation)
{
    PlatformPathOpenVG* dst = new PlatformPathOpenVG();
    // dst->makeCompatibleContextCurrent() is called by the platform path
    // constructor, therefore not necessary to call it again here.
    PainterOpenVG::transformPath(dst->vgPath(), m_path->vgPath(), transformation);
    delete m_path;
    m_path = dst;

    m_path->m_currentPoint = transformation.mapPoint(m_path->m_currentPoint);
    m_path->m_subpathStartPoint = transformation.mapPoint(m_path->m_subpathStartPoint);
}


// Path::length(), Path::pointAtLength() and Path::normalAngleAtLength() are
// reimplemented here instead of in Path.cpp, because OpenVG has its own
// functions and Path::apply() doesn't really work as long as we rely on VGPath
// as primary path storage.

float Path::length() const
{
    m_path->makeCompatibleContextCurrent();
    VGfloat length = vgPathLength(m_path->vgPath(), 0, vgGetParameteri(m_path->vgPath(), VG_PATH_NUM_SEGMENTS));
    ASSERT_VG_NO_ERROR();
    return length;
}

FloatPoint Path::pointAtLength(float length, bool& ok) const
{
    VGfloat x = 0, y = 0;
    m_path->makeCompatibleContextCurrent();

    vgPointAlongPath(m_path->vgPath(), 0, vgGetParameteri(m_path->vgPath(), VG_PATH_NUM_SEGMENTS),
                     length, &x, &y, 0, 0);
    ok = (vgGetError() == VG_NO_ERROR);
    return FloatPoint(x, y);
}

float Path::normalAngleAtLength(float length, bool& ok) const
{
    VGfloat tangentX, tangentY;
    m_path->makeCompatibleContextCurrent();

    vgPointAlongPath(m_path->vgPath(), 0, vgGetParameteri(m_path->vgPath(), VG_PATH_NUM_SEGMENTS),
                     length, 0, 0, &tangentX, &tangentY);
    ok = (vgGetError() == VG_NO_ERROR);
    return atan2f(tangentY, tangentX) * 180.0 / piFloat; // convert to degrees
}

}
