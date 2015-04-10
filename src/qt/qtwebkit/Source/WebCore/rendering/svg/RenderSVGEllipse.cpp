/*
 * Copyright (C) 2012 Google, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(SVG)
#include "RenderSVGEllipse.h"

#include "SVGCircleElement.h"
#include "SVGEllipseElement.h"
#include "SVGGraphicsElement.h"
#include "SVGNames.h"

namespace WebCore {

RenderSVGEllipse::RenderSVGEllipse(SVGGraphicsElement* node)
    : RenderSVGShape(node)
    , m_usePathFallback(false)
{
}

RenderSVGEllipse::~RenderSVGEllipse()
{
}

void RenderSVGEllipse::updateShapeFromElement()
{
    // Before creating a new object we need to clear the cached bounding box
    // to avoid using garbage.
    m_fillBoundingBox = FloatRect();
    m_strokeBoundingBox = FloatRect();
    m_center = FloatPoint();
    m_radii = FloatSize();

    // Fallback to RenderSVGShape if shape has a non-scaling stroke.
    if (hasNonScalingStroke()) {
        RenderSVGShape::updateShapeFromElement();
        m_usePathFallback = true;
        return;
    } else
        m_usePathFallback = false;

    calculateRadiiAndCenter();

    // Spec: "A value of zero disables rendering of the element."
    if (m_radii.width() <= 0 || m_radii.height() <= 0)
        return;

    m_fillBoundingBox = FloatRect(m_center.x() - m_radii.width(), m_center.y() - m_radii.height(), 2 * m_radii.width(), 2 * m_radii.height());
    m_strokeBoundingBox = m_fillBoundingBox;
    if (style()->svgStyle()->hasStroke())
        m_strokeBoundingBox.inflate(strokeWidth() / 2);
}

void RenderSVGEllipse::calculateRadiiAndCenter()
{
    ASSERT(node());
    if (node()->hasTagName(SVGNames::circleTag)) {

        SVGCircleElement* circle = static_cast<SVGCircleElement*>(node());

        SVGLengthContext lengthContext(circle);
        float radius = circle->r().value(lengthContext);
        m_radii = FloatSize(radius, radius);
        m_center = FloatPoint(circle->cx().value(lengthContext), circle->cy().value(lengthContext));
        return;
    }

    ASSERT(node()->hasTagName(SVGNames::ellipseTag));
    SVGEllipseElement* ellipse = static_cast<SVGEllipseElement*>(node());

    SVGLengthContext lengthContext(ellipse);
    m_radii = FloatSize(ellipse->rx().value(lengthContext), ellipse->ry().value(lengthContext));
    m_center = FloatPoint(ellipse->cx().value(lengthContext), ellipse->cy().value(lengthContext));
}

void RenderSVGEllipse::fillShape(GraphicsContext* context) const
{
    if (m_usePathFallback) {
        RenderSVGShape::fillShape(context);
        return;
    }
    context->fillEllipse(m_fillBoundingBox);
}

void RenderSVGEllipse::strokeShape(GraphicsContext* context) const
{
    if (!style()->svgStyle()->hasVisibleStroke())
        return;
    if (m_usePathFallback) {
        RenderSVGShape::strokeShape(context);
        return;
    }
    context->strokeEllipse(m_fillBoundingBox);
}

bool RenderSVGEllipse::shapeDependentStrokeContains(const FloatPoint& point)
{
    // The optimized contains code below does not support non-smooth strokes so we need
    // to fall back to RenderSVGShape::shapeDependentStrokeContains in these cases.
    if (m_usePathFallback || !hasSmoothStroke()) {
        if (!hasPath())
            RenderSVGShape::updateShapeFromElement();
        return RenderSVGShape::shapeDependentStrokeContains(point);
    }

    float halfStrokeWidth = strokeWidth() / 2;
    FloatPoint center = FloatPoint(m_center.x() - point.x(), m_center.y() - point.y());

    // This works by checking if the point satisfies the ellipse equation,
    // (x/rX)^2 + (y/rY)^2 <= 1, for the outer but not the inner stroke.
    float xrXOuter = center.x() / (m_radii.width() + halfStrokeWidth);
    float yrYOuter = center.y() / (m_radii.height() + halfStrokeWidth);
    if (xrXOuter * xrXOuter + yrYOuter * yrYOuter > 1.0)
        return false;

    float xrXInner = center.x() / (m_radii.width() - halfStrokeWidth);
    float yrYInner = center.y() / (m_radii.height() - halfStrokeWidth);
    return xrXInner * xrXInner + yrYInner * yrYInner >= 1.0;
}

bool RenderSVGEllipse::shapeDependentFillContains(const FloatPoint& point, const WindRule fillRule) const
{
    if (m_usePathFallback)
        return RenderSVGShape::shapeDependentFillContains(point, fillRule);

    FloatPoint center = FloatPoint(m_center.x() - point.x(), m_center.y() - point.y());

    // This works by checking if the point satisfies the ellipse equation.
    // (x/rX)^2 + (y/rY)^2 <= 1
    float xrX = center.x() / m_radii.width();
    float yrY = center.y() / m_radii.height();
    return xrX * xrX + yrY * yrY <= 1.0;
}

}

#endif // ENABLE(SVG)
