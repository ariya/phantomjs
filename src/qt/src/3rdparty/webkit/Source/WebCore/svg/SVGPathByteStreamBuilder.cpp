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

#include "config.h"

#if ENABLE(SVG)
#include "SVGPathByteStreamBuilder.h"

#include "SVGPathParser.h"
#include "SVGPathSeg.h"
#include "SVGPathStringSource.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

SVGPathByteStreamBuilder::SVGPathByteStreamBuilder()
    : m_byteStream(0)
{
}

void SVGPathByteStreamBuilder::moveTo(const FloatPoint& targetPoint, bool, PathCoordinateMode mode)
{
    ASSERT(m_byteStream);
    writeSegmentType(mode == RelativeCoordinates ?  PathSegMoveToRel : PathSegMoveToAbs);
    writeFloatPoint(targetPoint);
}

void SVGPathByteStreamBuilder::lineTo(const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    ASSERT(m_byteStream);
    writeSegmentType(mode == RelativeCoordinates ? PathSegLineToRel : PathSegLineToAbs);
    writeFloatPoint(targetPoint);
}

void SVGPathByteStreamBuilder::lineToHorizontal(float x, PathCoordinateMode mode)
{
    ASSERT(m_byteStream);
    writeSegmentType(mode == RelativeCoordinates ? PathSegLineToHorizontalRel : PathSegLineToHorizontalAbs);
    writeFloat(x);
}

void SVGPathByteStreamBuilder::lineToVertical(float y, PathCoordinateMode mode)
{
    ASSERT(m_byteStream);
    writeSegmentType(mode == RelativeCoordinates ? PathSegLineToVerticalRel : PathSegLineToVerticalAbs);
    writeFloat(y);
}

void SVGPathByteStreamBuilder::curveToCubic(const FloatPoint& point1, const FloatPoint& point2, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    ASSERT(m_byteStream);
    writeSegmentType(mode == RelativeCoordinates ? PathSegCurveToCubicRel : PathSegCurveToCubicAbs);
    writeFloatPoint(point1);
    writeFloatPoint(point2);
    writeFloatPoint(targetPoint);
}

void SVGPathByteStreamBuilder::curveToCubicSmooth(const FloatPoint& point2, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    ASSERT(m_byteStream);
    writeSegmentType(mode == RelativeCoordinates ? PathSegCurveToCubicSmoothRel : PathSegCurveToCubicSmoothAbs);
    writeFloatPoint(point2);
    writeFloatPoint(targetPoint);
}

void SVGPathByteStreamBuilder::curveToQuadratic(const FloatPoint& point1, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    ASSERT(m_byteStream);
    writeSegmentType(mode == RelativeCoordinates ? PathSegCurveToQuadraticRel : PathSegCurveToQuadraticAbs);
    writeFloatPoint(point1);
    writeFloatPoint(targetPoint);
}

void SVGPathByteStreamBuilder::curveToQuadraticSmooth(const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    ASSERT(m_byteStream);
    writeSegmentType(mode == RelativeCoordinates ? PathSegCurveToQuadraticSmoothRel : PathSegCurveToQuadraticSmoothAbs);
    writeFloatPoint(targetPoint);
}

void SVGPathByteStreamBuilder::arcTo(float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    ASSERT(m_byteStream);
    writeSegmentType(mode == RelativeCoordinates ? PathSegArcRel : PathSegArcAbs);
    writeFloat(r1);
    writeFloat(r2);
    writeFloat(angle);
    writeFlag(largeArcFlag);
    writeFlag(sweepFlag);
    writeFloatPoint(targetPoint);
}

void SVGPathByteStreamBuilder::closePath()
{
    ASSERT(m_byteStream);
    writeSegmentType(PathSegClosePath);
}

} // namespace WebCore

#endif // ENABLE(SVG)
