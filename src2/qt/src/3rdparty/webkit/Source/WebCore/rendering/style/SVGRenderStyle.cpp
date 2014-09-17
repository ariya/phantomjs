/*
    Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2010 Rob Buis <buis@kde.org>
    Copyright (C) Research In Motion Limited 2010. All rights reserved.

    Based on khtml code by:
    Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
    Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
    Copyright (C) 2002-2003 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Apple Computer, Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#if ENABLE(SVG)
#include "SVGRenderStyle.h"

#include "CSSPrimitiveValue.h"
#include "CSSValueList.h"
#include "IntRect.h"
#include "NodeRenderStyle.h"
#include "SVGStyledElement.h"

using namespace std;

namespace WebCore {

SVGRenderStyle::SVGRenderStyle()
{
    static SVGRenderStyle* defaultStyle = new SVGRenderStyle(CreateDefault);

    fill = defaultStyle->fill;
    stroke = defaultStyle->stroke;
    text = defaultStyle->text;
    stops = defaultStyle->stops;
    misc = defaultStyle->misc;
    shadowSVG = defaultStyle->shadowSVG;
    inheritedResources = defaultStyle->inheritedResources;
    resources = defaultStyle->resources;

    setBitDefaults();
}

SVGRenderStyle::SVGRenderStyle(CreateDefaultType)
{
    setBitDefaults();

    fill.init();
    stroke.init();
    text.init();
    stops.init();
    misc.init();
    shadowSVG.init();
    inheritedResources.init();
    resources.init();
}

SVGRenderStyle::SVGRenderStyle(const SVGRenderStyle& other)
    : RefCounted<SVGRenderStyle>()
{
    fill = other.fill;
    stroke = other.stroke;
    text = other.text;
    stops = other.stops;
    misc = other.misc;
    shadowSVG = other.shadowSVG;
    inheritedResources = other.inheritedResources;
    resources = other.resources;

    svg_inherited_flags = other.svg_inherited_flags;
    svg_noninherited_flags = other.svg_noninherited_flags;
}

SVGRenderStyle::~SVGRenderStyle()
{
}

bool SVGRenderStyle::operator==(const SVGRenderStyle& other) const
{
    return fill == other.fill
        && stroke == other.stroke
        && text == other.text
        && stops == other.stops
        && misc == other.misc
        && shadowSVG == other.shadowSVG
        && inheritedResources == other.inheritedResources
        && resources == other.resources
        && svg_inherited_flags == other.svg_inherited_flags
        && svg_noninherited_flags == other.svg_noninherited_flags;
}

bool SVGRenderStyle::inheritedNotEqual(const SVGRenderStyle* other) const
{
    return fill != other->fill
        || stroke != other->stroke
        || text != other->text
        || inheritedResources != other->inheritedResources
        || svg_inherited_flags != other->svg_inherited_flags;
}

void SVGRenderStyle::inheritFrom(const SVGRenderStyle* svgInheritParent)
{
    if (!svgInheritParent)
        return;

    fill = svgInheritParent->fill;
    stroke = svgInheritParent->stroke;
    text = svgInheritParent->text;
    inheritedResources = svgInheritParent->inheritedResources;

    svg_inherited_flags = svgInheritParent->svg_inherited_flags;
}

StyleDifference SVGRenderStyle::diff(const SVGRenderStyle* other) const
{
    // NOTE: All comparisions that may return StyleDifferenceLayout have to go before those who return StyleDifferenceRepaint

    // If kerning changes, we need a relayout, to force SVGCharacterData to be recalculated in the SVGRootInlineBox.
    if (text != other->text)
        return StyleDifferenceLayout;

    // If resources change, we need a relayout, as the presence of resources influences the repaint rect.
    if (resources != other->resources)
        return StyleDifferenceLayout;

    // If markers change, we need a relayout, as marker boundaries are cached in RenderSVGPath.
    if (inheritedResources != other->inheritedResources)
        return StyleDifferenceLayout;

    // All text related properties influence layout.
    if (svg_inherited_flags._textAnchor != other->svg_inherited_flags._textAnchor
        || svg_inherited_flags._writingMode != other->svg_inherited_flags._writingMode
        || svg_inherited_flags._glyphOrientationHorizontal != other->svg_inherited_flags._glyphOrientationHorizontal
        || svg_inherited_flags._glyphOrientationVertical != other->svg_inherited_flags._glyphOrientationVertical
        || svg_noninherited_flags.f._alignmentBaseline != other->svg_noninherited_flags.f._alignmentBaseline
        || svg_noninherited_flags.f._dominantBaseline != other->svg_noninherited_flags.f._dominantBaseline
        || svg_noninherited_flags.f._baselineShift != other->svg_noninherited_flags.f._baselineShift)
        return StyleDifferenceLayout;

    // Text related properties influence layout.
    bool miscNotEqual = misc != other->misc;
    if (miscNotEqual && misc->baselineShiftValue != other->misc->baselineShiftValue)
        return StyleDifferenceLayout;

    // These properties affect the cached stroke bounding box rects.
    if (svg_inherited_flags._capStyle != other->svg_inherited_flags._capStyle
        || svg_inherited_flags._joinStyle != other->svg_inherited_flags._joinStyle)
        return StyleDifferenceLayout;

    // Shadow changes require relayouts, as they affect the repaint rects.
    if (shadowSVG != other->shadowSVG)
        return StyleDifferenceLayout;

    // Some stroke properties, requires relayouts, as the cached stroke boundaries need to be recalculated.
    if (stroke != other->stroke) {
        if (stroke->width != other->stroke->width
            || stroke->paintType != other->stroke->paintType
            || stroke->paintColor != other->stroke->paintColor
            || stroke->paintUri != other->stroke->paintUri
            || stroke->miterLimit != other->stroke->miterLimit
            || stroke->dashArray != other->stroke->dashArray
            || stroke->dashOffset != other->stroke->dashOffset)
            return StyleDifferenceLayout;

        // Only the stroke-opacity case remains, where we only need a repaint.
        ASSERT(stroke->opacity != other->stroke->opacity);
        return StyleDifferenceRepaint;
    }

    // NOTE: All comparisions below may only return StyleDifferenceRepaint

    // Painting related properties only need repaints. 
    if (miscNotEqual) {
        if (misc->floodColor != other->misc->floodColor
            || misc->floodOpacity != other->misc->floodOpacity
            || misc->lightingColor != other->misc->lightingColor)
            return StyleDifferenceRepaint;
    }

    // If fill changes, we just need to repaint. Fill boundaries are not influenced by this, only by the Path, that RenderSVGPath contains.
    if (fill->paintType != other->fill->paintType || fill->paintColor != other->fill->paintColor
        || fill->paintUri != other->fill->paintUri || fill->opacity != other->fill->opacity)
        return StyleDifferenceRepaint;

    // If gradient stops change, we just need to repaint. Style updates are already handled through RenderSVGGradientSTop.
    if (stops != other->stops)
        return StyleDifferenceRepaint;

    // Changes of these flags only cause repaints.
    if (svg_inherited_flags._colorRendering != other->svg_inherited_flags._colorRendering
        || svg_inherited_flags._imageRendering != other->svg_inherited_flags._imageRendering
        || svg_inherited_flags._shapeRendering != other->svg_inherited_flags._shapeRendering
        || svg_inherited_flags._clipRule != other->svg_inherited_flags._clipRule
        || svg_inherited_flags._fillRule != other->svg_inherited_flags._fillRule
        || svg_inherited_flags._colorInterpolation != other->svg_inherited_flags._colorInterpolation
        || svg_inherited_flags._colorInterpolationFilters != other->svg_inherited_flags._colorInterpolationFilters)
        return StyleDifferenceRepaint;

    // FIXME: vector-effect is not taken into account in the layout-phase. Once this is fixed, we should relayout here.
    if (svg_noninherited_flags.f._vectorEffect != other->svg_noninherited_flags.f._vectorEffect)
        return StyleDifferenceRepaint;

    return StyleDifferenceEqual;
}

}

#endif // ENABLE(SVG)
