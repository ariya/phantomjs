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

#include "GraphicsContext.h"
#include "RenderStyle.h"
#include "SVGRenderSupport.h"

#if USE(SKIA)
#include "PlatformContextSkia.h"
#endif

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

    if (resourceMode & ApplyToFillMode) {
        context->setAlpha(svgStyle ? svgStyle->fillOpacity() : 1.0f);
        context->setFillColor(m_color, colorSpace);
        context->setFillRule(svgStyle ? svgStyle->fillRule() : RULE_NONZERO);

        if (resourceMode & ApplyToTextMode)
            context->setTextDrawingMode(TextModeFill);
    } else if (resourceMode & ApplyToStrokeMode) {
        context->setAlpha(svgStyle ? svgStyle->strokeOpacity() : 1.0f);
        context->setStrokeColor(m_color, colorSpace);

        if (style)
            SVGRenderSupport::applyStrokeStyleToContext(context, style, object);

        if (resourceMode & ApplyToTextMode)
            context->setTextDrawingMode(TextModeStroke);
    }

    return true;
}

void RenderSVGResourceSolidColor::postApplyResource(RenderObject*, GraphicsContext*& context, unsigned short resourceMode, const Path* path)
{
    ASSERT(context);
    ASSERT(resourceMode != ApplyToDefaultMode);

    if (path && !(resourceMode & ApplyToTextMode)) {
        if (resourceMode & ApplyToFillMode)
            context->fillPath(*path);
        else if (resourceMode & ApplyToStrokeMode)
            context->strokePath(*path);
    }
}

}

#endif
