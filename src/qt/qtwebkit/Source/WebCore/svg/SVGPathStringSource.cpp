/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "SVGPathStringSource.h"

#include "FloatPoint.h"
#include "SVGParserUtilities.h"

namespace WebCore {

SVGPathStringSource::SVGPathStringSource(const String& string)
    : m_string(string)
    , m_is8BitSource(string.is8Bit())
{
    ASSERT(!string.isEmpty());

    if (m_is8BitSource) {
        m_current.m_character8 = string.characters8();
        m_end.m_character8 = m_current.m_character8 + string.length();
    } else {
        m_current.m_character16 = string.characters16();
        m_end.m_character16 = m_current.m_character16 + string.length();
    }
}

bool SVGPathStringSource::hasMoreData() const
{
    if (m_is8BitSource)
        return m_current.m_character8 < m_end.m_character8;
    return m_current.m_character16 < m_end.m_character16;
}

bool SVGPathStringSource::moveToNextToken()
{
    if (m_is8BitSource)
        return skipOptionalSVGSpaces(m_current.m_character8, m_end.m_character8);
    return skipOptionalSVGSpaces(m_current.m_character16, m_end.m_character16);
}

template <typename CharacterType>
static bool parseSVGSegmentTypeHelper(const CharacterType*& current, SVGPathSegType& pathSegType)
{
    switch (*(current++)) {
    case 'Z':
    case 'z':
        pathSegType = PathSegClosePath;
        break;
    case 'M':
        pathSegType = PathSegMoveToAbs;
        break;
    case 'm':
        pathSegType = PathSegMoveToRel;
        break;
    case 'L':
        pathSegType = PathSegLineToAbs;
        break;
    case 'l':
        pathSegType = PathSegLineToRel;
        break;
    case 'C':
        pathSegType = PathSegCurveToCubicAbs;
        break;
    case 'c':
        pathSegType = PathSegCurveToCubicRel;
        break;
    case 'Q':
        pathSegType = PathSegCurveToQuadraticAbs;
        break;
    case 'q':
        pathSegType = PathSegCurveToQuadraticRel;
        break;
    case 'A':
        pathSegType = PathSegArcAbs;
        break;
    case 'a':
        pathSegType = PathSegArcRel;
        break;
    case 'H':
        pathSegType = PathSegLineToHorizontalAbs;
        break;
    case 'h':
        pathSegType = PathSegLineToHorizontalRel;
        break;
    case 'V':
        pathSegType = PathSegLineToVerticalAbs;
        break;
    case 'v':
        pathSegType = PathSegLineToVerticalRel;
        break;
    case 'S':
        pathSegType = PathSegCurveToCubicSmoothAbs;
        break;
    case 's':
        pathSegType = PathSegCurveToCubicSmoothRel;
        break;
    case 'T':
        pathSegType = PathSegCurveToQuadraticSmoothAbs;
        break;
    case 't':
        pathSegType = PathSegCurveToQuadraticSmoothRel;
        break;
    default:
        pathSegType = PathSegUnknown;
    }
    return true;
}

bool SVGPathStringSource::parseSVGSegmentType(SVGPathSegType& pathSegType)
{
    if (m_is8BitSource)
        return parseSVGSegmentTypeHelper(m_current.m_character8, pathSegType);
    return parseSVGSegmentTypeHelper(m_current.m_character16, pathSegType);
}

template <typename CharacterType>
static bool nextCommandHelper(const CharacterType*& current, SVGPathSegType previousCommand, SVGPathSegType& nextCommand)
{
    // Check for remaining coordinates in the current command.
    if ((*current == '+' || *current == '-' || *current == '.' || (*current >= '0' && *current <= '9'))
        && previousCommand != PathSegClosePath) {
        if (previousCommand == PathSegMoveToAbs) {
            nextCommand = PathSegLineToAbs;
            return true;
        }
        if (previousCommand == PathSegMoveToRel) {
            nextCommand = PathSegLineToRel;
            return true;
        }
        nextCommand = previousCommand;
        return true;
    }

    return false;
}

SVGPathSegType SVGPathStringSource::nextCommand(SVGPathSegType previousCommand)
{
    SVGPathSegType nextCommand;
    if (m_is8BitSource) {
        if (nextCommandHelper(m_current.m_character8, previousCommand, nextCommand))
            return nextCommand;
    } else {
        if (nextCommandHelper(m_current.m_character16, previousCommand, nextCommand))
            return nextCommand;
    }

    parseSVGSegmentType(nextCommand);
    return nextCommand;
}

bool SVGPathStringSource::parseMoveToSegment(FloatPoint& targetPoint)
{
    if (m_is8BitSource)
        return parseFloatPoint(m_current.m_character8, m_end.m_character8, targetPoint);
    return parseFloatPoint(m_current.m_character16, m_end.m_character16, targetPoint);
}

bool SVGPathStringSource::parseLineToSegment(FloatPoint& targetPoint)
{
    if (m_is8BitSource)
        return parseFloatPoint(m_current.m_character8, m_end.m_character8, targetPoint);
    return parseFloatPoint(m_current.m_character16, m_end.m_character16, targetPoint);
}

bool SVGPathStringSource::parseLineToHorizontalSegment(float& x)
{
    if (m_is8BitSource)
        return parseNumber(m_current.m_character8, m_end.m_character8, x);
    return parseNumber(m_current.m_character16, m_end.m_character16, x);
}

bool SVGPathStringSource::parseLineToVerticalSegment(float& y)
{
    if (m_is8BitSource)
        return parseNumber(m_current.m_character8, m_end.m_character8, y);
    return parseNumber(m_current.m_character16, m_end.m_character16, y);
}

bool SVGPathStringSource::parseCurveToCubicSegment(FloatPoint& point1, FloatPoint& point2, FloatPoint& targetPoint)
{
    if (m_is8BitSource)
        return parseFloatPoint3(m_current.m_character8, m_end.m_character8, point1, point2, targetPoint);
    return parseFloatPoint3(m_current.m_character16, m_end.m_character16, point1, point2, targetPoint);
}

bool SVGPathStringSource::parseCurveToCubicSmoothSegment(FloatPoint& point1, FloatPoint& targetPoint)
{
    if (m_is8BitSource)
        return parseFloatPoint2(m_current.m_character8, m_end.m_character8, point1, targetPoint);
    return parseFloatPoint2(m_current.m_character16, m_end.m_character16, point1, targetPoint);
}

bool SVGPathStringSource::parseCurveToQuadraticSegment(FloatPoint& point2, FloatPoint& targetPoint)
{
    if (m_is8BitSource)
        return parseFloatPoint2(m_current.m_character8, m_end.m_character8, point2, targetPoint);
    return parseFloatPoint2(m_current.m_character16, m_end.m_character16, point2, targetPoint);
}

bool SVGPathStringSource::parseCurveToQuadraticSmoothSegment(FloatPoint& targetPoint)
{
    if (m_is8BitSource)
        return parseFloatPoint(m_current.m_character8, m_end.m_character8, targetPoint);
    return parseFloatPoint(m_current.m_character16, m_end.m_character16, targetPoint);
}

template <typename CharacterType>
static bool parseArcToSegmentHelper(const CharacterType*& current, const CharacterType* end, float& rx, float& ry, float& angle, bool& largeArc, bool& sweep, FloatPoint& targetPoint)
{
    float toX;
    float toY;
    if (!parseNumber(current, end, rx)
        || !parseNumber(current, end, ry)
        || !parseNumber(current, end, angle)
        || !parseArcFlag(current, end, largeArc)
        || !parseArcFlag(current, end, sweep)
        || !parseNumber(current, end, toX)
        || !parseNumber(current, end, toY))
        return false;
    targetPoint = FloatPoint(toX, toY);
    return true;
}

bool SVGPathStringSource::parseArcToSegment(float& rx, float& ry, float& angle, bool& largeArc, bool& sweep, FloatPoint& targetPoint)
{
    if (m_is8BitSource)
        return parseArcToSegmentHelper(m_current.m_character8, m_end.m_character8, rx, ry, angle, largeArc, sweep, targetPoint);
    return parseArcToSegmentHelper(m_current.m_character16, m_end.m_character16, rx, ry, angle, largeArc, sweep, targetPoint);
}

} // namespace WebKit

#endif // ENABLE(SVG)
