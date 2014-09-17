/*
    Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2007 Rob Buis <buis@kde.org>
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
#include "SVGRenderStyleDefs.h"

#include "RenderStyle.h"
#include "SVGRenderStyle.h"

namespace WebCore {

StyleFillData::StyleFillData()
    : opacity(SVGRenderStyle::initialFillOpacity())
    , paintType(SVGRenderStyle::initialFillPaintType())
    , paintColor(SVGRenderStyle::initialFillPaintColor())
    , paintUri(SVGRenderStyle::initialFillPaintUri())
{
}

StyleFillData::StyleFillData(const StyleFillData& other)
    : RefCounted<StyleFillData>()
    , opacity(other.opacity)
    , paintType(other.paintType)
    , paintColor(other.paintColor)
    , paintUri(other.paintUri)
{
}

bool StyleFillData::operator==(const StyleFillData& other) const
{
    return opacity == other.opacity 
        && paintType == other.paintType
        && paintColor == other.paintColor
        && paintUri == other.paintUri;
}

StyleStrokeData::StyleStrokeData()
    : opacity(SVGRenderStyle::initialStrokeOpacity())
    , miterLimit(SVGRenderStyle::initialStrokeMiterLimit())
    , width(SVGRenderStyle::initialStrokeWidth())
    , dashOffset(SVGRenderStyle::initialStrokeDashOffset())
    , dashArray(SVGRenderStyle::initialStrokeDashArray())
    , paintType(SVGRenderStyle::initialStrokePaintType())
    , paintColor(SVGRenderStyle::initialStrokePaintColor())
    , paintUri(SVGRenderStyle::initialStrokePaintUri())
{
}

StyleStrokeData::StyleStrokeData(const StyleStrokeData& other)
    : RefCounted<StyleStrokeData>()
    , opacity(other.opacity)
    , miterLimit(other.miterLimit)
    , width(other.width)
    , dashOffset(other.dashOffset)
    , dashArray(other.dashArray)
    , paintType(other.paintType)
    , paintColor(other.paintColor)
    , paintUri(other.paintUri)
{
}

bool StyleStrokeData::operator==(const StyleStrokeData& other) const
{
    return width == other.width
        && opacity == other.opacity
        && miterLimit == other.miterLimit
        && dashOffset == other.dashOffset
        && dashArray == other.dashArray
        && paintType == other.paintType
        && paintColor == other.paintColor
        && paintUri == other.paintUri;
}

StyleStopData::StyleStopData()
    : opacity(SVGRenderStyle::initialStopOpacity())
    , color(SVGRenderStyle::initialStopColor())
{
}

StyleStopData::StyleStopData(const StyleStopData& other)
    : RefCounted<StyleStopData>()
    , opacity(other.opacity)
    , color(other.color)
{
}

bool StyleStopData::operator==(const StyleStopData& other) const
{
    return color == other.color
        && opacity == other.opacity;
}

StyleTextData::StyleTextData()
    : kerning(SVGRenderStyle::initialKerning())
{
}

StyleTextData::StyleTextData(const StyleTextData& other)
    : RefCounted<StyleTextData>()
    , kerning(other.kerning)
{
}

bool StyleTextData::operator==(const StyleTextData& other) const
{
    return kerning == other.kerning;
}

StyleMiscData::StyleMiscData()
    : floodColor(SVGRenderStyle::initialFloodColor())
    , floodOpacity(SVGRenderStyle::initialFloodOpacity())
    , lightingColor(SVGRenderStyle::initialLightingColor())
    , baselineShiftValue(SVGRenderStyle::initialBaselineShiftValue())
{
}

StyleMiscData::StyleMiscData(const StyleMiscData& other)
    : RefCounted<StyleMiscData>()
    , floodColor(other.floodColor)
    , floodOpacity(other.floodOpacity)
    , lightingColor(other.lightingColor)
    , baselineShiftValue(other.baselineShiftValue)
{
}

bool StyleMiscData::operator==(const StyleMiscData& other) const
{
    return floodOpacity == other.floodOpacity
        && floodColor == other.floodColor
        && lightingColor == other.lightingColor
        && baselineShiftValue == other.baselineShiftValue;
}

StyleShadowSVGData::StyleShadowSVGData()
{
}

StyleShadowSVGData::StyleShadowSVGData(const StyleShadowSVGData& other)
    : RefCounted<StyleShadowSVGData>()
    , shadow(other.shadow ? adoptPtr(new ShadowData(*other.shadow)) : nullptr)
{
}

bool StyleShadowSVGData::operator==(const StyleShadowSVGData& other) const
{
    if ((!shadow && other.shadow) || (shadow && !other.shadow))
        return false;
    if (shadow && other.shadow && (*shadow != *other.shadow))
        return false;
    return true;
}

StyleResourceData::StyleResourceData()
    : clipper(SVGRenderStyle::initialClipperResource())
    , filter(SVGRenderStyle::initialFilterResource())
    , masker(SVGRenderStyle::initialMaskerResource())
{
}

StyleResourceData::StyleResourceData(const StyleResourceData& other)
    : RefCounted<StyleResourceData>()
    , clipper(other.clipper)
    , filter(other.filter)
    , masker(other.masker)
{
}

bool StyleResourceData::operator==(const StyleResourceData& other) const
{
    return clipper == other.clipper
        && filter == other.filter
        && masker == other.masker;
}

StyleInheritedResourceData::StyleInheritedResourceData()
    : markerStart(SVGRenderStyle::initialMarkerStartResource())
    , markerMid(SVGRenderStyle::initialMarkerMidResource())
    , markerEnd(SVGRenderStyle::initialMarkerEndResource())
{
}

StyleInheritedResourceData::StyleInheritedResourceData(const StyleInheritedResourceData& other)
    : RefCounted<StyleInheritedResourceData>()
    , markerStart(other.markerStart)
    , markerMid(other.markerMid)
    , markerEnd(other.markerEnd)
{
}

bool StyleInheritedResourceData::operator==(const StyleInheritedResourceData& other) const
{
    return markerStart == other.markerStart
        && markerMid == other.markerMid
        && markerEnd == other.markerEnd;
}

}

#endif // ENABLE(SVG)
