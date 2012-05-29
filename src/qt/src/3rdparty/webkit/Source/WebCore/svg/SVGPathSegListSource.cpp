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
#include "SVGPathSegListSource.h"

#include "SVGPathSegArc.h"
#include "SVGPathSegCurvetoCubic.h"
#include "SVGPathSegCurvetoCubicSmooth.h"
#include "SVGPathSegCurvetoQuadratic.h"
#include "SVGPathSegLinetoHorizontal.h"
#include "SVGPathSegLinetoVertical.h"

namespace WebCore {

SVGPathSegListSource::SVGPathSegListSource(const SVGPathSegList& pathSegList)
    : m_pathSegList(pathSegList)
{
    m_itemCurrent = 0;
    m_itemEnd = m_pathSegList.size();
}

bool SVGPathSegListSource::hasMoreData() const
{
    return m_itemCurrent < m_itemEnd;
}

bool SVGPathSegListSource::parseSVGSegmentType(SVGPathSegType& pathSegType)
{
    m_segment = m_pathSegList.at(m_itemCurrent);
    pathSegType = static_cast<SVGPathSegType>(m_segment->pathSegType());
    ++m_itemCurrent;
    return true;
}

SVGPathSegType SVGPathSegListSource::nextCommand(SVGPathSegType)
{
    m_segment = m_pathSegList.at(m_itemCurrent);
    SVGPathSegType pathSegType = static_cast<SVGPathSegType>(m_segment->pathSegType());
    ++m_itemCurrent;
    return pathSegType;
}

bool SVGPathSegListSource::parseMoveToSegment(FloatPoint& targetPoint)
{
    ASSERT(m_segment);
    ASSERT(m_segment->pathSegType() == PathSegMoveToAbs || m_segment->pathSegType() == PathSegMoveToRel);
    SVGPathSegSingleCoordinate* moveTo = static_cast<SVGPathSegSingleCoordinate*>(m_segment.get());
    targetPoint = FloatPoint(moveTo->x(), moveTo->y());
    return true;
}

bool SVGPathSegListSource::parseLineToSegment(FloatPoint& targetPoint)
{
    ASSERT(m_segment);
    ASSERT(m_segment->pathSegType() == PathSegLineToAbs || m_segment->pathSegType() == PathSegLineToRel);
    SVGPathSegSingleCoordinate* lineTo = static_cast<SVGPathSegSingleCoordinate*>(m_segment.get());
    targetPoint = FloatPoint(lineTo->x(), lineTo->y());
    return true;
}

bool SVGPathSegListSource::parseLineToHorizontalSegment(float& x)
{
    ASSERT(m_segment);
    ASSERT(m_segment->pathSegType() == PathSegLineToHorizontalAbs || m_segment->pathSegType() == PathSegLineToHorizontalRel);
    SVGPathSegLinetoHorizontal* horizontal = static_cast<SVGPathSegLinetoHorizontal*>(m_segment.get());
    x = horizontal->x();
    return true;
}

bool SVGPathSegListSource::parseLineToVerticalSegment(float& y)
{
    ASSERT(m_segment);
    ASSERT(m_segment->pathSegType() == PathSegLineToVerticalAbs || m_segment->pathSegType() == PathSegLineToVerticalRel);
    SVGPathSegLinetoVertical* vertical = static_cast<SVGPathSegLinetoVertical*>(m_segment.get());
    y = vertical->y();
    return true;
}

bool SVGPathSegListSource::parseCurveToCubicSegment(FloatPoint& point1, FloatPoint& point2, FloatPoint& targetPoint)
{
    ASSERT(m_segment);
    ASSERT(m_segment->pathSegType() == PathSegCurveToCubicAbs || m_segment->pathSegType() == PathSegCurveToCubicRel);
    SVGPathSegCurvetoCubic* cubic = static_cast<SVGPathSegCurvetoCubic*>(m_segment.get());
    point1 = FloatPoint(cubic->x1(), cubic->y1());
    point2 = FloatPoint(cubic->x2(), cubic->y2());
    targetPoint = FloatPoint(cubic->x(), cubic->y());
    return true;
}

bool SVGPathSegListSource::parseCurveToCubicSmoothSegment(FloatPoint& point2, FloatPoint& targetPoint)
{
    ASSERT(m_segment);
    ASSERT(m_segment->pathSegType() == PathSegCurveToCubicSmoothAbs || m_segment->pathSegType() == PathSegCurveToCubicSmoothRel);
    SVGPathSegCurvetoCubicSmooth* cubicSmooth = static_cast<SVGPathSegCurvetoCubicSmooth*>(m_segment.get());
    point2 = FloatPoint(cubicSmooth->x2(), cubicSmooth->y2());
    targetPoint = FloatPoint(cubicSmooth->x(), cubicSmooth->y());
    return true;
}

bool SVGPathSegListSource::parseCurveToQuadraticSegment(FloatPoint& point1, FloatPoint& targetPoint)
{
    ASSERT(m_segment);
    ASSERT(m_segment->pathSegType() == PathSegCurveToQuadraticAbs || m_segment->pathSegType() == PathSegCurveToQuadraticRel);
    SVGPathSegCurvetoQuadratic* quadratic = static_cast<SVGPathSegCurvetoQuadratic*>(m_segment.get());
    point1 = FloatPoint(quadratic->x1(), quadratic->y1());
    targetPoint = FloatPoint(quadratic->x(), quadratic->y());
    return true;
}

bool SVGPathSegListSource::parseCurveToQuadraticSmoothSegment(FloatPoint& targetPoint)
{
    ASSERT(m_segment);
    ASSERT(m_segment->pathSegType() == PathSegCurveToQuadraticSmoothAbs || m_segment->pathSegType() == PathSegCurveToQuadraticSmoothRel);
    SVGPathSegSingleCoordinate* quadraticSmooth = static_cast<SVGPathSegSingleCoordinate*>(m_segment.get());
    targetPoint = FloatPoint(quadraticSmooth->x(), quadraticSmooth->y());
    return true;
}

bool SVGPathSegListSource::parseArcToSegment(float& rx, float& ry, float& angle, bool& largeArc, bool& sweep, FloatPoint& targetPoint)
{
    ASSERT(m_segment);
    ASSERT(m_segment->pathSegType() == PathSegArcAbs || m_segment->pathSegType() == PathSegArcRel);
    SVGPathSegArc* arcTo = static_cast<SVGPathSegArc*>(m_segment.get());
    rx = arcTo->r1();
    ry = arcTo->r2();
    angle = arcTo->angle();
    largeArc = arcTo->largeArcFlag();
    sweep = arcTo->sweepFlag();
    targetPoint = FloatPoint(arcTo->x(), arcTo->y());
    return true;
}

}

#endif // ENABLE(SVG)
