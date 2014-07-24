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
#include "SVGAnimatedLengthList.h"

#include "SVGAnimateElement.h"
#include "SVGAnimatedNumber.h"

namespace WebCore {

SVGAnimatedLengthListAnimator::SVGAnimatedLengthListAnimator(SVGAnimationElement* animationElement, SVGElement* contextElement)
    : SVGAnimatedTypeAnimator(AnimatedLengthList, animationElement, contextElement)
    , m_lengthMode(SVGLength::lengthModeForAnimatedLengthAttribute(animationElement->attributeName()))
{
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedLengthListAnimator::constructFromString(const String& string)
{
    OwnPtr<SVGAnimatedType> animateType = SVGAnimatedType::createLengthList(new SVGLengthList);
    animateType->lengthList().parse(string, m_lengthMode);
    return animateType.release();
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedLengthListAnimator::startAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    return SVGAnimatedType::createLengthList(constructFromBaseValue<SVGAnimatedLengthList>(animatedTypes));
}

void SVGAnimatedLengthListAnimator::stopAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    stopAnimValAnimationForType<SVGAnimatedLengthList>(animatedTypes);
}

void SVGAnimatedLengthListAnimator::resetAnimValToBaseVal(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType* type)
{
    resetFromBaseValue<SVGAnimatedLengthList>(animatedTypes, type, &SVGAnimatedType::lengthList);
}

void SVGAnimatedLengthListAnimator::animValWillChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValWillChangeForType<SVGAnimatedLengthList>(animatedTypes);
}

void SVGAnimatedLengthListAnimator::animValDidChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValDidChangeForType<SVGAnimatedLengthList>(animatedTypes);
}

void SVGAnimatedLengthListAnimator::addAnimatedTypes(SVGAnimatedType* from, SVGAnimatedType* to)
{
    ASSERT(from->type() == AnimatedLengthList);
    ASSERT(from->type() == to->type());

    const SVGLengthList& fromLengthList = from->lengthList();
    SVGLengthList& toLengthList = to->lengthList();

    unsigned fromLengthListSize = fromLengthList.size();
    if (!fromLengthListSize || fromLengthListSize != toLengthList.size())
        return;

    SVGLengthContext lengthContext(m_contextElement);
    for (unsigned i = 0; i < fromLengthListSize; ++i)
        toLengthList[i].setValue(toLengthList[i].value(lengthContext) + fromLengthList[i].value(lengthContext), lengthContext, ASSERT_NO_EXCEPTION);
}

static SVGLengthList parseLengthListFromString(SVGAnimationElement* animationElement, const String& string)
{
    SVGLengthList lengthList;
    lengthList.parse(string, SVGLength::lengthModeForAnimatedLengthAttribute(animationElement->attributeName()));
    return lengthList;
}

void SVGAnimatedLengthListAnimator::calculateAnimatedValue(float percentage, unsigned repeatCount, SVGAnimatedType* from, SVGAnimatedType* to, SVGAnimatedType* toAtEndOfDuration, SVGAnimatedType* animated)
{
    ASSERT(m_animationElement);
    ASSERT(m_contextElement);

    SVGLengthList fromLengthList = m_animationElement->animationMode() == ToAnimation ? animated->lengthList() : from->lengthList();
    SVGLengthList toLengthList = to->lengthList();
    const SVGLengthList& toAtEndOfDurationLengthList = toAtEndOfDuration->lengthList();
    SVGLengthList& animatedLengthList = animated->lengthList();

    // Apply CSS inheritance rules.
    m_animationElement->adjustForInheritance<SVGLengthList>(parseLengthListFromString, m_animationElement->fromPropertyValueType(), fromLengthList, m_contextElement);
    m_animationElement->adjustForInheritance<SVGLengthList>(parseLengthListFromString, m_animationElement->toPropertyValueType(), toLengthList, m_contextElement);

    if (!m_animationElement->adjustFromToListValues<SVGLengthList>(fromLengthList, toLengthList, animatedLengthList, percentage))
        return;

    unsigned fromLengthListSize = fromLengthList.size();
    unsigned toLengthListSize = toLengthList.size();
    unsigned toAtEndOfDurationListSize = toAtEndOfDurationLengthList.size();

    SVGLengthContext lengthContext(m_contextElement);
    for (unsigned i = 0; i < toLengthListSize; ++i) {
        float animatedNumber = animatedLengthList[i].value(lengthContext);
        SVGLengthType unitType = toLengthList[i].unitType();
        float effectiveFrom = 0;
        if (fromLengthListSize) {
            if (percentage < 0.5)
                unitType = fromLengthList[i].unitType();
            effectiveFrom = fromLengthList[i].value(lengthContext);
        }
        float effectiveToAtEnd = i < toAtEndOfDurationListSize ? toAtEndOfDurationLengthList[i].value(lengthContext) : 0;

        m_animationElement->animateAdditiveNumber(percentage, repeatCount, effectiveFrom, toLengthList[i].value(lengthContext), effectiveToAtEnd, animatedNumber);
        animatedLengthList[i].setValue(lengthContext, animatedNumber, m_lengthMode, unitType, ASSERT_NO_EXCEPTION);
    }
}

float SVGAnimatedLengthListAnimator::calculateDistance(const String&, const String&)
{
    // FIXME: Distance calculation is not possible for SVGLengthList right now. We need the distance for every single value.
    return -1;
}

}

#endif // ENABLE(SVG)
