/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#if ENABLE(SVG)
#include "RenderSVGResourceSolidColor.h"

#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "RenderSVGShape.h"
#include "RenderStyle.h"
#include "SVGRenderSupport.h"

namespace WebCore {

RenderSVGResourceType RenderSVGResourceSolidColor::s_resourceType = SolidColorResourceType;

RenderSVGResourceSolidColor::RenderSVGResourceSolidColor()
{
}

RenderSVGResourceSolidColor::~RenderSVGResourceSolidColor()
{
}

bool RenderSVGResourceSolidColor::applyResource(RenderObject* object, RenderStyle* style, GraphicsContext*& context, unsigned short resourceMode)
{
    // We are NOT allowed to ASSERT(object) here, unlike all other resources.
    // RenderSVGResourceSolidColor is the only resource which may be used from HTML, when rendering
    // SVG Fonts for a HTML document. This will be indicated by a null RenderObject pointer.
    ASSERT(context);
    ASSERT(resourceMode != ApplyToDefaultMode);

    const SVGRenderStyle* svgStyle = style ? style->svgStyle() : 0;
    ColorSpace colorSpace = style ? style->colorSpace() : ColorSpaceDeviceRGB;

    bool isRenderingMask = false;
    if (object->frame() && object->frame()->view())
        isRenderingMask = object->frame()->view()->paintBehavior() & PaintBehaviorRenderingSVGMask;

    if (resourceMode & ApplyToFillMode) {
        if (!isRenderingMask && svgStyle)
            context->setAlpha(svgStyle->fillOpacity());
        else
            context->setAlpha(1);
        context->setFillColor(m_color, colorSpace);
        if (!isRenderingMask)
            context->setFillRule(svgStyle ? svgStyle->fillRule() : RULE_NONZERO);

        if (resourceMode & ApplyToTextMode)
            context->setTextDrawingMode(TextModeFill);
    } else if (resourceMode & ApplyToStrokeMode) {
        // When rendering the mask for a RenderSVGResourceClipper, the stroke code path is never hit.
        ASSERT(!isRenderingMask);
        context->setAlpha(svgStyle ? svgStyle->strokeOpacity() : 1);
        context->setStrokeColor(m_color, colorSpace);

        if (style)
            SVGRenderSupport::applyStrokeStyleToContext(context, style, object);

        if (resourceMode & ApplyToTextMode)
            context->setTextDrawingMode(TextModeStroke);
    }

    return true;
}

void RenderSVGResourceSolidColor::postApplyResource(RenderObject*, GraphicsContext*& context, unsigned short resourceMode, const Path* path, const RenderSVGShape* shape)
{
    ASSERT(context);
    ASSERT(resourceMode != ApplyToDefaultMode);

    if (resourceMode & ApplyToFillMode) {
        if (path)
            context->fillPath(*path);
        else if (shape)
            shape->fillShape(context);
    }
    if (resourceMode & ApplyToStrokeMode) {
        if (path)
            context->strokePath(*path);
        else if (shape)
            shape->strokeShape(context);
    }
}

}

#endif
