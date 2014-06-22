/*
 * Copyright (C) Research In Motion Limited 2010-2011. All rights reserved.
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
#include <wtf/text/WTFString.h>

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
        m_stringBuilder.append("M " + String::number(targetPoint.x()) + ' ' + String::number(targetPoint.y()) + ' ');
    else
        m_stringBuilder.append("m " + String::number(targetPoint.x()) + ' ' + String::number(targetPoint.y()) + ' ');
}

void SVGPathStringBuilder::lineTo(const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates)
        m_stringBuilder.append("L " + String::number(targetPoint.x()) + ' ' + String::number(targetPoint.y()) + ' ');
    else
        m_stringBuilder.append("l " + String::number(targetPoint.x()) + ' ' + String::number(targetPoint.y()) + ' ');
}

void SVGPathStringBuilder::lineToHorizontal(float x, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates)
        m_stringBuilder.append("H " + String::number(x) + ' ');
    else
        m_stringBuilder.append("h " + String::number(x) + ' ');
}

void SVGPathStringBuilder::lineToVertical(float y, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates)
        m_stringBuilder.append("V " + String::number(y) + ' ');
    else
        m_stringBuilder.append("v " + String::number(y) + ' ');
}

void SVGPathStringBuilder::curveToCubic(const FloatPoint& point1, const FloatPoint& point2, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates) {
        m_stringBuilder.append("C " + String::number(point1.x()) + ' ' + String::number(point1.y())
                              + ' ' + String::number(point2.x()) + ' ' + String::number(point2.y())
                              + ' ' + String::number(targetPoint.x()) + ' ' + String::number(targetPoint.y()) + ' ');
        return;
    }

    m_stringBuilder.append("c " + String::number(point1.x()) + ' ' + String::number(point1.y())
                          + ' ' + String::number(point2.x()) + ' ' + String::number(point2.y())
                          + ' ' + String::number(targetPoint.x()) + ' ' + String::number(targetPoint.y()) + ' ');
}

void SVGPathStringBuilder::curveToCubicSmooth(const FloatPoint& point2, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates) {
        m_stringBuilder.append("S " + String::number(point2.x()) + ' ' + String::number(point2.y())
                              + ' ' + String::number(targetPoint.x()) + ' ' + String::number(targetPoint.y()) + ' ');
        return;
    }

    m_stringBuilder.append("s " + String::number(point2.x()) + ' ' + String::number(point2.y())
                          + ' ' + String::number(targetPoint.x()) + ' ' + String::number(targetPoint.y()) + ' ');
}

void SVGPathStringBuilder::curveToQuadratic(const FloatPoint& point1, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates) {
        m_stringBuilder.append("Q " + String::number(point1.x()) + ' ' + String::number(point1.y())
                              + ' ' + String::number(targetPoint.x()) + ' ' + String::number(targetPoint.y()) + ' ');
        return;
    }

    m_stringBuilder.append("q " + String::number(point1.x()) + ' ' + String::number(point1.y())
                          + ' ' + String::number(targetPoint.x()) + ' ' + String::number(targetPoint.y()) + ' ');
}

void SVGPathStringBuilder::curveToQuadraticSmooth(const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates)
        m_stringBuilder.append("T " + String::number(targetPoint.x()) + ' ' + String::number(targetPoint.y()) + ' ');
    else
        m_stringBuilder.append("t " + String::number(targetPoint.x()) + ' ' + String::number(targetPoint.y()) + ' ');
}

void SVGPathStringBuilder::arcTo(float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    if (mode == AbsoluteCoordinates) {
        m_stringBuilder.append("A " + String::number(r1) + ' ' + String::number(r2)
                              + ' ' + String::number(angle) + ' ' + String::number(largeArcFlag) + ' ' + String::number(sweepFlag)
                              + ' ' + String::number(targetPoint.x()) + ' ' + String::number(targetPoint.y()) + ' ');
        return;
    }

    m_stringBuilder.append("a " + String::number(r1) + ' ' + String::number(r2)
                          + ' ' + String::number(angle) + ' ' + String::number(largeArcFlag) + ' ' + String::number(sweepFlag)
                          + ' ' + String::number(targetPoint.x()) + ' ' + String::number(targetPoint.y()) + ' ');
}

void SVGPathStringBuilder::closePath()
{
    m_stringBuilder.append("Z ");
}

} // namespace WebCore

#endif // ENABLE(SVG)
