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

#ifndef SVGPathBuilder_h
#define SVGPathBuilder_h

#if ENABLE(SVG)
#include "FloatPoint.h"
#include "SVGPathConsumer.h"

namespace WebCore {

class Path;

class SVGPathBuilder : public SVGPathConsumer {
public:
    SVGPathBuilder();

    void setCurrentPath(Path* path) { m_path = path; }

private:
    virtual void incrementPathSegmentCount() { }
    virtual bool continueConsuming() { return true; }
    virtual void cleanup() { m_path = 0; }

    // Used in UnalteredParsing/NormalizedParsing modes.
    virtual void moveTo(const FloatPoint&, bool closed, PathCoordinateMode);
    virtual void lineTo(const FloatPoint&, PathCoordinateMode);
    virtual void curveToCubic(const FloatPoint&, const FloatPoint&, const FloatPoint&, PathCoordinateMode);
    virtual void closePath();

    // Only used in UnalteredParsing mode.
    virtual void lineToHorizontal(float, PathCoordinateMode) { ASSERT_NOT_REACHED(); }
    virtual void lineToVertical(float, PathCoordinateMode) { ASSERT_NOT_REACHED(); }
    virtual void curveToCubicSmooth(const FloatPoint&, const FloatPoint&, PathCoordinateMode) { ASSERT_NOT_REACHED(); }
    virtual void curveToQuadratic(const FloatPoint&, const FloatPoint&, PathCoordinateMode) { ASSERT_NOT_REACHED(); }
    virtual void curveToQuadraticSmooth(const FloatPoint&, PathCoordinateMode) { ASSERT_NOT_REACHED(); }
    virtual void arcTo(float, float, float, bool, bool, const FloatPoint&, PathCoordinateMode) { ASSERT_NOT_REACHED(); }

    Path* m_path;
    FloatPoint m_current;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGPathBuilder_h
