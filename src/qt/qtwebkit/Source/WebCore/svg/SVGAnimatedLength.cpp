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
#include "SVGAnimatedLength.h"

#include "SVGAnimateElement.h"
#include "SVGAnimatedNumber.h"

namespace WebCore {

SVGAnimatedLengthAnimator::SVGAnimatedLengthAnimator(SVGAnimationElement* animationElement, SVGElement* contextElement)
    : SVGAnimatedTypeAnimator(AnimatedLength, animationElement, contextElement)
    , m_lengthMode(SVGLength::lengthModeForAnimatedLengthAttribute(animationElement->attributeName()))
{
}

static inline SVGLength& sharedSVGLength(SVGLengthMode mode, const String& valueAsString)
{
    DEFINE_STATIC_LOCAL(SVGLength, sharedLength, ());
    sharedLength.setValueAsString(valueAsString, mode, ASSERT_NO_EXCEPTION);
    return sharedLength;
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedLengthAnimator::constructFromString(const String& string)
{
    return SVGAnimatedType::createLength(new SVGLength(m_lengthMode, string));
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedLengthAnimator::startAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    return SVGAnimatedType::createLength(constructFromBaseValue<SVGAnimatedLength>(animatedTypes));
}

void SVGAnimatedLengthAnimator::stopAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    stopAnimValAnimationForType<SVGAnimatedLength>(animatedTypes);
}

void SVGAnimatedLengthAnimator::resetAnimValToBaseVal(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType* type)
{
    resetFromBaseValue<SVGAnimatedLength>(animatedTypes, type, &SVGAnimatedType::length);
}

void SVGAnimatedLengthAnimator::animValWillChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValWillChangeForType<SVGAnimatedLength>(animatedTypes);
}

void SVGAnimatedLengthAnimator::animValDidChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValDidChangeForType<SVGAnimatedLength>(animatedTypes);
}

void SVGAnimatedLengthAnimator::addAnimatedTypes(SVGAnimatedType* from, SVGAnimatedType* to)
{
    ASSERT(from->type() == AnimatedLength);
    ASSERT(from->type() == to->type());

    SVGLengthContext lengthContext(m_contextElement);
    const SVGLength& fromLength = from->length();
    SVGLength& toLength = to->length();

    toLength.setValue(toLength.value(lengthContext) + fromLength.value(lengthContext), lengthContext, ASSERT_NO_EXCEPTION);
}

static SVGLength parseLengthFromString(SVGAnimationElement* animationElement, const String& string)
{
    return sharedSVGLength(SVGLength::lengthModeForAnimatedLengthAttribute(animationElement->attributeName()), string);
}

void SVGAnimatedLengthAnimator::calculateAnimatedValue(float percentage, unsigned repeatCount, SVGAnimatedType* from, SVGAnimatedType* to, SVGAnimatedType* toAtEndOfDuration, SVGAnimatedType* animated)
{
    ASSERT(m_animationElement);
    ASSERT(m_contextElement);

    SVGLength fromSVGLength = m_animationElement->animationMode() == ToAnimation ? animated->length() : from->length();
    SVGLength toSVGLength = to->length();
    const SVGLength& toAtEndOfDurationSVGLength = toAtEndOfDuration->length();
    SVGLength& animatedSVGLength = animated->length();

    // Apply CSS inheritance rules.
    m_animationElement->adjustForInheritance<SVGLength>(parseLengthFromString, m_animationElement->fromPropertyValueType(), fromSVGLength, m_contextElement);
    m_animationElement->adjustForInheritance<SVGLength>(parseLengthFromString, m_animationElement->toPropertyValueType(), toSVGLength, m_contextElement);

    SVGLengthContext lengthContext(m_contextElement);
    float animatedNumber = animatedSVGLength.value(lengthContext);
    SVGLengthType unitType = percentage < 0.5 ? fromSVGLength.unitType() : toSVGLength.unitType();
    m_animationElement->animateAdditiveNumber(percentage, repeatCount, fromSVGLength.value(lengthContext), toSVGLength.value(lengthContext), toAtEndOfDurationSVGLength.value(lengthContext), animatedNumber);

    animatedSVGLength.setValue(lengthContext, animatedNumber, m_lengthMode, unitType, ASSERT_NO_EXCEPTION);
}

float SVGAnimatedLengthAnimator::calculateDistance(const String& fromString, const String& toString)
{
    ASSERT(m_animationElement);
    ASSERT(m_contextElement);
    SVGLengthMode lengthMode = SVGLength::lengthModeForAnimatedLengthAttribute(m_animationElement->attributeName());
    SVGLength from = SVGLength(lengthMode, fromString);
    SVGLength to = SVGLength(lengthMode, toString);
    SVGLengthContext lengthContext(m_contextElement);
    return fabsf(to.value(lengthContext) - from.value(lengthContext));
}

}

#endif // ENABLE(SVG)
