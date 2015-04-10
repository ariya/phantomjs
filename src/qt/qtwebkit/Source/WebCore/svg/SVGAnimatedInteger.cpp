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
#include "SVGAnimatedInteger.h"

#include "SVGAnimateElement.h"
#include "SVGAnimatedNumber.h"
#include <wtf/MathExtras.h>

namespace WebCore {

SVGAnimatedIntegerAnimator::SVGAnimatedIntegerAnimator(SVGAnimationElement* animationElement, SVGElement* contextElement)
    : SVGAnimatedTypeAnimator(AnimatedInteger, animationElement, contextElement)
{
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedIntegerAnimator::constructFromString(const String& string)
{
    OwnPtr<SVGAnimatedType> animtedType = SVGAnimatedType::createInteger(new int);
    animtedType->integer() = string.toIntStrict();
    return animtedType.release();
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedIntegerAnimator::startAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    return SVGAnimatedType::createInteger(constructFromBaseValue<SVGAnimatedInteger>(animatedTypes));
}

void SVGAnimatedIntegerAnimator::stopAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    stopAnimValAnimationForType<SVGAnimatedInteger>(animatedTypes);
}

void SVGAnimatedIntegerAnimator::resetAnimValToBaseVal(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType* type)
{
    resetFromBaseValue<SVGAnimatedInteger>(animatedTypes, type, &SVGAnimatedType::integer);
}

void SVGAnimatedIntegerAnimator::animValWillChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValWillChangeForType<SVGAnimatedInteger>(animatedTypes);
}

void SVGAnimatedIntegerAnimator::animValDidChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValDidChangeForType<SVGAnimatedInteger>(animatedTypes);
}

void SVGAnimatedIntegerAnimator::addAnimatedTypes(SVGAnimatedType* from, SVGAnimatedType* to)
{
    ASSERT(from->type() == AnimatedInteger);
    ASSERT(from->type() == to->type());

    to->integer() += from->integer();
}

void SVGAnimatedIntegerAnimator::calculateAnimatedInteger(SVGAnimationElement* animationElement, float percentage, unsigned repeatCount, int fromInteger, int toInteger, int toAtEndOfDurationInteger, int& animatedInteger)
{
    float animatedNumber = animatedInteger;
    animationElement->animateAdditiveNumber(percentage, repeatCount, fromInteger, toInteger, toAtEndOfDurationInteger, animatedNumber);
    animatedInteger = static_cast<int>(roundf(animatedNumber));
}

void SVGAnimatedIntegerAnimator::calculateAnimatedValue(float percentage, unsigned repeatCount, SVGAnimatedType* from, SVGAnimatedType* to, SVGAnimatedType* toAtEndOfDuration, SVGAnimatedType* animated)
{
    ASSERT(m_animationElement);
    ASSERT(m_contextElement);

    int fromInteger = m_animationElement->animationMode() == ToAnimation ? animated->integer() : from->integer();
    int toInteger = to->integer();
    int toAtEndOfDurationInteger = toAtEndOfDuration->integer();
    int& animatedInteger = animated->integer();

    calculateAnimatedInteger(m_animationElement, percentage, repeatCount, fromInteger, toInteger, toAtEndOfDurationInteger, animatedInteger);
}

float SVGAnimatedIntegerAnimator::calculateDistance(const String& fromString, const String& toString)
{
    ASSERT(m_contextElement);
    int from = fromString.toIntStrict();
    int to = toString.toIntStrict();
    return abs(to - from);
}
    
}

#endif // ENABLE(SVG)
