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

#ifndef SVGPathSource_h
#define SVGPathSource_h

#if ENABLE(SVG)
#include "SVGPathSeg.h"

namespace WebCore {

class FloatPoint;

class SVGPathSource {
    WTF_MAKE_NONCOPYABLE(SVGPathSource); WTF_MAKE_FAST_ALLOCATED;
public:
    SVGPathSource() { }
    virtual ~SVGPathSource() { }

    virtual bool hasMoreData() const = 0;
    virtual bool moveToNextToken() = 0;
    virtual bool parseSVGSegmentType(SVGPathSegType&) = 0;
    virtual SVGPathSegType nextCommand(SVGPathSegType previousCommand) = 0;

    virtual bool parseMoveToSegment(FloatPoint&) = 0;
    virtual bool parseLineToSegment(FloatPoint&) = 0;
    virtual bool parseLineToHorizontalSegment(float&) = 0;
    virtual bool parseLineToVerticalSegment(float&) = 0;
    virtual bool parseCurveToCubicSegment(FloatPoint&, FloatPoint&, FloatPoint&) = 0;
    virtual bool parseCurveToCubicSmoothSegment(FloatPoint&, FloatPoint&) = 0;
    virtual bool parseCurveToQuadraticSegment(FloatPoint&, FloatPoint&) = 0;
    virtual bool parseCurveToQuadraticSmoothSegment(FloatPoint&) = 0;
    virtual bool parseArcToSegment(float&, float&, float&, bool&, bool&, FloatPoint&) = 0;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGPathSource_h
