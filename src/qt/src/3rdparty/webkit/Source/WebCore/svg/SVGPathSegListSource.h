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

#ifndef SVGPathSegListSource_h
#define SVGPathSegListSource_h

#if ENABLE(SVG)
#include "FloatPoint.h"
#include "SVGPathSeg.h"
#include "SVGPathSegList.h"
#include "SVGPathSource.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class SVGPathSegListSource : public SVGPathSource {
public:
    static PassOwnPtr<SVGPathSegListSource> create(const SVGPathSegList& pathSegList)
    {
        return adoptPtr(new SVGPathSegListSource(pathSegList));
    }

private:
    SVGPathSegListSource(const SVGPathSegList&);

    virtual bool hasMoreData() const;
    virtual bool moveToNextToken() { return true; }
    virtual bool parseSVGSegmentType(SVGPathSegType&);
    virtual SVGPathSegType nextCommand(SVGPathSegType);

    virtual bool parseMoveToSegment(FloatPoint&);
    virtual bool parseLineToSegment(FloatPoint&);
    virtual bool parseLineToHorizontalSegment(float&);
    virtual bool parseLineToVerticalSegment(float&);
    virtual bool parseCurveToCubicSegment(FloatPoint&, FloatPoint&, FloatPoint&);
    virtual bool parseCurveToCubicSmoothSegment(FloatPoint&, FloatPoint&);
    virtual bool parseCurveToQuadraticSegment(FloatPoint&, FloatPoint&);
    virtual bool parseCurveToQuadraticSmoothSegment(FloatPoint&);
    virtual bool parseArcToSegment(float&, float&, float&, bool&, bool&, FloatPoint&);

    const SVGPathSegList& m_pathSegList;
    RefPtr<SVGPathSeg> m_segment;
    int m_itemCurrent;
    int m_itemEnd;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGPathSegListSource_h
