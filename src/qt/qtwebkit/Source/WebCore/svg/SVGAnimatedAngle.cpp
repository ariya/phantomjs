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
#include "SVGAnimatedAngle.h"

#include "SVGAnimateElement.h"
#include "SVGMarkerElement.h"

namespace WebCore {

SVGAnimatedAngleAnimator::SVGAnimatedAngleAnimator(SVGAnimationElement* animationElement, SVGElement* contextElement)
    : SVGAnimatedTypeAnimator(AnimatedAngle, animationElement, contextElement)
{
}

static inline SVGAngle& sharedSVGAngle(const String& valueAsString)
{
    DEFINE_STATIC_LOCAL(SVGAngle, sharedAngle, ());
    sharedAngle.setValueAsString(valueAsString, ASSERT_NO_EXCEPTION);
    return sharedAngle;
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedAngleAnimator::constructFromString(const String& string)
{
    OwnPtr<SVGAnimatedType> animatedType = SVGAnimatedType::createAngleAndEnumeration(new pair<SVGAngle, unsigned>);
    pair<SVGAngle, unsigned>& animatedPair = animatedType->angleAndEnumeration();

    SVGAngle angle;
    SVGMarkerOrientType orientType = SVGPropertyTraits<SVGMarkerOrientType>::fromString(string,  angle);
    if (orientType > 0)
        animatedPair.second = orientType;
    if (orientType == SVGMarkerOrientAngle)
        animatedPair.first = angle;

    return animatedType.release();
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedAngleAnimator::startAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    return SVGAnimatedType::createAngleAndEnumeration(constructFromBaseValues<SVGAnimatedAngle, SVGAnimatedEnumeration>(animatedTypes));
}

void SVGAnimatedAngleAnimator::stopAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    stopAnimValAnimationForTypes<SVGAnimatedAngle, SVGAnimatedEnumeration>(animatedTypes);
}

void SVGAnimatedAngleAnimator::resetAnimValToBaseVal(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType* type)
{
    resetFromBaseValues<SVGAnimatedAngle, SVGAnimatedEnumeration>(animatedTypes, type, &SVGAnimatedType::angleAndEnumeration);
}

void SVGAnimatedAngleAnimator::animValWillChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValWillChangeForTypes<SVGAnimatedAngle, SVGAnimatedEnumeration>(animatedTypes);
}

void SVGAnimatedAngleAnimator::animValDidChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValDidChangeForTypes<SVGAnimatedAngle, SVGAnimatedEnumeration>(animatedTypes);
}

void SVGAnimatedAngleAnimator::addAnimatedTypes(SVGAnimatedType* from, SVGAnimatedType* to)
{
    ASSERT(from->type() == AnimatedAngle);
    ASSERT(from->type() == to->type());

    const pair<SVGAngle, unsigned>& fromAngleAndEnumeration = from->angleAndEnumeration();
    pair<SVGAngle, unsigned>& toAngleAndEnumeration = to->angleAndEnumeration();
    // Only respect by animations, if from and by are both specified in angles (and not eg. 'auto').
    if (fromAngleAndEnumeration.second != toAngleAndEnumeration.second || fromAngleAndEnumeration.second != SVGMarkerOrientAngle)
        return;
    const SVGAngle& fromAngle = fromAngleAndEnumeration.first;
    SVGAngle& toAngle = toAngleAndEnumeration.first;
    toAngle.setValue(toAngle.value() + fromAngle.value());
}

void SVGAnimatedAngleAnimator::calculateAnimatedValue(float percentage, unsigned repeatCount, SVGAnimatedType* from, SVGAnimatedType* to, SVGAnimatedType* toAtEndOfDuration, SVGAnimatedType* animated)
{
    ASSERT(m_animationElement);
    ASSERT(m_contextElement);

    const pair<SVGAngle, unsigned>& fromAngleAndEnumeration = m_animationElement->animationMode() == ToAnimation ? animated->angleAndEnumeration() : from->angleAndEnumeration();
    const pair<SVGAngle, unsigned>& toAngleAndEnumeration = to->angleAndEnumeration();
    const pair<SVGAngle, unsigned>& toAtEndOfDurationAngleAndEnumeration = toAtEndOfDuration->angleAndEnumeration();
    pair<SVGAngle, unsigned>& animatedAngleAndEnumeration = animated->angleAndEnumeration();

    if (fromAngleAndEnumeration.second != toAngleAndEnumeration.second) {
        // Animating from eg. auto to 90deg, or auto to 90deg.
        if (fromAngleAndEnumeration.second == SVGMarkerOrientAngle) {
            // Animating from an angle value to eg. 'auto' - this disabled additive as 'auto' is a keyword..
            if (toAngleAndEnumeration.second == SVGMarkerOrientAuto) {
                if (percentage < 0.5f) {
                    animatedAngleAndEnumeration.first = fromAngleAndEnumeration.first;
                    animatedAngleAndEnumeration.second = SVGMarkerOrientAngle;
                    return;
                }
                animatedAngleAndEnumeration.first.setValue(0);
                animatedAngleAndEnumeration.second = SVGMarkerOrientAuto;
                return;
            }
            animatedAngleAndEnumeration.first.setValue(0);
            animatedAngleAndEnumeration.second = SVGMarkerOrientUnknown;
            return;
        }
    }

    // From 'auto' to 'auto'.
    if (fromAngleAndEnumeration.second == SVGMarkerOrientAuto) {
        animatedAngleAndEnumeration.first.setValue(0);
        animatedAngleAndEnumeration.second = SVGMarkerOrientAuto;
        return;
    }

    // If the enumeration value is not angle or auto, its unknown.
    if (fromAngleAndEnumeration.second != SVGMarkerOrientAngle) {
        animatedAngleAndEnumeration.first.setValue(0);
        animatedAngleAndEnumeration.second = SVGMarkerOrientUnknown;
        return;
    }

    // Regular from angle to angle animation, with all features like additive etc.
    animatedAngleAndEnumeration.second = SVGMarkerOrientAngle;

    SVGAngle& animatedSVGAngle = animatedAngleAndEnumeration.first;
    const SVGAngle& toAtEndOfDurationSVGAngle = toAtEndOfDurationAngleAndEnumeration.first;
    float animatedAngle = animatedSVGAngle.value();
    m_animationElement->animateAdditiveNumber(percentage, repeatCount, fromAngleAndEnumeration.first.value(), toAngleAndEnumeration.first.value(), toAtEndOfDurationSVGAngle.value(), animatedAngle);
    animatedSVGAngle.setValue(animatedAngle);
}

float SVGAnimatedAngleAnimator::calculateDistance(const String& fromString, const String& toString)
{
    SVGAngle from = SVGAngle();
    from.setValueAsString(fromString, ASSERT_NO_EXCEPTION);
    SVGAngle to = SVGAngle();
    to.setValueAsString(toString, ASSERT_NO_EXCEPTION);
    return fabsf(to.value() - from.value());
}

}

#endif // ENABLE(SVG)
