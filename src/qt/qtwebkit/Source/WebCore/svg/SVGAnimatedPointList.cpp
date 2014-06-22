/*
 * Copyright (C) Research In Motion Limited 2011, 2012. All rights reserved.
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
#include "SVGAnimatedPointList.h"

#include "SVGAnimateElement.h"
#include "SVGParserUtilities.h"
#include "SVGPointList.h"

namespace WebCore {

SVGAnimatedPointListAnimator::SVGAnimatedPointListAnimator(SVGAnimationElement* animationElement, SVGElement* contextElement)
    : SVGAnimatedTypeAnimator(AnimatedPoints, animationElement, contextElement)
{
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedPointListAnimator::constructFromString(const String& string)
{
    OwnPtr<SVGAnimatedType> animtedType = SVGAnimatedType::createPointList(new SVGPointList);
    pointsListFromSVGData(animtedType->pointList(), string);
    return animtedType.release();
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedPointListAnimator::startAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    return SVGAnimatedType::createPointList(constructFromBaseValue<SVGAnimatedPointList>(animatedTypes));
}

void SVGAnimatedPointListAnimator::stopAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    stopAnimValAnimationForType<SVGAnimatedPointList>(animatedTypes);
}

void SVGAnimatedPointListAnimator::resetAnimValToBaseVal(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType* type)
{
    resetFromBaseValue<SVGAnimatedPointList>(animatedTypes, type, &SVGAnimatedType::pointList);
}

void SVGAnimatedPointListAnimator::animValWillChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValWillChangeForType<SVGAnimatedPointList>(animatedTypes);
}

void SVGAnimatedPointListAnimator::animValDidChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValDidChangeForType<SVGAnimatedPointList>(animatedTypes);
}

void SVGAnimatedPointListAnimator::addAnimatedTypes(SVGAnimatedType* from, SVGAnimatedType* to)
{
    ASSERT(from->type() == AnimatedPoints);
    ASSERT(from->type() == to->type());

    const SVGPointList& fromPointList = from->pointList();
    SVGPointList& toPointList = to->pointList();

    unsigned fromPointListSize = fromPointList.size();
    if (!fromPointListSize || fromPointListSize != toPointList.size())
        return;

    for (unsigned i = 0; i < fromPointListSize; ++i)
        toPointList[i] += fromPointList[i];
}

void SVGAnimatedPointListAnimator::calculateAnimatedValue(float percentage, unsigned repeatCount, SVGAnimatedType* from, SVGAnimatedType* to, SVGAnimatedType* toAtEndOfDuration, SVGAnimatedType* animated)
{
    ASSERT(m_animationElement);

    const SVGPointList& fromPointList = m_animationElement->animationMode() == ToAnimation ? animated->pointList() : from->pointList();
    const SVGPointList& toPointList = to->pointList();
    const SVGPointList& toAtEndOfDurationPointList = toAtEndOfDuration->pointList();
    SVGPointList& animatedPointList = animated->pointList();
    if (!m_animationElement->adjustFromToListValues<SVGPointList>(fromPointList, toPointList, animatedPointList, percentage))
        return;

    unsigned fromPointListSize = fromPointList.size();
    unsigned toPointListSize = toPointList.size();
    unsigned toAtEndOfDurationSize = toAtEndOfDurationPointList.size();

    for (unsigned i = 0; i < toPointListSize; ++i) {
        FloatPoint effectiveFrom;
        if (fromPointListSize)
            effectiveFrom = fromPointList[i];
        FloatPoint effectiveToAtEnd = i < toAtEndOfDurationSize ? toAtEndOfDurationPointList[i] : FloatPoint();

        float animatedX = animatedPointList[i].x();
        float animatedY = animatedPointList[i].y();
        m_animationElement->animateAdditiveNumber(percentage, repeatCount, effectiveFrom.x(), toPointList[i].x(), effectiveToAtEnd.x(), animatedX);
        m_animationElement->animateAdditiveNumber(percentage, repeatCount, effectiveFrom.y(), toPointList[i].y(), effectiveToAtEnd.y(), animatedY);
        animatedPointList[i] = FloatPoint(animatedX, animatedY);
    }
}

float SVGAnimatedPointListAnimator::calculateDistance(const String&, const String&)
{
    // FIXME: Distance calculation is not possible for SVGPointList right now. We need the distance of for every single value.
    return -1;
}

}

#endif // ENABLE(SVG)
