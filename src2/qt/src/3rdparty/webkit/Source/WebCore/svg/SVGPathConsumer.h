/*
 * Copyright (C) 2002, 2003 The Karbon Developers
 * Copyright (C) 2006 Alexander Kellett <lypanov@kde.org>
 * Copyright (C) 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
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

#ifndef SVGPathConsumer_h
#define SVGPathConsumer_h

#if ENABLE(SVG)
#include "FloatPoint.h"
#include <wtf/FastAllocBase.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

enum PathCoordinateMode {
    AbsoluteCoordinates,
    RelativeCoordinates
};

enum PathParsingMode {
    NormalizedParsing,
    UnalteredParsing
};

class SVGPathConsumer {
    WTF_MAKE_NONCOPYABLE(SVGPathConsumer); WTF_MAKE_FAST_ALLOCATED;
public:
    SVGPathConsumer() { }
    virtual void incrementPathSegmentCount() = 0;
    virtual bool continueConsuming() = 0;
    virtual void cleanup() = 0;

    // Used in UnalteredParisng/NormalizedParsing modes.
    virtual void moveTo(const FloatPoint&, bool closed, PathCoordinateMode) = 0;
    virtual void lineTo(const FloatPoint&, PathCoordinateMode) = 0;
    virtual void curveToCubic(const FloatPoint&, const FloatPoint&, const FloatPoint&, PathCoordinateMode) = 0;
    virtual void closePath() = 0;

    // Only used in UnalteredParsing mode.
    virtual void lineToHorizontal(float, PathCoordinateMode) = 0;
    virtual void lineToVertical(float, PathCoordinateMode) = 0;
    virtual void curveToCubicSmooth(const FloatPoint&, const FloatPoint&, PathCoordinateMode) = 0;
    virtual void curveToQuadratic(const FloatPoint&, const FloatPoint&, PathCoordinateMode) = 0;
    virtual void curveToQuadraticSmooth(const FloatPoint&, PathCoordinateMode) = 0;
    virtual void arcTo(float, float, float, bool largeArcFlag, bool sweepFlag, const FloatPoint&, PathCoordinateMode) = 0;

protected:
    virtual ~SVGPathConsumer() { }
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGPathConsumer_h
