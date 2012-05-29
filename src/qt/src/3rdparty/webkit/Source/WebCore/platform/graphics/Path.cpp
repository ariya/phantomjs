/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
 *                     2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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

#include "FloatPoint.h"
#include "FloatRect.h"
#include "PathTraversalState.h"
#include <math.h>
#include <wtf/MathExtras.h>

// Approximation of control point positions on a bezier to simulate a quarter of a circle.
static const float gCircleControlPoint = 0.448f;

namespace WebCore {

#if !PLATFORM(OPENVG) && !PLATFORM(QT)
static void pathLengthApplierFunction(void* info, const PathElement* element)
{
    PathTraversalState& traversalState = *static_cast<PathTraversalState*>(info);
    if (traversalState.m_success)
        return;
    traversalState.m_previous = traversalState.m_current;
    FloatPoint* points = element->points;
    float segmentLength = 0;
    switch (element->type) {
        case PathElementMoveToPoint:
            segmentLength = traversalState.moveTo(points[0]);
            break;
        case PathElementAddLineToPoint:
            segmentLength = traversalState.lineTo(points[0]);
            break;
        case PathElementAddQuadCurveToPoint:
            segmentLength = traversalState.quadraticBezierTo(points[0], points[1]);
            break;
        case PathElementAddCurveToPoint:
            segmentLength = traversalState.cubicBezierTo(points[0], points[1], points[2]);
            break;
        case PathElementCloseSubpath:
            segmentLength = traversalState.closeSubpath();
            break;
    }
    traversalState.m_totalLength += segmentLength; 
    if ((traversalState.m_action == PathTraversalState::TraversalPointAtLength || 
         traversalState.m_action == PathTraversalState::TraversalNormalAngleAtLength) &&
        (traversalState.m_totalLength >= traversalState.m_desiredLength)) {
        FloatSize change = traversalState.m_current - traversalState.m_previous;
        float slope = atan2f(change.height(), change.width());

        if (traversalState.m_action == PathTraversalState::TraversalPointAtLength) {
            float offset = traversalState.m_desiredLength - traversalState.m_totalLength;
            traversalState.m_current.move(offset * cosf(slope), offset * sinf(slope));
        } else
            traversalState.m_normalAngle = rad2deg(slope);

        traversalState.m_success = true;
    }
}

float Path::length() const
{
    PathTraversalState traversalState(PathTraversalState::TraversalTotalLength);
    apply(&traversalState, pathLengthApplierFunction);
    return traversalState.m_totalLength;
}

FloatPoint Path::pointAtLength(float length, bool& ok) const
{
    PathTraversalState traversalState(PathTraversalState::TraversalPointAtLength);
    traversalState.m_desiredLength = length;
    apply(&traversalState, pathLengthApplierFunction);
    ok = traversalState.m_success;
    return traversalState.m_current;
}

float Path::normalAngleAtLength(float length, bool& ok) const
{
    PathTraversalState traversalState(PathTraversalState::TraversalNormalAngleAtLength);
    traversalState.m_desiredLength = length ? length : std::numeric_limits<float>::epsilon();
    apply(&traversalState, pathLengthApplierFunction);
    ok = traversalState.m_success;
    return traversalState.m_normalAngle;
}
#endif

void Path::addRoundedRect(const FloatRect& rect, const FloatSize& roundingRadii)
{
    if (rect.isEmpty())
        return;

    FloatSize radius(roundingRadii);
    FloatSize halfSize(rect.width() / 2, rect.height() / 2);

    // If rx is greater than half of the width of the rectangle
    // then set rx to half of the width (required in SVG spec)
    if (radius.width() > halfSize.width())
        radius.setWidth(halfSize.width());

    // If ry is greater than half of the height of the rectangle
    // then set ry to half of the height (required in SVG spec)
    if (radius.height() > halfSize.height())
        radius.setHeight(halfSize.height());

    moveTo(FloatPoint(rect.x() + radius.width(), rect.y()));

    if (radius.width() < halfSize.width())
        addLineTo(FloatPoint(rect.x() + rect.width() - roundingRadii.width(), rect.y()));

    addBezierCurveTo(FloatPoint(rect.x() + rect.width() - radius.width() * gCircleControlPoint, rect.y()), FloatPoint(rect.x() + rect.width(), rect.y() + radius.height() * gCircleControlPoint), FloatPoint(rect.x() + rect.width(), rect.y() + radius.height()));

    if (radius.height() < halfSize.height())
        addLineTo(FloatPoint(rect.x() + rect.width(), rect.y() + rect.height() - radius.height()));

    addBezierCurveTo(FloatPoint(rect.x() + rect.width(), rect.y() + rect.height() - radius.height() * gCircleControlPoint), FloatPoint(rect.x() + rect.width() - radius.width() * gCircleControlPoint, rect.y() + rect.height()), FloatPoint(rect.x() + rect.width() - radius.width(), rect.y() + rect.height()));

    if (radius.width() < halfSize.width())
        addLineTo(FloatPoint(rect.x() + radius.width(), rect.y() + rect.height()));

    addBezierCurveTo(FloatPoint(rect.x() + radius.width() * gCircleControlPoint, rect.y() + rect.height()), FloatPoint(rect.x(), rect.y() + rect.height() - radius.height() * gCircleControlPoint), FloatPoint(rect.x(), rect.y() + rect.height() - radius.height()));

    if (radius.height() < halfSize.height())
        addLineTo(FloatPoint(rect.x(), rect.y() + radius.height()));

    addBezierCurveTo(FloatPoint(rect.x(), rect.y() + radius.height() * gCircleControlPoint), FloatPoint(rect.x() + radius.width() * gCircleControlPoint, rect.y()), FloatPoint(rect.x() + radius.width(), rect.y()));

    closeSubpath();
}

void Path::addRoundedRect(const FloatRect& rect, const FloatSize& topLeftRadius, const FloatSize& topRightRadius, const FloatSize& bottomLeftRadius, const FloatSize& bottomRightRadius)
{
    if (rect.isEmpty())
        return;

    if (rect.width() < topLeftRadius.width() + topRightRadius.width()
            || rect.width() < bottomLeftRadius.width() + bottomRightRadius.width()
            || rect.height() < topLeftRadius.height() + bottomLeftRadius.height()
            || rect.height() < topRightRadius.height() + bottomRightRadius.height()) {
        // If all the radii cannot be accommodated, return a rect.
        addRect(rect);
        return;
    }

    moveTo(FloatPoint(rect.x() + topLeftRadius.width(), rect.y()));

    addLineTo(FloatPoint(rect.x() + rect.width() - topRightRadius.width(), rect.y()));
    addBezierCurveTo(FloatPoint(rect.x() + rect.width() - topRightRadius.width() * gCircleControlPoint, rect.y()),
                     FloatPoint(rect.x() + rect.width(), rect.y() + topRightRadius.height() * gCircleControlPoint),
                     FloatPoint(rect.x() + rect.width(), rect.y() + topRightRadius.height()));
    addLineTo(FloatPoint(rect.x() + rect.width(), rect.y() + rect.height() - bottomRightRadius.height()));
    addBezierCurveTo(FloatPoint(rect.x() + rect.width(), rect.y() + rect.height() - bottomRightRadius.height() * gCircleControlPoint),
                     FloatPoint(rect.x() + rect.width() - bottomRightRadius.width() * gCircleControlPoint, rect.y() + rect.height()),
                     FloatPoint(rect.x() + rect.width() - bottomRightRadius.width(), rect.y() + rect.height()));
    addLineTo(FloatPoint(rect.x() + bottomLeftRadius.width(), rect.y() + rect.height()));
    addBezierCurveTo(FloatPoint(rect.x() + bottomLeftRadius.width() * gCircleControlPoint, rect.y() + rect.height()),
                     FloatPoint(rect.x(), rect.y() + rect.height() - bottomLeftRadius.height() * gCircleControlPoint),
                     FloatPoint(rect.x(), rect.y() + rect.height() - bottomLeftRadius.height()));
    addLineTo(FloatPoint(rect.x(), rect.y() + topLeftRadius.height()));
    addBezierCurveTo(FloatPoint(rect.x(), rect.y() + topLeftRadius.height() * gCircleControlPoint),
                     FloatPoint(rect.x() + topLeftRadius.width() * gCircleControlPoint, rect.y()),
                     FloatPoint(rect.x() + topLeftRadius.width(), rect.y()));

    closeSubpath();
}

void Path::addRoundedRect(const RoundedIntRect& r)
{
    addRoundedRect(r.rect(), r.radii().topLeft(), r.radii().topRight(), r.radii().bottomLeft(), r.radii().bottomRight());
}

}
