/*
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
#include "SVGAnimatedNumberList.h"

#include "SVGAnimateElement.h"
#include "SVGAnimatedNumber.h"

namespace WebCore {

SVGAnimatedNumberListAnimator::SVGAnimatedNumberListAnimator(SVGAnimationElement* animationElement, SVGElement* contextElement)
    : SVGAnimatedTypeAnimator(AnimatedNumberList, animationElement, contextElement)
{
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedNumberListAnimator::constructFromString(const String& string)
{
    OwnPtr<SVGAnimatedType> animtedType = SVGAnimatedType::createNumberList(new SVGNumberList);
    animtedType->numberList().parse(string);
    return animtedType.release();
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedNumberListAnimator::startAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    return SVGAnimatedType::createNumberList(constructFromBaseValue<SVGAnimatedNumberList>(animatedTypes));
}

void SVGAnimatedNumberListAnimator::stopAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    stopAnimValAnimationForType<SVGAnimatedNumberList>(animatedTypes);
}

void SVGAnimatedNumberListAnimator::resetAnimValToBaseVal(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType* type)
{
    resetFromBaseValue<SVGAnimatedNumberList>(animatedTypes, type, &SVGAnimatedType::numberList);
}

void SVGAnimatedNumberListAnimator::animValWillChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValWillChangeForType<SVGAnimatedNumberList>(animatedTypes);
}

void SVGAnimatedNumberListAnimator::animValDidChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValDidChangeForType<SVGAnimatedNumberList>(animatedTypes);
}

void SVGAnimatedNumberListAnimator::addAnimatedTypes(SVGAnimatedType* from, SVGAnimatedType* to)
{
    ASSERT(from->type() == AnimatedNumberList);
    ASSERT(from->type() == to->type());

    const SVGNumberList& fromNumberList = from->numberList();
    SVGNumberList& toNumberList = to->numberList();

    unsigned fromNumberListSize = fromNumberList.size();
    if (!fromNumberListSize || fromNumberListSize != toNumberList.size())
        return;

    for (unsigned i = 0; i < fromNumberListSize; ++i)
        toNumberList[i] += fromNumberList[i];
}

void SVGAnimatedNumberListAnimator::calculateAnimatedValue(float percentage, unsigned repeatCount, SVGAnimatedType* from, SVGAnimatedType* to, SVGAnimatedType* toAtEndOfDuration, SVGAnimatedType* animated)
{
    ASSERT(m_animationElement);

    const SVGNumberList& fromNumberList = m_animationElement->animationMode() == ToAnimation ? animated->numberList() : from->numberList();
    const SVGNumberList& toNumberList = to->numberList();
    const SVGNumberList& toAtEndOfDurationNumberList = toAtEndOfDuration->numberList();
    SVGNumberList& animatedNumberList = animated->numberList();
    if (!m_animationElement->adjustFromToListValues<SVGNumberList>(fromNumberList, toNumberList, animatedNumberList, percentage))
        return;

    unsigned fromNumberListSize = fromNumberList.size();
    unsigned toNumberListSize = toNumberList.size();
    unsigned toAtEndOfDurationSize = toAtEndOfDurationNumberList.size();

    for (unsigned i = 0; i < toNumberListSize; ++i) {
        float effectiveFrom = fromNumberListSize ? fromNumberList[i] : 0;
        float effectiveToAtEnd = i < toAtEndOfDurationSize ? toAtEndOfDurationNumberList[i] : 0;
        m_animationElement->animateAdditiveNumber(percentage, repeatCount, effectiveFrom, toNumberList[i], effectiveToAtEnd, animatedNumberList[i]);
    }
}

float SVGAnimatedNumberListAnimator::calculateDistance(const String&, const String&)
{
    // FIXME: Distance calculation is not possible for SVGNumberList right now. We need the distance for every single value.
    return -1;
}

}

#endif // ENABLE(SVG)
