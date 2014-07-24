/*
 * Copyright (C) Research In Motion Limited 2012. All rights reserved.
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
#include "SVGAnimatedIntegerOptionalInteger.h"

#include "SVGAnimateElement.h"
#include "SVGAnimatedInteger.h"
#include "SVGParserUtilities.h"

namespace WebCore {

SVGAnimatedIntegerOptionalIntegerAnimator::SVGAnimatedIntegerOptionalIntegerAnimator(SVGAnimationElement* animationElement, SVGElement* contextElement)
    : SVGAnimatedTypeAnimator(AnimatedIntegerOptionalInteger, animationElement, contextElement)
{
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedIntegerOptionalIntegerAnimator::constructFromString(const String& string)
{
    OwnPtr<SVGAnimatedType> animtedType = SVGAnimatedType::createIntegerOptionalInteger(new pair<int, int>);
    pair<int, int>& animatedInteger = animtedType->integerOptionalInteger();
    float firstNumber = 0;
    float secondNumber = 0;
    if (!parseNumberOptionalNumber(string, firstNumber, secondNumber)) {
        animatedInteger.first = 0;
        animatedInteger.second = 0;
    } else {
        animatedInteger.first = static_cast<int>(roundf(firstNumber));
        animatedInteger.second = static_cast<int>(roundf(secondNumber));
    }
    return animtedType.release();
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedIntegerOptionalIntegerAnimator::startAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    return SVGAnimatedType::createIntegerOptionalInteger(constructFromBaseValues<SVGAnimatedInteger, SVGAnimatedInteger>(animatedTypes));
}

void SVGAnimatedIntegerOptionalIntegerAnimator::stopAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    stopAnimValAnimationForTypes<SVGAnimatedInteger, SVGAnimatedInteger>(animatedTypes);
}

void SVGAnimatedIntegerOptionalIntegerAnimator::resetAnimValToBaseVal(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType* type)
{
    resetFromBaseValues<SVGAnimatedInteger, SVGAnimatedInteger>(animatedTypes, type, &SVGAnimatedType::integerOptionalInteger);
}

void SVGAnimatedIntegerOptionalIntegerAnimator::animValWillChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValWillChangeForTypes<SVGAnimatedInteger, SVGAnimatedInteger>(animatedTypes);
}

void SVGAnimatedIntegerOptionalIntegerAnimator::animValDidChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValDidChangeForTypes<SVGAnimatedInteger, SVGAnimatedInteger>(animatedTypes);
}

void SVGAnimatedIntegerOptionalIntegerAnimator::addAnimatedTypes(SVGAnimatedType* from, SVGAnimatedType* to)
{
    ASSERT(from->type() == AnimatedIntegerOptionalInteger);
    ASSERT(from->type() == to->type());

    const pair<int, int>& fromIntegerPair = from->integerOptionalInteger();
    pair<int, int>& toIntegerPair = to->integerOptionalInteger();

    toIntegerPair.first += fromIntegerPair.first;
    toIntegerPair.second += fromIntegerPair.second;
}

void SVGAnimatedIntegerOptionalIntegerAnimator::calculateAnimatedValue(float percentage, unsigned repeatCount, SVGAnimatedType* from, SVGAnimatedType* to, SVGAnimatedType* toAtEndOfDuration, SVGAnimatedType* animated)
{
    ASSERT(m_animationElement);
    ASSERT(m_contextElement);

    const pair<int, int>& fromIntegerPair = m_animationElement->animationMode() == ToAnimation ? animated->integerOptionalInteger() : from->integerOptionalInteger();
    const pair<int, int>& toIntegerPair = to->integerOptionalInteger();
    const pair<int, int>& toAtEndOfDurationIntegerPair = toAtEndOfDuration->integerOptionalInteger();
    pair<int, int>& animatedIntegerPair = animated->integerOptionalInteger();

    SVGAnimatedIntegerAnimator::calculateAnimatedInteger(m_animationElement, percentage, repeatCount, fromIntegerPair.first, toIntegerPair.first, toAtEndOfDurationIntegerPair.first, animatedIntegerPair.first);
    SVGAnimatedIntegerAnimator::calculateAnimatedInteger(m_animationElement, percentage, repeatCount, fromIntegerPair.second, toIntegerPair.second, toAtEndOfDurationIntegerPair.second, animatedIntegerPair.second);
}

float SVGAnimatedIntegerOptionalIntegerAnimator::calculateDistance(const String&, const String&)
{
    // FIXME: Distance calculation is not possible for SVGIntegerOptionalInteger right now. We need the distance for every single value.
    return -1;
}

}

#endif // ENABLE(SVG)
