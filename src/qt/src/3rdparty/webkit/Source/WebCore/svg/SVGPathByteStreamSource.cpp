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
#include "SVGPathByteStreamSource.h"

namespace WebCore {

SVGPathByteStreamSource::SVGPathByteStreamSource(SVGPathByteStream* stream)
    : m_stream(stream)
{
    ASSERT(stream);
    m_streamCurrent = stream->begin();
    m_streamEnd = stream->end();
}

bool SVGPathByteStreamSource::hasMoreData() const
{
    return m_streamCurrent < m_streamEnd;
}

bool SVGPathByteStreamSource::parseSVGSegmentType(SVGPathSegType& pathSegType)
{
    pathSegType = static_cast<SVGPathSegType>(readSVGSegmentType());
    return true;
}

SVGPathSegType SVGPathByteStreamSource::nextCommand(SVGPathSegType)
{
    return static_cast<SVGPathSegType>(readSVGSegmentType());
}

bool SVGPathByteStreamSource::parseMoveToSegment(FloatPoint& targetPoint)
{
    targetPoint = readFloatPoint();
    return true;
}

bool SVGPathByteStreamSource::parseLineToSegment(FloatPoint& targetPoint)
{
    targetPoint = readFloatPoint();
    return true;
}

bool SVGPathByteStreamSource::parseLineToHorizontalSegment(float& x)
{
    x = readFloat();
    return true;
}

bool SVGPathByteStreamSource::parseLineToVerticalSegment(float& y)
{
    y = readFloat();
    return true;
}

bool SVGPathByteStreamSource::parseCurveToCubicSegment(FloatPoint& point1, FloatPoint& point2, FloatPoint& targetPoint)
{
    point1 = readFloatPoint();
    point2 = readFloatPoint();
    targetPoint = readFloatPoint();
    return true;
}

bool SVGPathByteStreamSource::parseCurveToCubicSmoothSegment(FloatPoint& point2, FloatPoint& targetPoint)
{
    point2 = readFloatPoint();
    targetPoint = readFloatPoint();
    return true;
}

bool SVGPathByteStreamSource::parseCurveToQuadraticSegment(FloatPoint& point1, FloatPoint& targetPoint)
{
    point1 = readFloatPoint();
    targetPoint = readFloatPoint();
    return true;
}

bool SVGPathByteStreamSource::parseCurveToQuadraticSmoothSegment(FloatPoint& targetPoint)
{
    targetPoint = readFloatPoint();
    return true;
}

bool SVGPathByteStreamSource::parseArcToSegment(float& rx, float& ry, float& angle, bool& largeArc, bool& sweep, FloatPoint& targetPoint)
{
    rx = readFloat();
    ry = readFloat();
    angle = readFloat();
    largeArc = readFlag();
    sweep = readFlag();
    targetPoint = readFloatPoint();
    return true;
}

}

#endif // ENABLE(SVG)
