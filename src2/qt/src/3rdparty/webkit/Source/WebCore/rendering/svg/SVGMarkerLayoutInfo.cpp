/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) 2005, 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.
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
#include "SVGMarkerLayoutInfo.h"

#include "RenderSVGResourceMarker.h"

namespace WebCore {

SVGMarkerLayoutInfo::SVGMarkerLayoutInfo()
    : m_midMarker(0)
    , m_elementIndex(0)
    , m_strokeWidth(0)
{
}

SVGMarkerLayoutInfo::~SVGMarkerLayoutInfo()
{
}

static inline void processStartAndMidMarkers(void* infoPtr, const PathElement* element)
{
    SVGMarkerLayoutInfo& info = *reinterpret_cast<SVGMarkerLayoutInfo*>(infoPtr);
    SVGMarkerData& markerData = info.markerData();
    int& elementIndex = info.elementIndex();

    // First update the outslope for the previous element
    markerData.updateOutslope(element->points[0]);

    // Draw the marker for the previous element
    RenderSVGResourceMarker* marker = markerData.marker();
    if (elementIndex > 0 && marker)
        info.addLayoutedMarker(marker, markerData.origin(), markerData.currentAngle());

    // Update our marker data for this element
    markerData.updateMarkerDataForPathElement(element);

    // After drawing the start marker, switch to drawing mid markers
    if (elementIndex == 1)
        markerData.updateTypeAndMarker(SVGMarkerData::Mid, info.midMarker());

    ++elementIndex;
}

FloatRect SVGMarkerLayoutInfo::calculateBoundaries(RenderSVGResourceMarker* startMarker, RenderSVGResourceMarker* midMarker, RenderSVGResourceMarker* endMarker, float strokeWidth, const Path& path)
{
    m_layout.clear();
    m_midMarker = midMarker;
    m_strokeWidth = strokeWidth;
    m_elementIndex = 0;
    m_markerData = SVGMarkerData(SVGMarkerData::Start, startMarker);
    path.apply(this, processStartAndMidMarkers);

    if (endMarker) {
        m_markerData.updateTypeAndMarker(SVGMarkerData::End, endMarker);
        addLayoutedMarker(endMarker, m_markerData.origin(), m_markerData.currentAngle());
    }

    if (m_layout.isEmpty())
        return FloatRect();

    Vector<MarkerLayout>::iterator it = m_layout.begin();
    Vector<MarkerLayout>::iterator end = m_layout.end();

    FloatRect bounds;
    for (; it != end; ++it) {
        MarkerLayout& layout = *it;

        RenderSVGResourceMarker* markerContent = layout.marker;
        ASSERT(markerContent);

        bounds.unite(markerContent->markerBoundaries(layout.matrix));
    }

    return bounds;
}

void SVGMarkerLayoutInfo::clear()
{
    m_midMarker = 0;
    m_elementIndex = 0;
    m_strokeWidth = 0;
    m_markerData.updateTypeAndMarker(SVGMarkerData::Unknown, 0);
    m_layout.clear();
}

void SVGMarkerLayoutInfo::drawMarkers(PaintInfo& paintInfo)
{
    if (m_layout.isEmpty())
        return;

    Vector<MarkerLayout>::iterator it = m_layout.begin();
    Vector<MarkerLayout>::iterator end = m_layout.end();

    for (; it != end; ++it) {
        MarkerLayout& layout = *it;
        layout.marker->draw(paintInfo, layout.matrix);
    }
}

void SVGMarkerLayoutInfo::addLayoutedMarker(RenderSVGResourceMarker* marker, const FloatPoint& origin, float angle)
{
    ASSERT(marker);
    m_layout.append(MarkerLayout(marker, marker->markerTransformation(origin, angle, m_strokeWidth)));
}

}

#endif // ENABLE(SVG)
