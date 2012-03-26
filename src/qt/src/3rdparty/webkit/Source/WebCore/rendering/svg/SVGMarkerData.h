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

#ifndef SVGMarkerData_h
#define SVGMarkerData_h

#if ENABLE(SVG)
#include "FloatConversion.h"
#include "Path.h"
#include <wtf/MathExtras.h>

namespace WebCore {

class RenderSVGResourceMarker;

class SVGMarkerData {
public:
    enum Type {
        Unknown = 0,
        Start,
        Mid,
        End
    };

    SVGMarkerData(const Type& type = Unknown, RenderSVGResourceMarker* marker = 0)
        : m_type(type)
        , m_marker(marker)
    {
    }

    FloatPoint origin() const { return m_origin; }
    RenderSVGResourceMarker* marker() const { return m_marker; }

    float currentAngle() const
    {
        FloatSize inslopeChange = m_inslopePoints[1] - m_inslopePoints[0];
        FloatSize outslopeChange = m_outslopePoints[1] - m_outslopePoints[0];

        double inslope = rad2deg(atan2(inslopeChange.height(), inslopeChange.width()));
        double outslope = rad2deg(atan2(outslopeChange.height(), outslopeChange.width()));

        double angle = 0;
        switch (m_type) {
        case Start:
            angle = outslope;
            break;
        case Mid:
            angle = (inslope + outslope) / 2;
            break;
        case End:
            angle = inslope;
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }

        return narrowPrecisionToFloat(angle);
    }

    void updateTypeAndMarker(const Type& type, RenderSVGResourceMarker* marker)
    {
        m_type = type;
        m_marker = marker;
    }

    void updateOutslope(const FloatPoint& point)
    {
        m_outslopePoints[0] = m_origin;
        m_outslopePoints[1] = point;
    }

    void updateMarkerDataForPathElement(const PathElement* element)
    {
        FloatPoint* points = element->points;

        switch (element->type) {
        case PathElementAddQuadCurveToPoint:
            // FIXME: https://bugs.webkit.org/show_bug.cgi?id=33115 (PathElementAddQuadCurveToPoint not handled for <marker>)
            m_origin = points[1];
            break;
        case PathElementAddCurveToPoint:
            m_inslopePoints[0] = points[1];
            m_inslopePoints[1] = points[2];
            m_origin = points[2];
            break;
        case PathElementMoveToPoint:
            m_subpathStart = points[0];
        case PathElementAddLineToPoint:
            updateInslope(points[0]);
            m_origin = points[0];
            break;
        case PathElementCloseSubpath:
            updateInslope(points[0]);
            m_origin = m_subpathStart;
            m_subpathStart = FloatPoint();
        }
    }

private:
    void updateInslope(const FloatPoint& point)
    {
        m_inslopePoints[0] = m_origin;
        m_inslopePoints[1] = point;
    }

    Type m_type;
    RenderSVGResourceMarker* m_marker;
    FloatPoint m_origin;
    FloatPoint m_subpathStart;
    FloatPoint m_inslopePoints[2];
    FloatPoint m_outslopePoints[2];
};

}

#endif // ENABLE(SVG)
#endif // SVGMarkerData_h
