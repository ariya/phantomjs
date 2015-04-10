/*
 * Copyright (C) Research In Motion Limited 2010, 2011. All rights reserved.
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
#include "SVGPathBlender.h"

#include "AnimationUtilities.h"
#include "SVGPathSeg.h"
#include "SVGPathSource.h"
#include <wtf/TemporaryChange.h>

namespace WebCore {

SVGPathBlender::SVGPathBlender()
    : m_fromSource(0)
    , m_toSource(0)
    , m_consumer(0)
    , m_progress(0)
    , m_addTypesCount(0)
    , m_isInFirstHalfOfAnimation(false)
{
}

// Helper functions
static inline FloatPoint blendFloatPoint(const FloatPoint& a, const FloatPoint& b, float progress)
{
    return FloatPoint(blend(a.x(), b.x(), progress), blend(a.y(), b.y(), progress));
}

float SVGPathBlender::blendAnimatedDimensonalFloat(float from, float to, FloatBlendMode blendMode)
{
    if (m_addTypesCount) {
        ASSERT(m_fromMode == m_toMode);
        return from + to * m_addTypesCount;
    }

    if (m_fromMode == m_toMode)
        return blend(from, to, m_progress);
    
    float fromValue = blendMode == BlendHorizontal ? m_fromCurrentPoint.x() : m_fromCurrentPoint.y();
    float toValue = blendMode == BlendHorizontal ? m_toCurrentPoint.x() : m_toCurrentPoint.y();

    // Transform toY to the coordinate mode of fromY
    float animValue = blend(from, m_fromMode == AbsoluteCoordinates ? to + toValue : to - toValue, m_progress);
    
    if (m_isInFirstHalfOfAnimation)
        return animValue;
    
    // Transform the animated point to the coordinate mode, needed for the current progress.
    float currentValue = blend(fromValue, toValue, m_progress);
    return m_toMode == AbsoluteCoordinates ? animValue + currentValue : animValue - currentValue;
}

FloatPoint SVGPathBlender::blendAnimatedFloatPoint(const FloatPoint& fromPoint, const FloatPoint& toPoint)
{
    if (m_addTypesCount) {
        ASSERT(m_fromMode == m_toMode);
        FloatPoint repeatedToPoint = toPoint;
        repeatedToPoint.scale(m_addTypesCount, m_addTypesCount);
        return fromPoint + repeatedToPoint;
    }

    if (m_fromMode == m_toMode)
        return blendFloatPoint(fromPoint, toPoint, m_progress);

    // Transform toPoint to the coordinate mode of fromPoint
    FloatPoint animatedPoint = toPoint;
    if (m_fromMode == AbsoluteCoordinates)
        animatedPoint += m_toCurrentPoint;
    else
        animatedPoint.move(-m_toCurrentPoint.x(), -m_toCurrentPoint.y());

    animatedPoint = blendFloatPoint(fromPoint, animatedPoint, m_progress);

    if (m_isInFirstHalfOfAnimation)
        return animatedPoint;

    // Transform the animated point to the coordinate mode, needed for the current progress.
    FloatPoint currentPoint = blendFloatPoint(m_fromCurrentPoint, m_toCurrentPoint, m_progress);
    if (m_toMode == AbsoluteCoordinates)
        return animatedPoint + currentPoint;

    animatedPoint.move(-currentPoint.x(), -currentPoint.y());
    return animatedPoint;
}

bool SVGPathBlender::blendMoveToSegment()
{
    FloatPoint fromTargetPoint;
    FloatPoint toTargetPoint;
    if ((m_fromSource->hasMoreData() && !m_fromSource->parseMoveToSegment(fromTargetPoint))
        || !m_toSource->parseMoveToSegment(toTargetPoint))
        return false;

    m_consumer->moveTo(blendAnimatedFloatPoint(fromTargetPoint, toTargetPoint), false, m_isInFirstHalfOfAnimation ? m_fromMode : m_toMode);
    m_fromCurrentPoint = m_fromMode == AbsoluteCoordinates ? fromTargetPoint : m_fromCurrentPoint + fromTargetPoint;
    m_toCurrentPoint = m_toMode == AbsoluteCoordinates ? toTargetPoint : m_toCurrentPoint + toTargetPoint;
    return true;
}

bool SVGPathBlender::blendLineToSegment()
{
    FloatPoint fromTargetPoint;
    FloatPoint toTargetPoint;
    if ((m_fromSource->hasMoreData() && !m_fromSource->parseLineToSegment(fromTargetPoint))
        || !m_toSource->parseLineToSegment(toTargetPoint))
        return false;

    m_consumer->lineTo(blendAnimatedFloatPoint(fromTargetPoint, toTargetPoint), m_isInFirstHalfOfAnimation ? m_fromMode : m_toMode);
    m_fromCurrentPoint = m_fromMode == AbsoluteCoordinates ? fromTargetPoint : m_fromCurrentPoint + fromTargetPoint;
    m_toCurrentPoint = m_toMode == AbsoluteCoordinates ? toTargetPoint : m_toCurrentPoint + toTargetPoint;
    return true;
}

bool SVGPathBlender::blendLineToHorizontalSegment()
{
    float fromX = 0;
    float toX = 0;
    if ((m_fromSource->hasMoreData() && !m_fromSource->parseLineToHorizontalSegment(fromX))
        || !m_toSource->parseLineToHorizontalSegment(toX))
        return false;

    m_consumer->lineToHorizontal(blendAnimatedDimensonalFloat(fromX, toX, BlendHorizontal), m_isInFirstHalfOfAnimation ? m_fromMode : m_toMode);
    m_fromCurrentPoint.setX(m_fromMode == AbsoluteCoordinates ? fromX : m_fromCurrentPoint.x() + fromX);
    m_toCurrentPoint.setX(m_toMode == AbsoluteCoordinates ? toX : m_toCurrentPoint.x() + toX);
    return true;
}

bool SVGPathBlender::blendLineToVerticalSegment()
{
    float fromY = 0;
    float toY = 0;
    if ((m_fromSource->hasMoreData() && !m_fromSource->parseLineToVerticalSegment(fromY))
        || !m_toSource->parseLineToVerticalSegment(toY))
        return false;

    m_consumer->lineToVertical(blendAnimatedDimensonalFloat(fromY, toY, BlendVertical), m_isInFirstHalfOfAnimation ? m_fromMode : m_toMode);
    m_fromCurrentPoint.setY(m_fromMode == AbsoluteCoordinates ? fromY : m_fromCurrentPoint.y() + fromY);
    m_toCurrentPoint.setY(m_toMode == AbsoluteCoordinates ? toY : m_toCurrentPoint.y() + toY);
    return true;
}

bool SVGPathBlender::blendCurveToCubicSegment()
{
    FloatPoint fromTargetPoint;
    FloatPoint fromPoint1;
    FloatPoint fromPoint2;
    FloatPoint toTargetPoint;
    FloatPoint toPoint1;
    FloatPoint toPoint2;
    if ((m_fromSource->hasMoreData() && !m_fromSource->parseCurveToCubicSegment(fromPoint1, fromPoint2, fromTargetPoint))
        || !m_toSource->parseCurveToCubicSegment(toPoint1, toPoint2, toTargetPoint))
        return false;

    m_consumer->curveToCubic(blendAnimatedFloatPoint(fromPoint1, toPoint1),
                             blendAnimatedFloatPoint(fromPoint2, toPoint2),
                             blendAnimatedFloatPoint(fromTargetPoint, toTargetPoint),
                             m_isInFirstHalfOfAnimation ? m_fromMode : m_toMode);
    m_fromCurrentPoint = m_fromMode == AbsoluteCoordinates ? fromTargetPoint : m_fromCurrentPoint + fromTargetPoint;
    m_toCurrentPoint = m_toMode == AbsoluteCoordinates ? toTargetPoint : m_toCurrentPoint + toTargetPoint;
    return true;
}

bool SVGPathBlender::blendCurveToCubicSmoothSegment()
{
    FloatPoint fromTargetPoint;
    FloatPoint fromPoint2;
    FloatPoint toTargetPoint;
    FloatPoint toPoint2;
    if ((m_fromSource->hasMoreData() && !m_fromSource->parseCurveToCubicSmoothSegment(fromPoint2, fromTargetPoint))
        || !m_toSource->parseCurveToCubicSmoothSegment(toPoint2, toTargetPoint))
        return false;

    m_consumer->curveToCubicSmooth(blendAnimatedFloatPoint(fromPoint2, toPoint2),
                                   blendAnimatedFloatPoint(fromTargetPoint, toTargetPoint),
                                   m_isInFirstHalfOfAnimation ? m_fromMode : m_toMode);
    m_fromCurrentPoint = m_fromMode == AbsoluteCoordinates ? fromTargetPoint : m_fromCurrentPoint + fromTargetPoint;
    m_toCurrentPoint = m_toMode == AbsoluteCoordinates ? toTargetPoint : m_toCurrentPoint + toTargetPoint;
    return true;
}

bool SVGPathBlender::blendCurveToQuadraticSegment()
{
    FloatPoint fromTargetPoint;
    FloatPoint fromPoint1;
    FloatPoint toTargetPoint;
    FloatPoint toPoint1;
    if ((m_fromSource->hasMoreData() && !m_fromSource->parseCurveToQuadraticSegment(fromPoint1, fromTargetPoint))
        || !m_toSource->parseCurveToQuadraticSegment(toPoint1, toTargetPoint))
        return false;

    m_consumer->curveToQuadratic(blendAnimatedFloatPoint(fromPoint1, toPoint1),
                                 blendAnimatedFloatPoint(fromTargetPoint, toTargetPoint),
                                 m_isInFirstHalfOfAnimation ? m_fromMode : m_toMode);
    m_fromCurrentPoint = m_fromMode == AbsoluteCoordinates ? fromTargetPoint : m_fromCurrentPoint + fromTargetPoint;
    m_toCurrentPoint = m_toMode == AbsoluteCoordinates ? toTargetPoint : m_toCurrentPoint + toTargetPoint;
    return true;
}

bool SVGPathBlender::blendCurveToQuadraticSmoothSegment()
{
    FloatPoint fromTargetPoint;
    FloatPoint toTargetPoint;
    if ((m_fromSource->hasMoreData() && !m_fromSource->parseCurveToQuadraticSmoothSegment(fromTargetPoint))
        || !m_toSource->parseCurveToQuadraticSmoothSegment(toTargetPoint))
        return false;

    m_consumer->curveToQuadraticSmooth(blendAnimatedFloatPoint(fromTargetPoint, toTargetPoint), m_isInFirstHalfOfAnimation ? m_fromMode : m_toMode);
    m_fromCurrentPoint = m_fromMode == AbsoluteCoordinates ? fromTargetPoint : m_fromCurrentPoint + fromTargetPoint;
    m_toCurrentPoint = m_toMode == AbsoluteCoordinates ? toTargetPoint : m_toCurrentPoint + toTargetPoint;
    return true;
}

bool SVGPathBlender::blendArcToSegment()
{
    float fromRx = 0;
    float fromRy = 0;
    float fromAngle = 0;
    bool fromLargeArc = false;
    bool fromSweep = false;
    FloatPoint fromTargetPoint;
    float toRx = 0;
    float toRy = 0;
    float toAngle = 0;
    bool toLargeArc = false;
    bool toSweep = false;
    FloatPoint toTargetPoint;
    if ((m_fromSource->hasMoreData() && !m_fromSource->parseArcToSegment(fromRx, fromRy, fromAngle, fromLargeArc, fromSweep, fromTargetPoint))
        || !m_toSource->parseArcToSegment(toRx, toRy, toAngle, toLargeArc, toSweep, toTargetPoint))
        return false;

    if (m_addTypesCount) {
        ASSERT(m_fromMode == m_toMode);
        FloatPoint scaledToTargetPoint = toTargetPoint;
        scaledToTargetPoint.scale(m_addTypesCount, m_addTypesCount);
        m_consumer->arcTo(fromRx + toRx * m_addTypesCount,
                          fromRy + toRy * m_addTypesCount,
                          fromAngle + toAngle * m_addTypesCount,
                          fromLargeArc || toLargeArc,
                          fromSweep || toSweep,
                          fromTargetPoint + scaledToTargetPoint,
                          m_fromMode);
    } else {
        m_consumer->arcTo(blend(fromRx, toRx, m_progress),
                          blend(fromRy, toRy, m_progress),
                          blend(fromAngle, toAngle, m_progress),
                          m_isInFirstHalfOfAnimation ? fromLargeArc : toLargeArc,
                          m_isInFirstHalfOfAnimation ? fromSweep : toSweep,
                          blendAnimatedFloatPoint(fromTargetPoint, toTargetPoint),
                          m_isInFirstHalfOfAnimation ? m_fromMode : m_toMode);
    }
    m_fromCurrentPoint = m_fromMode == AbsoluteCoordinates ? fromTargetPoint : m_fromCurrentPoint + fromTargetPoint;
    m_toCurrentPoint = m_toMode == AbsoluteCoordinates ? toTargetPoint : m_toCurrentPoint + toTargetPoint;
    return true;
}

static inline PathCoordinateMode coordinateModeOfCommand(const SVGPathSegType& type)
{
    if (type < PathSegMoveToAbs)
        return AbsoluteCoordinates;

    // Odd number = relative command
    if (type % 2)
        return RelativeCoordinates;

    return AbsoluteCoordinates;
}

static inline bool isSegmentEqual(const SVGPathSegType& fromType, const SVGPathSegType& toType, const PathCoordinateMode& fromMode, const PathCoordinateMode& toMode)
{
    if (fromType == toType && (fromType == PathSegUnknown || fromType == PathSegClosePath))
        return true;

    unsigned short from = fromType;
    unsigned short to = toType;
    if (fromMode == toMode)
        return from == to;
    if (fromMode == AbsoluteCoordinates)
        return from == to - 1;
    return to == from - 1;
}

bool SVGPathBlender::addAnimatedPath(SVGPathSource* fromSource, SVGPathSource* toSource, SVGPathConsumer* consumer, unsigned repeatCount)
{
    TemporaryChange<unsigned> change(m_addTypesCount, repeatCount);
    return blendAnimatedPath(0, fromSource, toSource, consumer);
}

bool SVGPathBlender::blendAnimatedPath(float progress, SVGPathSource* fromSource, SVGPathSource* toSource, SVGPathConsumer* consumer)
{
    ASSERT(fromSource);
    ASSERT(toSource);
    ASSERT(consumer);
    m_fromSource = fromSource;
    m_toSource = toSource;
    m_consumer = consumer;
    m_isInFirstHalfOfAnimation = progress < 0.5f;
    m_progress = progress;

    bool fromSourceHadData = m_fromSource->hasMoreData();
    while (m_toSource->hasMoreData()) {
        SVGPathSegType fromCommand;
        SVGPathSegType toCommand;
        if ((fromSourceHadData && !m_fromSource->parseSVGSegmentType(fromCommand)) || !m_toSource->parseSVGSegmentType(toCommand))
            return false;

        m_toMode = coordinateModeOfCommand(toCommand);
        m_fromMode = fromSourceHadData ? coordinateModeOfCommand(fromCommand) : m_toMode;
        if (m_fromMode != m_toMode && m_addTypesCount)
            return false;

        if (fromSourceHadData && !isSegmentEqual(fromCommand, toCommand, m_fromMode, m_toMode))
            return false;

        switch (toCommand) {
        case PathSegMoveToRel:
        case PathSegMoveToAbs:
            if (!blendMoveToSegment())
                return false;
            break;
        case PathSegLineToRel:
        case PathSegLineToAbs:
            if (!blendLineToSegment())
                return false;
            break;
        case PathSegLineToHorizontalRel:
        case PathSegLineToHorizontalAbs:
            if (!blendLineToHorizontalSegment())
                return false;
            break;
        case PathSegLineToVerticalRel:
        case PathSegLineToVerticalAbs:
            if (!blendLineToVerticalSegment())
                return false;
            break;
        case PathSegClosePath:
            m_consumer->closePath();
            break;
        case PathSegCurveToCubicRel:
        case PathSegCurveToCubicAbs:
            if (!blendCurveToCubicSegment())
                return false;
            break;
        case PathSegCurveToCubicSmoothRel:
        case PathSegCurveToCubicSmoothAbs:
            if (!blendCurveToCubicSmoothSegment())
                return false;
            break;
        case PathSegCurveToQuadraticRel:
        case PathSegCurveToQuadraticAbs:
            if (!blendCurveToQuadraticSegment())
                return false;
            break;
        case PathSegCurveToQuadraticSmoothRel:
        case PathSegCurveToQuadraticSmoothAbs:
            if (!blendCurveToQuadraticSmoothSegment())
                return false;
            break;
        case PathSegArcRel:
        case PathSegArcAbs:
            if (!blendArcToSegment())
                return false;
            break;
        case PathSegUnknown:
            return false;
        }

        if (!fromSourceHadData)
            continue;
        if (m_fromSource->hasMoreData() != m_toSource->hasMoreData())
            return false;
        if (!m_fromSource->hasMoreData() || !m_toSource->hasMoreData())
            return true;
    }

    return true;
}

void SVGPathBlender::cleanup()
{
    ASSERT(m_toSource);
    ASSERT(m_fromSource);
    ASSERT(m_consumer);

    m_consumer->cleanup();
    m_toSource = 0;
    m_fromSource = 0;
    m_consumer = 0;
    m_fromCurrentPoint = FloatPoint();
    m_toCurrentPoint = FloatPoint();
}

}

#endif // ENABLE(SVG)
