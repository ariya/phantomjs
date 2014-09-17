/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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

#ifndef SVGPathTraversalStateBuilder_h
#define SVGPathTraversalStateBuilder_h

#if ENABLE(SVG)
#include "FloatPoint.h"
#include "PathTraversalState.h"
#include "SVGPathConsumer.h"

namespace WebCore {

class SVGPathTraversalStateBuilder : public SVGPathConsumer {
public:
    SVGPathTraversalStateBuilder();

    unsigned long pathSegmentIndex();
    void setCurrentTraversalState(PathTraversalState* traversalState) { m_traversalState = traversalState; }
    void setDesiredLength(float);
    virtual void incrementPathSegmentCount();
    virtual bool continueConsuming();
    virtual void cleanup() { m_traversalState = 0; }

private:
    // Used in UnalteredParisng/NormalizedParsing modes.
    virtual void moveTo(const FloatPoint&, bool closed, PathCoordinateMode);
    virtual void lineTo(const FloatPoint&, PathCoordinateMode);
    virtual void curveToCubic(const FloatPoint&, const FloatPoint&, const FloatPoint&, PathCoordinateMode);
    virtual void closePath();

private:
    // Not used for PathTraversalState.
    virtual void lineToHorizontal(float, PathCoordinateMode) { ASSERT_NOT_REACHED(); }
    virtual void lineToVertical(float, PathCoordinateMode) { ASSERT_NOT_REACHED(); }
    virtual void curveToCubicSmooth(const FloatPoint&, const FloatPoint&, PathCoordinateMode) { ASSERT_NOT_REACHED(); }
    virtual void curveToQuadratic(const FloatPoint&, const FloatPoint&, PathCoordinateMode) { ASSERT_NOT_REACHED(); }
    virtual void curveToQuadraticSmooth(const FloatPoint&, PathCoordinateMode) { ASSERT_NOT_REACHED(); }
    virtual void arcTo(float, float, float, bool, bool, const FloatPoint&, PathCoordinateMode) { ASSERT_NOT_REACHED(); }

    PathTraversalState* m_traversalState;
    float m_desiredLength;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGPathTraversalStateBuilder_h
