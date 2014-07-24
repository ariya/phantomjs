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
#include "SVGAnimatedPreserveAspectRatio.h"

#include "SVGAnimateElement.h"

namespace WebCore {
    
SVGAnimatedPreserveAspectRatioAnimator::SVGAnimatedPreserveAspectRatioAnimator(SVGAnimationElement* animationElement, SVGElement* contextElement)
    : SVGAnimatedTypeAnimator(AnimatedPreserveAspectRatio, animationElement, contextElement)
{
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedPreserveAspectRatioAnimator::constructFromString(const String& string)
{
    OwnPtr<SVGAnimatedType> animatedType = SVGAnimatedType::createPreserveAspectRatio(new SVGPreserveAspectRatio);
    animatedType->preserveAspectRatio().parse(string);
    return animatedType.release();
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedPreserveAspectRatioAnimator::startAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    return SVGAnimatedType::createPreserveAspectRatio(constructFromBaseValue<SVGAnimatedPreserveAspectRatio>(animatedTypes));
}

void SVGAnimatedPreserveAspectRatioAnimator::stopAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    stopAnimValAnimationForType<SVGAnimatedPreserveAspectRatio>(animatedTypes);
}

void SVGAnimatedPreserveAspectRatioAnimator::resetAnimValToBaseVal(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType* type)
{
    resetFromBaseValue<SVGAnimatedPreserveAspectRatio>(animatedTypes, type, &SVGAnimatedType::preserveAspectRatio);
}

void SVGAnimatedPreserveAspectRatioAnimator::animValWillChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValWillChangeForType<SVGAnimatedPreserveAspectRatio>(animatedTypes);
}

void SVGAnimatedPreserveAspectRatioAnimator::animValDidChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValDidChangeForType<SVGAnimatedPreserveAspectRatio>(animatedTypes);
}

void SVGAnimatedPreserveAspectRatioAnimator::addAnimatedTypes(SVGAnimatedType*, SVGAnimatedType*)
{
    ASSERT_NOT_REACHED();
}

void SVGAnimatedPreserveAspectRatioAnimator::calculateAnimatedValue(float percentage, unsigned, SVGAnimatedType* from, SVGAnimatedType* to, SVGAnimatedType*, SVGAnimatedType* animated)
{
    ASSERT(m_animationElement);
    ASSERT(m_contextElement);

    const SVGPreserveAspectRatio& fromPreserveAspectRatio = m_animationElement->animationMode() == ToAnimation ? animated->preserveAspectRatio() : from->preserveAspectRatio();
    const SVGPreserveAspectRatio& toPreserveAspectRatio = to->preserveAspectRatio();
    SVGPreserveAspectRatio& animatedPreserveAspectRatio = animated->preserveAspectRatio();

    m_animationElement->animateDiscreteType<SVGPreserveAspectRatio>(percentage, fromPreserveAspectRatio, toPreserveAspectRatio, animatedPreserveAspectRatio);
}

float SVGAnimatedPreserveAspectRatioAnimator::calculateDistance(const String&, const String&)
{
    // No paced animations for SVGPreserveAspectRatio.
    return -1;
}

}

#endif // ENABLE(SVG)
