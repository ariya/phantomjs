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
#include "SVGPathStringBuilder.h"

namespace WebCore {

String SVGPathStringBuilder::result()
{
    unsigned size = m_stringBuilder.length();
    if (!size)
        return String();

    // Remove trailing space.
    m_stringBuilder.resize(size - 1);
    return m_stringBuilder.toString();
}

void SVGPathStringBuilder::moveTo(const FloatPoint& targetPoint, bool, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates)
        m_stringBuilder.append(String::format("M %.6lg %.6lg ", targetPoint.x(), targetPoint.y()));
    else
        m_stringBuilder.append(String::format("m %.6lg %.6lg ", targetPoint.x(), targetPoint.y()));
}

void SVGPathStringBuilder::lineTo(const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates)
        m_stringBuilder.append(String::format("L %.6lg %.6lg ", targetPoint.x(), targetPoint.y()));
    else
        m_stringBuilder.append(String::format("l %.6lg %.6lg ", targetPoint.x(), targetPoint.y()));
}

void SVGPathStringBuilder::lineToHorizontal(float x, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates)
        m_stringBuilder.append(String::format("H %.6lg ", x));
    else
        m_stringBuilder.append(String::format("h %.6lg ", x));
}

void SVGPathStringBuilder::lineToVertical(float y, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates)
        m_stringBuilder.append(String::format("V %.6lg ", y));
    else
        m_stringBuilder.append(String::format("v %.6lg ", y));
}

void SVGPathStringBuilder::curveToCubic(const FloatPoint& point1, const FloatPoint& point2, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates)
        m_stringBuilder.append(String::format("C %.6lg %.6lg %.6lg %.6lg %.6lg %.6lg ", point1.x(), point1.y(), point2.x(), point2.y(), targetPoint.x(), targetPoint.y()));
    else
        m_stringBuilder.append(String::format("c %.6lg %.6lg %.6lg %.6lg %.6lg %.6lg ", point1.x(), point1.y(), point2.x(), point2.y(), targetPoint.x(), targetPoint.y()));
}

void SVGPathStringBuilder::curveToCubicSmooth(const FloatPoint& point2, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates)
        m_stringBuilder.append(String::format("S %.6lg %.6lg %.6lg %.6lg ", point2.x(), point2.y(), targetPoint.x(), targetPoint.y()));
    else
        m_stringBuilder.append(String::format("s %.6lg %.6lg %.6lg %.6lg ", point2.x(), point2.y(), targetPoint.x(), targetPoint.y()));
}

void SVGPathStringBuilder::curveToQuadratic(const FloatPoint& point1, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates)
        m_stringBuilder.append(String::format("Q %.6lg %.6lg %.6lg %.6lg ", point1.x(), point1.y(), targetPoint.x(), targetPoint.y()));
    else
        m_stringBuilder.append(String::format("q %.6lg %.6lg %.6lg %.6lg ", point1.x(), point1.y(), targetPoint.x(), targetPoint.y()));
}

void SVGPathStringBuilder::curveToQuadraticSmooth(const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates)
        m_stringBuilder.append(String::format("T %.6lg %.6lg ", targetPoint.x(), targetPoint.y()));
    else
        m_stringBuilder.append(String::format("t %.6lg %.6lg ", targetPoint.x(), targetPoint.y()));
}

void SVGPathStringBuilder::arcTo(float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates)
        m_stringBuilder.append(String::format("A %.6lg %.6lg %.6lg %d %d %.6lg %.6lg ", r1, r2, angle, largeArcFlag, sweepFlag, targetPoint.x(), targetPoint.y()));
    else
        m_stringBuilder.append(String::format("a %.6lg %.6lg %.6lg %d %d %.6lg %.6lg ", r1, r2, angle, largeArcFlag, sweepFlag, targetPoint.x(), targetPoint.y()));
}

void SVGPathStringBuilder::closePath()
{
    m_stringBuilder.append("Z ");
}

} // namespace WebCore

#endif // ENABLE(SVG)
