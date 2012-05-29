/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2008 Dirk Schulze <krit@webkit.org>
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
#include "RenderSVGResource.h"

#include "RenderSVGResourceContainer.h"
#include "RenderSVGResourceSolidColor.h"
#include "SVGResources.h"
#include "SVGURIReference.h"

namespace WebCore {

static inline RenderSVGResource* requestPaintingResource(RenderSVGResourceMode mode, RenderObject* object, const RenderStyle* style, Color& fallbackColor)
{
    ASSERT(object);
    ASSERT(style);

    // If we have no style at all, ignore it.
    const SVGRenderStyle* svgStyle = style->svgStyle();
    if (!svgStyle)
        return 0;

    // If we have no fill/stroke, return 0.
    if (mode == ApplyToFillMode) {
        if (!svgStyle->hasFill())
            return 0;
    } else {
        if (!svgStyle->hasStroke())
            return 0;
    }

    bool applyToFill = mode == ApplyToFillMode;
    SVGPaint::SVGPaintType paintType = applyToFill ? svgStyle->fillPaintType() : svgStyle->strokePaintType();
    if (paintType == SVGPaint::SVG_PAINTTYPE_NONE)
        return 0;

    Color color;
    switch (paintType) {
    case SVGPaint::SVG_PAINTTYPE_CURRENTCOLOR:
    case SVGPaint::SVG_PAINTTYPE_RGBCOLOR:
    case SVGPaint::SVG_PAINTTYPE_RGBCOLOR_ICCCOLOR:
    case SVGPaint::SVG_PAINTTYPE_URI_CURRENTCOLOR:
    case SVGPaint::SVG_PAINTTYPE_URI_RGBCOLOR:
    case SVGPaint::SVG_PAINTTYPE_URI_RGBCOLOR_ICCCOLOR:
        color = applyToFill ? svgStyle->fillPaintColor() : svgStyle->strokePaintColor();
    default:
        break;
    }

    if (style->insideLink() == InsideVisitedLink) {
        RenderStyle* visitedStyle = style->getCachedPseudoStyle(VISITED_LINK);
        ASSERT(visitedStyle);

        const SVGRenderStyle* svgVisitedStyle = visitedStyle->svgStyle();
        SVGPaint::SVGPaintType visitedPaintType = applyToFill ? svgVisitedStyle->fillPaintType() : svgVisitedStyle->strokePaintType();

        // For SVG_PAINTTYPE_CURRENTCOLOR, 'color' already contains the 'visitedColor'.
        if (visitedPaintType < SVGPaint::SVG_PAINTTYPE_URI_NONE && visitedPaintType != SVGPaint::SVG_PAINTTYPE_CURRENTCOLOR) {
            const Color& visitedColor = applyToFill ? svgVisitedStyle->fillPaintColor() : svgVisitedStyle->strokePaintColor();
            if (visitedColor.isValid())
                color = Color(visitedColor.red(), visitedColor.green(), visitedColor.blue(), color.alpha());
        }
    }

    // If the primary resource is just a color, return immediately.
    RenderSVGResourceSolidColor* colorResource = RenderSVGResource::sharedSolidPaintingResource();
    if (paintType < SVGPaint::SVG_PAINTTYPE_URI_NONE) {
        // If an invalid fill color is specified, fallback to fill/stroke="none".
        if (!color.isValid())
            return 0;

        colorResource->setColor(color);
        return colorResource;
    }

    // If no resources are associated with the given renderer, return the color resource.
    SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(object);
    if (!resources) {
        // If a paint server is specified, and no or an invalid fallback color is given, default to fill/stroke="black".
        if (!color.isValid())
            color = Color::black;

        colorResource->setColor(color);
        return colorResource;
    }

    // If the requested resource is not available, return the color resource.
    RenderSVGResource* uriResource = mode == ApplyToFillMode ? resources->fill() : resources->stroke();
    if (!uriResource) {
        // If a paint server is specified, and no or an invalid fallback color is given, default to fill/stroke="black".
        if (!color.isValid())
            color = Color::black;

        colorResource->setColor(color);
        return colorResource;
    }

    // The paint server resource exists, though it may be invalid (pattern with width/height=0). Pass the fallback color to our caller
    // so it can use the solid color painting resource, if applyResource() on the URI resource failed.
    fallbackColor = color;
    return uriResource;
}

RenderSVGResource* RenderSVGResource::fillPaintingResource(RenderObject* object, const RenderStyle* style, Color& fallbackColor)
{
    return requestPaintingResource(ApplyToFillMode, object, style, fallbackColor);
}

RenderSVGResource* RenderSVGResource::strokePaintingResource(RenderObject* object, const RenderStyle* style, Color& fallbackColor)
{
    return requestPaintingResource(ApplyToStrokeMode, object, style, fallbackColor);
}

RenderSVGResourceSolidColor* RenderSVGResource::sharedSolidPaintingResource()
{
    static RenderSVGResourceSolidColor* s_sharedSolidPaintingResource = 0;
    if (!s_sharedSolidPaintingResource)
        s_sharedSolidPaintingResource = new RenderSVGResourceSolidColor;
    return s_sharedSolidPaintingResource;
}

void RenderSVGResource::markForLayoutAndParentResourceInvalidation(RenderObject* object, bool needsLayout)
{
    ASSERT(object);
    if (needsLayout)
        object->setNeedsLayout(true);

    // Invalidate resources in ancestor chain, if needed.
    RenderObject* current = object->parent();
    while (current) {
        if (current->isSVGResourceContainer()) {
            current->toRenderSVGResourceContainer()->removeAllClientsFromCache();
            break;
        }

        current = current->parent();
    }
}

}

#endif
