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
#include "SVGPathParser.h"

#include "AffineTransform.h"
#include "SVGPathSource.h"
#include <wtf/MathExtras.h>

static const float gOneOverThree = 1 / 3.f;

namespace WebCore {

SVGPathParser::SVGPathParser()
    : m_consumer(0)
{
}

void SVGPathParser::parseClosePathSegment()
{
    // Reset m_currentPoint for the next path.
    if (m_pathParsingMode == NormalizedParsing)
        m_currentPoint = m_subPathPoint;
    m_closePath = true;
    m_consumer->closePath();
}

bool SVGPathParser::parseMoveToSegment()
{
    FloatPoint targetPoint;
    if (!m_source->parseMoveToSegment(targetPoint))
        return false;

    if (m_pathParsingMode == NormalizedParsing) {
        if (m_mode == RelativeCoordinates)
            m_currentPoint += targetPoint;
        else
            m_currentPoint = targetPoint;
        m_subPathPoint = m_currentPoint;
        m_consumer->moveTo(m_currentPoint, m_closePath, AbsoluteCoordinates);
    } else
        m_consumer->moveTo(targetPoint, m_closePath, m_mode);
    m_closePath = false;
    return true;
}

bool SVGPathParser::parseLineToSegment()
{
    FloatPoint targetPoint;
    if (!m_source->parseLineToSegment(targetPoint))
        return false;

    if (m_pathParsingMode == NormalizedParsing) {
        if (m_mode == RelativeCoordinates)
            m_currentPoint += targetPoint;
        else
            m_currentPoint = targetPoint;
        m_consumer->lineTo(m_currentPoint, AbsoluteCoordinates);
    } else
        m_consumer->lineTo(targetPoint, m_mode);
    return true;
}

bool SVGPathParser::parseLineToHorizontalSegment()
{
    float toX;
    if (!m_source->parseLineToHorizontalSegment(toX))
        return false;

    if (m_pathParsingMode == NormalizedParsing) {
        if (m_mode == RelativeCoordinates)
            m_currentPoint.move(toX, 0);
        else
            m_currentPoint.setX(toX);
        m_consumer->lineTo(m_currentPoint, AbsoluteCoordinates);
    } else
        m_consumer->lineToHorizontal(toX, m_mode);
    return true;
}

bool SVGPathParser::parseLineToVerticalSegment()
{
    float toY;
    if (!m_source->parseLineToVerticalSegment(toY))
        return false;

    if (m_pathParsingMode == NormalizedParsing) {
        if (m_mode == RelativeCoordinates)
            m_currentPoint.move(0, toY);
        else
            m_currentPoint.setY(toY);
        m_consumer->lineTo(m_currentPoint, AbsoluteCoordinates);
    } else
        m_consumer->lineToVertical(toY, m_mode);
    return true;
}

bool SVGPathParser::parseCurveToCubicSegment()
{
    FloatPoint point1;
    FloatPoint point2;
    FloatPoint targetPoint;
    if (!m_source->parseCurveToCubicSegment(point1, point2, targetPoint))
        return false;

    if (m_pathParsingMode == NormalizedParsing) {
        if (m_mode == RelativeCoordinates) {
            point1 += m_currentPoint;
            point2 += m_currentPoint;
            targetPoint += m_currentPoint;
        }
        m_consumer->curveToCubic(point1, point2, targetPoint, AbsoluteCoordinates);

        m_controlPoint = point2;
        m_currentPoint = targetPoint;
    } else
        m_consumer->curveToCubic(point1, point2, targetPoint, m_mode);
    return true;
}

bool SVGPathParser::parseCurveToCubicSmoothSegment()
{
    FloatPoint point2;
    FloatPoint targetPoint;
    if (!m_source->parseCurveToCubicSmoothSegment(point2, targetPoint))
        return false;

    if (m_lastCommand != PathSegCurveToCubicAbs
        && m_lastCommand != PathSegCurveToCubicRel
        && m_lastCommand != PathSegCurveToCubicSmoothAbs
        && m_lastCommand != PathSegCurveToCubicSmoothRel)
        m_controlPoint = m_currentPoint;

    if (m_pathParsingMode == NormalizedParsing) {
        FloatPoint point1 = m_currentPoint;
        point1.scale(2, 2);
        point1.move(-m_controlPoint.x(), -m_controlPoint.y());
        if (m_mode == RelativeCoordinates) {
            point2 += m_currentPoint;
            targetPoint += m_currentPoint;
        }

        m_consumer->curveToCubic(point1, point2, targetPoint, AbsoluteCoordinates);

        m_controlPoint = point2;
        m_currentPoint = targetPoint;
    } else
        m_consumer->curveToCubicSmooth(point2, targetPoint, m_mode);
    return true;
}

bool SVGPathParser::parseCurveToQuadraticSegment()
{
    FloatPoint point1;
    FloatPoint targetPoint;
    if (!m_source->parseCurveToQuadraticSegment(point1, targetPoint))
        return false;

    if (m_pathParsingMode == NormalizedParsing) {
        m_controlPoint = point1;
        FloatPoint point1 = m_currentPoint;
        point1.move(2 * m_controlPoint.x(), 2 * m_controlPoint.y());
        FloatPoint point2(targetPoint.x() + 2 * m_controlPoint.x(), targetPoint.y() + 2 * m_controlPoint.y());
        if (m_mode == RelativeCoordinates) {
            point1.move(2 * m_currentPoint.x(), 2 * m_currentPoint.y());
            point2.move(3 * m_currentPoint.x(), 3 * m_currentPoint.y());
            targetPoint += m_currentPoint;
        }
        point1.scale(gOneOverThree, gOneOverThree);
        point2.scale(gOneOverThree, gOneOverThree);

        m_consumer->curveToCubic(point1, point2, targetPoint, AbsoluteCoordinates);

        if (m_mode == RelativeCoordinates)
            m_controlPoint += m_currentPoint;
        m_currentPoint = targetPoint;
    } else
        m_consumer->curveToQuadratic(point1, targetPoint, m_mode);
    return true;
}

bool SVGPathParser::parseCurveToQuadraticSmoothSegment()
{
    FloatPoint targetPoint;
    if (!m_source->parseCurveToQuadraticSmoothSegment(targetPoint))
        return false;

    if (m_lastCommand != PathSegCurveToQuadraticAbs
        && m_lastCommand != PathSegCurveToQuadraticRel
        && m_lastCommand != PathSegCurveToQuadraticSmoothAbs
        && m_lastCommand != PathSegCurveToQuadraticSmoothRel)
        m_controlPoint = m_currentPoint;

    if (m_pathParsingMode == NormalizedParsing) {
        FloatPoint cubicPoint = m_currentPoint;
        cubicPoint.scale(2, 2);
        cubicPoint.move(-m_controlPoint.x(), -m_controlPoint.y());
        FloatPoint point1(m_currentPoint.x() + 2 * cubicPoint.x(), m_currentPoint.y() + 2 * cubicPoint.y());
        FloatPoint point2(targetPoint.x() + 2 * cubicPoint.x(), targetPoint.y() + 2 * cubicPoint.y());
        if (m_mode == RelativeCoordinates) {
            point2 += m_currentPoint;
            targetPoint += m_currentPoint;
        }
        point1.scale(gOneOverThree, gOneOverThree);
        point2.scale(gOneOverThree, gOneOverThree);

        m_consumer->curveToCubic(point1, point2, targetPoint, AbsoluteCoordinates);

        m_controlPoint = cubicPoint;
        m_currentPoint = targetPoint;
    } else
        m_consumer->curveToQuadraticSmooth(targetPoint, m_mode);
    return true;
}

bool SVGPathParser::parseArcToSegment()
{
    float rx;
    float ry;
    float angle;
    bool largeArc;
    bool sweep;
    FloatPoint targetPoint;
    if (!m_source->parseArcToSegment(rx, ry, angle, largeArc, sweep, targetPoint))
        return false;

    // If rx = 0 or ry = 0 then this arc is treated as a straight line segment (a "lineto") joining the endpoints.
    // http://www.w3.org/TR/SVG/implnote.html#ArcOutOfRangeParameters
    // If the current point and target point for the arc are identical, it should be treated as a zero length
    // path. This ensures continuity in animations.
    rx = fabsf(rx);
    ry = fabsf(ry);
    bool arcIsZeroLength = false;
    if (m_pathParsingMode == NormalizedParsing) {
        if (m_mode == RelativeCoordinates)
            arcIsZeroLength = targetPoint == FloatPoint::zero();
        else
            arcIsZeroLength = targetPoint == m_currentPoint;
    }
    if (!rx || !ry || arcIsZeroLength) {
        if (m_pathParsingMode == NormalizedParsing) {
            if (m_mode == RelativeCoordinates)
                m_currentPoint += targetPoint;
            else
                m_currentPoint = targetPoint;
            m_consumer->lineTo(m_currentPoint, AbsoluteCoordinates);
        } else
            m_consumer->lineTo(targetPoint, m_mode);
        return true;
    }

    if (m_pathParsingMode == NormalizedParsing) {
        FloatPoint point1 = m_currentPoint;
        if (m_mode == RelativeCoordinates)
            targetPoint += m_currentPoint;
        m_currentPoint = targetPoint;
        return decomposeArcToCubic(angle, rx, ry, point1, targetPoint, largeArc, sweep);
    }
    m_consumer->arcTo(rx, ry, angle, largeArc, sweep, targetPoint, m_mode);
    return true;
}

bool SVGPathParser::parsePathDataFromSource(PathParsingMode pathParsingMode, bool checkForInitialMoveTo)
{
    ASSERT(m_source);
    ASSERT(m_consumer);

    m_pathParsingMode = pathParsingMode;

    m_controlPoint = FloatPoint();
    m_currentPoint = FloatPoint();
    m_subPathPoint = FloatPoint();
    m_closePath = true;

    // Skip any leading spaces.
    if (!m_source->moveToNextToken())
        return false;

    SVGPathSegType command;
    m_source->parseSVGSegmentType(command);
    m_lastCommand = PathSegUnknown;

    // Path must start with moveto.
    if (checkForInitialMoveTo && command != PathSegMoveToAbs && command != PathSegMoveToRel)
        return false;

    while (true) {
        // Skip spaces between command and first coordinate.
        m_source->moveToNextToken();
        m_mode = AbsoluteCoordinates;
        switch (command) {
        case PathSegMoveToRel:
            m_mode = RelativeCoordinates;
        case PathSegMoveToAbs:
            if (!parseMoveToSegment())
                return false;
            break;
        case PathSegLineToRel:
            m_mode = RelativeCoordinates;
        case PathSegLineToAbs:
            if (!parseLineToSegment())
                return false;
            break;
        case PathSegLineToHorizontalRel:
            m_mode = RelativeCoordinates;
        case PathSegLineToHorizontalAbs:
            if (!parseLineToHorizontalSegment())
                return false;
            break;
        case PathSegLineToVerticalRel:
            m_mode = RelativeCoordinates;
        case PathSegLineToVerticalAbs:
            if (!parseLineToVerticalSegment())
                return false;
            break;
        case PathSegClosePath:
            parseClosePathSegment();
            break;
        case PathSegCurveToCubicRel:
            m_mode = RelativeCoordinates;
        case PathSegCurveToCubicAbs:
            if (!parseCurveToCubicSegment())
                return false;
            break;
        case PathSegCurveToCubicSmoothRel:
            m_mode = RelativeCoordinates;
        case PathSegCurveToCubicSmoothAbs:
            if (!parseCurveToCubicSmoothSegment())
                return false;
            break;
        case PathSegCurveToQuadraticRel:
            m_mode = RelativeCoordinates;
        case PathSegCurveToQuadraticAbs:
            if (!parseCurveToQuadraticSegment())
                return false;
            break;
        case PathSegCurveToQuadraticSmoothRel:
            m_mode = RelativeCoordinates;
        case PathSegCurveToQuadraticSmoothAbs:
            if (!parseCurveToQuadraticSmoothSegment())
                return false;
            break;
        case PathSegArcRel:
            m_mode = RelativeCoordinates;
        case PathSegArcAbs:
            if (!parseArcToSegment())
                return false;
            break;
        default:
            return false;
        }
        if (!m_consumer->continueConsuming())
            return true;

        m_lastCommand = command;

        if (!m_source->hasMoreData())
            return true;

        command = m_source->nextCommand(command);

        if (m_lastCommand != PathSegCurveToCubicAbs
            && m_lastCommand != PathSegCurveToCubicRel
            && m_lastCommand != PathSegCurveToCubicSmoothAbs
            && m_lastCommand != PathSegCurveToCubicSmoothRel
            && m_lastCommand != PathSegCurveToQuadraticAbs
            && m_lastCommand != PathSegCurveToQuadraticRel
            && m_lastCommand != PathSegCurveToQuadraticSmoothAbs
            && m_lastCommand != PathSegCurveToQuadraticSmoothRel)
            m_controlPoint = m_currentPoint;

        m_consumer->incrementPathSegmentCount();
    }

    return false;
}

void SVGPathParser::cleanup()
{
    ASSERT(m_source);
    ASSERT(m_consumer);

    m_consumer->cleanup();
    m_source = 0;
    m_consumer = 0;
}

// This works by converting the SVG arc to "simple" beziers.
// Partly adapted from Niko's code in kdelibs/kdecore/svgicons.
// See also SVG implementation notes: http://www.w3.org/TR/SVG/implnote.html#ArcConversionEndpointToCenter
bool SVGPathParser::decomposeArcToCubic(float angle, float rx, float ry, FloatPoint& point1, FloatPoint& point2, bool largeArcFlag, bool sweepFlag)
{
    FloatSize midPointDistance = point1 - point2;
    midPointDistance.scale(0.5f);

    AffineTransform pointTransform;
    pointTransform.rotate(-angle);

    FloatPoint transformedMidPoint = pointTransform.mapPoint(FloatPoint(midPointDistance.width(), midPointDistance.height()));
    float squareRx = rx * rx;
    float squareRy = ry * ry;
    float squareX = transformedMidPoint.x() * transformedMidPoint.x();
    float squareY = transformedMidPoint.y() * transformedMidPoint.y();

    // Check if the radii are big enough to draw the arc, scale radii if not.
    // http://www.w3.org/TR/SVG/implnote.html#ArcCorrectionOutOfRangeRadii
    float radiiScale = squareX / squareRx + squareY / squareRy;
    if (radiiScale > 1) {
        rx *= sqrtf(radiiScale);
        ry *= sqrtf(radiiScale);
    }

    pointTransform.makeIdentity();
    pointTransform.scale(1 / rx, 1 / ry);
    pointTransform.rotate(-angle);

    point1 = pointTransform.mapPoint(point1);
    point2 = pointTransform.mapPoint(point2);
    FloatSize delta = point2 - point1;

    float d = delta.width() * delta.width() + delta.height() * delta.height();
    float scaleFactorSquared = std::max(1 / d - 0.25f, 0.f);

    float scaleFactor = sqrtf(scaleFactorSquared);
    if (sweepFlag == largeArcFlag)
        scaleFactor = -scaleFactor;

    delta.scale(scaleFactor);
    FloatPoint centerPoint = point1 + point2;
    centerPoint.scale(0.5f, 0.5f);
    centerPoint.move(-delta.height(), delta.width());

    float theta1 = FloatPoint(point1 - centerPoint).slopeAngleRadians();
    float theta2 = FloatPoint(point2 - centerPoint).slopeAngleRadians();

    float thetaArc = theta2 - theta1;
    if (thetaArc < 0 && sweepFlag)
        thetaArc += 2 * piFloat;
    else if (thetaArc > 0 && !sweepFlag)
        thetaArc -= 2 * piFloat;

    pointTransform.makeIdentity();
    pointTransform.rotate(angle);
    pointTransform.scale(rx, ry);

    // Some results of atan2 on some platform implementations are not exact enough. So that we get more
    // cubic curves than expected here. Adding 0.001f reduces the count of sgements to the correct count.
    int segments = ceilf(fabsf(thetaArc / (piOverTwoFloat + 0.001f)));
    for (int i = 0; i < segments; ++i) {
        float startTheta = theta1 + i * thetaArc / segments;
        float endTheta = theta1 + (i + 1) * thetaArc / segments;

        float t = (8 / 6.f) * tanf(0.25f * (endTheta - startTheta));
        if (!std::isfinite(t))
            return false;
        float sinStartTheta = sinf(startTheta);
        float cosStartTheta = cosf(startTheta);
        float sinEndTheta = sinf(endTheta);
        float cosEndTheta = cosf(endTheta);

        point1 = FloatPoint(cosStartTheta - t * sinStartTheta, sinStartTheta + t * cosStartTheta);
        point1.move(centerPoint.x(), centerPoint.y());
        FloatPoint targetPoint = FloatPoint(cosEndTheta, sinEndTheta);
        targetPoint.move(centerPoint.x(), centerPoint.y());
        point2 = targetPoint;
        point2.move(t * sinEndTheta, -t * cosEndTheta);

        m_consumer->curveToCubic(pointTransform.mapPoint(point1), pointTransform.mapPoint(point2),
                                 pointTransform.mapPoint(targetPoint), AbsoluteCoordinates);
    }
    return true;
}

}

#endif // ENABLE(SVG)
