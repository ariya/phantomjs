/*
 * Copyright (C) 2011 University of Szeged
 * Copyright (C) 2011 Renata Hodovan <reni@webkit.org>
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
#include "RenderSVGRect.h"

#include "SVGNames.h"
#include "SVGRectElement.h"

namespace WebCore {

RenderSVGRect::RenderSVGRect(SVGRectElement* node)
    : RenderSVGShape(node)
    , m_usePathFallback(false)
{
}

RenderSVGRect::~RenderSVGRect()
{
}

void RenderSVGRect::updateShapeFromElement()
{
    // Before creating a new object we need to clear the cached bounding box
    // to avoid using garbage.
    m_fillBoundingBox = FloatRect();
    m_innerStrokeRect = FloatRect();
    m_outerStrokeRect = FloatRect();
    SVGRectElement* rect = static_cast<SVGRectElement*>(node());
    ASSERT(rect);

    // Fallback to RenderSVGShape if rect has rounded corners or a non-scaling stroke.
    if (rect->hasAttribute(SVGNames::rxAttr) || rect->hasAttribute(SVGNames::ryAttr) || hasNonScalingStroke()) {
        RenderSVGShape::updateShapeFromElement();
        m_usePathFallback = true;
        return;
    } else
        m_usePathFallback = false;

    SVGLengthContext lengthContext(rect);
    FloatSize boundingBoxSize(rect->width().value(lengthContext), rect->height().value(lengthContext));
    if (boundingBoxSize.isEmpty())
        return;

    m_fillBoundingBox = FloatRect(FloatPoint(rect->x().value(lengthContext), rect->y().value(lengthContext)), boundingBoxSize);

    // To decide if the stroke contains a point we create two rects which represent the inner and
    // the outer stroke borders. A stroke contains the point, if the point is between them.
    m_innerStrokeRect = m_fillBoundingBox;
    m_outerStrokeRect = m_fillBoundingBox;

    if (style()->svgStyle()->hasStroke()) {
        float strokeWidth = this->strokeWidth();
        m_innerStrokeRect.inflate(-strokeWidth / 2);
        m_outerStrokeRect.inflate(strokeWidth / 2);
    }

    m_strokeBoundingBox = m_outerStrokeRect;

#if USE(CG)
    // CoreGraphics can inflate the stroke by 1px when drawing a rectangle with antialiasing disabled at non-integer coordinates, we need to compensate.
    if (style()->svgStyle()->shapeRendering() == SR_CRISPEDGES)
        m_strokeBoundingBox.inflate(1);
#endif
}

void RenderSVGRect::fillShape(GraphicsContext* context) const
{
    if (m_usePathFallback) {
        RenderSVGShape::fillShape(context);
        return;
    }

#if USE(CG)
    // FIXME: CG implementation of GraphicsContextCG::fillRect has an own
    // shadow drawing method, which draws an extra shadow.
    // This is a workaround for switching off the extra shadow.
    // https://bugs.webkit.org/show_bug.cgi?id=68899
    if (context->hasShadow()) {
        GraphicsContextStateSaver stateSaver(*context);
        context->clearShadow();
        context->fillRect(m_fillBoundingBox);
        return;
    }
#endif

    context->fillRect(m_fillBoundingBox);
}

void RenderSVGRect::strokeShape(GraphicsContext* context) const
{
    if (!style()->svgStyle()->hasVisibleStroke())
        return;

    if (m_usePathFallback) {
        RenderSVGShape::strokeShape(context);
        return;
    }

    context->strokeRect(m_fillBoundingBox, strokeWidth());
}

bool RenderSVGRect::shapeDependentStrokeContains(const FloatPoint& point)
{
    // The optimized contains code below does not support non-smooth strokes so we need
    // to fall back to RenderSVGShape::shapeDependentStrokeContains in these cases.
    if (m_usePathFallback || !hasSmoothStroke()) {
        if (!hasPath())
            RenderSVGShape::updateShapeFromElement();
        return RenderSVGShape::shapeDependentStrokeContains(point);
    }

    return m_outerStrokeRect.contains(point, FloatRect::InsideOrOnStroke) && !m_innerStrokeRect.contains(point, FloatRect::InsideButNotOnStroke);
}

bool RenderSVGRect::shapeDependentFillContains(const FloatPoint& point, const WindRule fillRule) const
{
    if (m_usePathFallback)
        return RenderSVGShape::shapeDependentFillContains(point, fillRule);
    return m_fillBoundingBox.contains(point.x(), point.y());
}

}

#endif // ENABLE(SVG)
