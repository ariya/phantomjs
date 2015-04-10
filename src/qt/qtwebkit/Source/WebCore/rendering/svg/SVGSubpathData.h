/*
 * Copyright (C) 2012 Google, Inc.
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

#ifndef SVGSubpathData_h
#define SVGSubpathData_h

#if ENABLE(SVG)
#include "Path.h"
#include <wtf/Vector.h>

namespace WebCore {

class SVGSubpathData {
public:
    SVGSubpathData(Vector<FloatPoint>& zeroLengthSubpathLocations)
        : m_zeroLengthSubpathLocations(zeroLengthSubpathLocations)
        , m_haveSeenMoveOnly(true)
        , m_pathIsZeroLength(true)
    {
        m_lastPoint.set(0, 0);
        m_movePoint.set(0, 0);
    }

    static void updateFromPathElement(void* info, const PathElement* element)
    {
        SVGSubpathData* subpathFinder = static_cast<SVGSubpathData*>(info);
        switch (element->type) {
        case PathElementMoveToPoint:
            if (subpathFinder->m_pathIsZeroLength && !subpathFinder->m_haveSeenMoveOnly)
                subpathFinder->m_zeroLengthSubpathLocations.append(subpathFinder->m_lastPoint);
            subpathFinder->m_lastPoint = subpathFinder->m_movePoint = element->points[0];
            subpathFinder->m_haveSeenMoveOnly = true;
            subpathFinder->m_pathIsZeroLength = true;
            break;
        case PathElementAddLineToPoint:
            if (subpathFinder->m_lastPoint != element->points[0]) {
                subpathFinder->m_pathIsZeroLength = false;
                subpathFinder->m_lastPoint = element->points[0];
            }
            subpathFinder->m_haveSeenMoveOnly = false;
            break;
        case PathElementAddQuadCurveToPoint:
            if (subpathFinder->m_lastPoint != element->points[0] || element->points[0] != element->points[1]) {
                subpathFinder->m_pathIsZeroLength = false;
                subpathFinder->m_lastPoint = element->points[1];
            }
            subpathFinder->m_haveSeenMoveOnly = false;
            break;
        case PathElementAddCurveToPoint:
            if (subpathFinder->m_lastPoint != element->points[0] || element->points[0] != element->points[1] || element->points[1] != element->points[2]) {
                subpathFinder->m_pathIsZeroLength = false;
                subpathFinder->m_lastPoint = element->points[2];
            }
            subpathFinder->m_haveSeenMoveOnly = false;
            break;
        case PathElementCloseSubpath:
            if (subpathFinder->m_pathIsZeroLength)
                subpathFinder->m_zeroLengthSubpathLocations.append(subpathFinder->m_lastPoint);
            subpathFinder->m_haveSeenMoveOnly = true; // This is an implicit move for the next element
            subpathFinder->m_pathIsZeroLength = true; // A new sub-path also starts here
            subpathFinder->m_lastPoint = subpathFinder->m_movePoint;
            break;
        }
    }

    void pathIsDone()
    {
        if (m_pathIsZeroLength && !m_haveSeenMoveOnly)
            m_zeroLengthSubpathLocations.append(m_lastPoint);
    }

private:
    Vector<FloatPoint>& m_zeroLengthSubpathLocations;
    FloatPoint m_lastPoint;
    FloatPoint m_movePoint;
    bool m_haveSeenMoveOnly;
    bool m_pathIsZeroLength;
};

}

#endif // ENABLE(SVG)
#endif // SVGSubpathData_h

