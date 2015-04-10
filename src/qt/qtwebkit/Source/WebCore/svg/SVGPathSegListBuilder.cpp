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

#include "config.h"

#if ENABLE(SVG)
#include "SVGPathSegListBuilder.h"

#include "ExceptionCode.h"
#include "SVGPathElement.h"
#include "SVGPathSegArcAbs.h"
#include "SVGPathSegArcRel.h"
#include "SVGPathSegClosePath.h"
#include "SVGPathSegCurvetoCubicAbs.h"
#include "SVGPathSegCurvetoCubicRel.h"
#include "SVGPathSegCurvetoCubicSmoothAbs.h"
#include "SVGPathSegCurvetoCubicSmoothRel.h"
#include "SVGPathSegCurvetoQuadraticAbs.h"
#include "SVGPathSegCurvetoQuadraticRel.h"
#include "SVGPathSegCurvetoQuadraticSmoothAbs.h"
#include "SVGPathSegCurvetoQuadraticSmoothRel.h"
#include "SVGPathSegLinetoAbs.h"
#include "SVGPathSegLinetoHorizontalAbs.h"
#include "SVGPathSegLinetoHorizontalRel.h"
#include "SVGPathSegLinetoRel.h"
#include "SVGPathSegLinetoVerticalAbs.h"
#include "SVGPathSegLinetoVerticalRel.h"
#include "SVGPathSegList.h"
#include "SVGPathSegMovetoAbs.h"
#include "SVGPathSegMovetoRel.h"

namespace WebCore {

SVGPathSegListBuilder::SVGPathSegListBuilder()
    : m_pathElement(0)
    , m_pathSegList(0)
    , m_pathSegRole(PathSegUndefinedRole)
{
}

void SVGPathSegListBuilder::moveTo(const FloatPoint& targetPoint, bool, PathCoordinateMode mode)
{
    ASSERT(m_pathElement);
    ASSERT(m_pathSegList);
    if (mode == AbsoluteCoordinates)
        m_pathSegList->append(m_pathElement->createSVGPathSegMovetoAbs(targetPoint.x(), targetPoint.y(), m_pathSegRole));
    else
        m_pathSegList->append(m_pathElement->createSVGPathSegMovetoRel(targetPoint.x(), targetPoint.y(), m_pathSegRole));
}

void SVGPathSegListBuilder::lineTo(const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    ASSERT(m_pathElement);
    ASSERT(m_pathSegList);
    if (mode == AbsoluteCoordinates)
        m_pathSegList->append(m_pathElement->createSVGPathSegLinetoAbs(targetPoint.x(), targetPoint.y(), m_pathSegRole));
    else
        m_pathSegList->append(m_pathElement->createSVGPathSegLinetoRel(targetPoint.x(), targetPoint.y(), m_pathSegRole));
}

void SVGPathSegListBuilder::lineToHorizontal(float x, PathCoordinateMode mode)
{
    ASSERT(m_pathElement);
    ASSERT(m_pathSegList);
    if (mode == AbsoluteCoordinates)
        m_pathSegList->append(m_pathElement->createSVGPathSegLinetoHorizontalAbs(x, m_pathSegRole));
    else
        m_pathSegList->append(m_pathElement->createSVGPathSegLinetoHorizontalRel(x, m_pathSegRole));
}

void SVGPathSegListBuilder::lineToVertical(float y, PathCoordinateMode mode)
{
    ASSERT(m_pathElement);
    ASSERT(m_pathSegList);
    if (mode == AbsoluteCoordinates)
        m_pathSegList->append(m_pathElement->createSVGPathSegLinetoVerticalAbs(y, m_pathSegRole));
    else
        m_pathSegList->append(m_pathElement->createSVGPathSegLinetoVerticalRel(y, m_pathSegRole));
}

void SVGPathSegListBuilder::curveToCubic(const FloatPoint& point1, const FloatPoint& point2, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    ASSERT(m_pathElement);
    ASSERT(m_pathSegList);
    if (mode == AbsoluteCoordinates)
        m_pathSegList->append(m_pathElement->createSVGPathSegCurvetoCubicAbs(targetPoint.x(), targetPoint.y(), point1.x(), point1.y(), point2.x(), point2.y(), m_pathSegRole));
    else
        m_pathSegList->append(m_pathElement->createSVGPathSegCurvetoCubicRel(targetPoint.x(), targetPoint.y(), point1.x(), point1.y(), point2.x(), point2.y(), m_pathSegRole));
}

void SVGPathSegListBuilder::curveToCubicSmooth(const FloatPoint& point2, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    ASSERT(m_pathElement);
    ASSERT(m_pathSegList);
    if (mode == AbsoluteCoordinates)
        m_pathSegList->append(m_pathElement->createSVGPathSegCurvetoCubicSmoothAbs(targetPoint.x(), targetPoint.y(), point2.x(), point2.y(), m_pathSegRole));
    else
        m_pathSegList->append(m_pathElement->createSVGPathSegCurvetoCubicSmoothRel(targetPoint.x(), targetPoint.y(), point2.x(), point2.y(), m_pathSegRole));
}

void SVGPathSegListBuilder::curveToQuadratic(const FloatPoint& point1, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    ASSERT(m_pathElement);
    ASSERT(m_pathSegList);
    if (mode == AbsoluteCoordinates)
        m_pathSegList->append(m_pathElement->createSVGPathSegCurvetoQuadraticAbs(targetPoint.x(), targetPoint.y(), point1.x(), point1.y(), m_pathSegRole));
    else
        m_pathSegList->append(m_pathElement->createSVGPathSegCurvetoQuadraticRel(targetPoint.x(), targetPoint.y(), point1.x(), point1.y(), m_pathSegRole));
}

void SVGPathSegListBuilder::curveToQuadraticSmooth(const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    ASSERT(m_pathElement);
    ASSERT(m_pathSegList);
    if (mode == AbsoluteCoordinates)
        m_pathSegList->append(m_pathElement->createSVGPathSegCurvetoQuadraticSmoothAbs(targetPoint.x(), targetPoint.y(), m_pathSegRole));
    else
        m_pathSegList->append(m_pathElement->createSVGPathSegCurvetoQuadraticSmoothRel(targetPoint.x(), targetPoint.y(), m_pathSegRole));
}

void SVGPathSegListBuilder::arcTo(float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    ASSERT(m_pathElement);
    ASSERT(m_pathSegList);
    if (mode == AbsoluteCoordinates)
        m_pathSegList->append(m_pathElement->createSVGPathSegArcAbs(targetPoint.x(), targetPoint.y(), r1, r2, angle, largeArcFlag, sweepFlag, m_pathSegRole));
    else
        m_pathSegList->append(m_pathElement->createSVGPathSegArcRel(targetPoint.x(), targetPoint.y(), r1, r2, angle, largeArcFlag, sweepFlag, m_pathSegRole));
}

void SVGPathSegListBuilder::closePath()
{
    ASSERT(m_pathElement);
    ASSERT(m_pathSegList);
    m_pathSegList->append(m_pathElement->createSVGPathSegClosePath(m_pathSegRole));
}

}

#endif // ENABLE(SVG)
