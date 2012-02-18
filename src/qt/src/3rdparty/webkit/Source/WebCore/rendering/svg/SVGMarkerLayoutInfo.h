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

#ifndef SVGMarkerLayoutInfo_h
#define SVGMarkerLayoutInfo_h

#if ENABLE(SVG)
#include "AffineTransform.h"
#include "RenderObject.h"
#include "SVGMarkerData.h"
#include <wtf/Noncopyable.h>

namespace WebCore {

class Path;
class RenderSVGResourceMarker;

struct MarkerLayout {
    MarkerLayout(RenderSVGResourceMarker* markerObj = 0, AffineTransform matrixObj = AffineTransform())
        : marker(markerObj)
        , matrix(matrixObj)
    {
        ASSERT(marker);
    }

    RenderSVGResourceMarker* marker;
    AffineTransform matrix;
};

class SVGMarkerLayoutInfo {
    WTF_MAKE_NONCOPYABLE(SVGMarkerLayoutInfo);
public:
    SVGMarkerLayoutInfo();
    ~SVGMarkerLayoutInfo();

    FloatRect calculateBoundaries(RenderSVGResourceMarker* startMarker, RenderSVGResourceMarker* midMarker, RenderSVGResourceMarker* endMarker, float strokeWidth, const Path&);
    void drawMarkers(PaintInfo&);

    // Used by static inline helper functions in SVGMarkerLayoutInfo.cpp
    SVGMarkerData& markerData() { return m_markerData; }
    RenderSVGResourceMarker* midMarker() const { return m_midMarker; }
    int& elementIndex() { return m_elementIndex; }
    void addLayoutedMarker(RenderSVGResourceMarker*, const FloatPoint& origin, float angle);
    void clear();

private:
    RenderSVGResourceMarker* m_midMarker;

    // Used while layouting markers
    int m_elementIndex;
    SVGMarkerData m_markerData;
    float m_strokeWidth;

    // Holds the final computed result
    Vector<MarkerLayout> m_layout;
};

}

#endif
#endif
