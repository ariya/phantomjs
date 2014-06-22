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

#ifndef SVGPathStringBuilder_h
#define SVGPathStringBuilder_h

#if ENABLE(SVG)
#include "FloatPoint.h"
#include "SVGPathConsumer.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

class SVGPathStringBuilder : public SVGPathConsumer {
public:
    String result();

private:
    virtual void cleanup() { m_stringBuilder.clear(); }
    virtual void incrementPathSegmentCount() { }
    virtual bool continueConsuming() { return true; }

    // Used in UnalteredParsing/NormalizedParsing modes.
    virtual void moveTo(const FloatPoint&, bool closed, PathCoordinateMode);
    virtual void lineTo(const FloatPoint&, PathCoordinateMode);
    virtual void curveToCubic(const FloatPoint&, const FloatPoint&, const FloatPoint&, PathCoordinateMode);
    virtual void closePath();

    // Only used in UnalteredParsing mode.
    virtual void lineToHorizontal(float, PathCoordinateMode);
    virtual void lineToVertical(float, PathCoordinateMode);
    virtual void curveToCubicSmooth(const FloatPoint&, const FloatPoint&, PathCoordinateMode);
    virtual void curveToQuadratic(const FloatPoint&, const FloatPoint&, PathCoordinateMode);
    virtual void curveToQuadraticSmooth(const FloatPoint&, PathCoordinateMode);
    virtual void arcTo(float, float, float, bool largeArcFlag, bool sweepFlag, const FloatPoint&, PathCoordinateMode);

    StringBuilder m_stringBuilder;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGPathStringBuilder_h
