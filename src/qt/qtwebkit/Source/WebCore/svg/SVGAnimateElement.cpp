/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
#include "SVGAnimateElement.h"

#include "CSSParser.h"
#include "CSSPropertyNames.h"
#include "QualifiedName.h"
#include "RenderObject.h"
#include "SVGAnimatorFactory.h"
#include "SVGNames.h"
#include "SVGStyledElement.h"
#include "StylePropertySet.h"

namespace WebCore {

SVGAnimateElement::SVGAnimateElement(const QualifiedName& tagName, Document* document)
    : SVGAnimationElement(tagName, document)
    , m_animatedPropertyType(AnimatedString)
{
    ASSERT(hasTagName(SVGNames::animateTag) || hasTagName(SVGNames::setTag) || hasTagName(SVGNames::animateColorTag) || hasTagName(SVGNames::animateTransformTag));
}

PassRefPtr<SVGAnimateElement> SVGAnimateElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGAnimateElement(tagName, document));
}

SVGAnimateElement::~SVGAnimateElement()
{
}

bool SVGAnimateElement::hasValidAttributeType()
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return false;

    return m_animatedPropertyType != AnimatedUnknown && !hasInvalidCSSAttributeType();
}

AnimatedPropertyType SVGAnimateElement::determineAnimatedPropertyType(SVGElement* targetElement) const
{
    ASSERT(targetElement);

    Vector<AnimatedPropertyType> propertyTypes;
    targetElement->animatedPropertyTypeForAttribute(attributeName(), propertyTypes);
    if (propertyTypes.isEmpty())
        return AnimatedUnknown;

    ASSERT(propertyTypes.size() <= 2);
    AnimatedPropertyType type = propertyTypes[0];
    if (hasTagName(SVGNames::animateColorTag) && type != AnimatedColor)
        return AnimatedUnknown;

    // Animations of transform lists are not allowed for <animate> or <set>
    // http://www.w3.org/TR/SVG/animate.html#AnimationAttributesAndProperties
    if (type == AnimatedTransformList && !hasTagName(SVGNames::animateTransformTag))
        return AnimatedUnknown;

    // Fortunately there's just one special case needed here: SVGMarkerElements orientAttr, which
    // corresponds to SVGAnimatedAngle orientAngle and SVGAnimatedEnumeration orientType. We have to
    // figure out whose value to change here.
    if (targetElement->hasTagName(SVGNames::markerTag) && type == AnimatedAngle) {
        ASSERT(propertyTypes.size() == 2);
        ASSERT(propertyTypes[0] == AnimatedAngle);
        ASSERT(propertyTypes[1] == AnimatedEnumeration);
    } else if (propertyTypes.size() == 2)
        ASSERT(propertyTypes[0] == propertyTypes[1]);

    return type;
}

void SVGAnimateElement::calculateAnimatedValue(float percentage, unsigned repeatCount, SVGSMILElement* resultElement)
{
    ASSERT(resultElement);
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return;

    ASSERT(m_animatedPropertyType == determineAnimatedPropertyType(targetElement));

    ASSERT(percentage >= 0 && percentage <= 1);
    ASSERT(m_animatedPropertyType != AnimatedTransformList || hasTagName(SVGNames::animateTransformTag));
    ASSERT(m_animatedPropertyType != AnimatedUnknown);
    ASSERT(m_animator);
    ASSERT(m_animator->type() == m_animatedPropertyType);
    ASSERT(m_fromType);
    ASSERT(m_fromType->type() == m_animatedPropertyType);
    ASSERT(m_toType);

    ASSERT(resultElement->hasTagName(SVGNames::animateTag)
        || resultElement->hasTagName(SVGNames::animateColorTag)
        || resultElement->hasTagName(SVGNames::animateTransformTag)
        || resultElement->hasTagName(SVGNames::setTag));

    SVGAnimateElement* resultAnimationElement = static_cast<SVGAnimateElement*>(resultElement);
    ASSERT(resultAnimationElement->m_animatedType);
    ASSERT(resultAnimationElement->m_animatedPropertyType == m_animatedPropertyType);

    if (hasTagName(SVGNames::setTag))
        percentage = 1;

    if (calcMode() == CalcModeDiscrete)
        percentage = percentage < 0.5 ? 0 : 1;

    // Target element might have changed.
    m_animator->setContextElement(targetElement);

    // Be sure to detach list wrappers before we modfiy their underlying value. If we'd do
    // if after calculateAnimatedValue() ran the cached pointers in the list propery tear
    // offs would point nowhere, and we couldn't create copies of those values anymore,
    // while detaching. This is covered by assertions, moving this down would fire them.
    if (!m_animatedProperties.isEmpty())
        m_animator->animValWillChange(m_animatedProperties);

    // Values-animation accumulates using the last values entry corresponding to the end of duration time.
    SVGAnimatedType* toAtEndOfDurationType = m_toAtEndOfDurationType ? m_toAtEndOfDurationType.get() : m_toType.get();
    m_animator->calculateAnimatedValue(percentage, repeatCount, m_fromType.get(), m_toType.get(), toAtEndOfDurationType, resultAnimationElement->m_animatedType.get());
}

bool SVGAnimateElement::calculateToAtEndOfDurationValue(const String& toAtEndOfDurationString)
{
    if (toAtEndOfDurationString.isEmpty())
        return false;
    m_toAtEndOfDurationType = ensureAnimator()->constructFromString(toAtEndOfDurationString);
    return true;
}

bool SVGAnimateElement::calculateFromAndToValues(const String& fromString, const String& toString)
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return false;

    determinePropertyValueTypes(fromString, toString);
    ensureAnimator()->calculateFromAndToValues(m_fromType, m_toType, fromString, toString);
    ASSERT(m_animatedPropertyType == m_animator->type());
    return true;
}

bool SVGAnimateElement::calculateFromAndByValues(const String& fromString, const String& byString)
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return false;

    if (animationMode() == ByAnimation && !isAdditive())
        return false;

    ASSERT(!hasTagName(SVGNames::setTag));

    determinePropertyValueTypes(fromString, byString);
    ensureAnimator()->calculateFromAndByValues(m_fromType, m_toType, fromString, byString);
    ASSERT(m_animatedPropertyType == m_animator->type());
    return true;
}

#ifndef NDEBUG
static inline bool propertyTypesAreConsistent(AnimatedPropertyType expectedPropertyType, const SVGElementAnimatedPropertyList& animatedTypes)
{
    SVGElementAnimatedPropertyList::const_iterator end = animatedTypes.end();
    for (SVGElementAnimatedPropertyList::const_iterator it = animatedTypes.begin(); it != end; ++it) {
        for (size_t i = 0; i < it->properties.size(); ++i) {
            if (expectedPropertyType != it->properties[i]->animatedPropertyType()) {
                // This is the only allowed inconsistency. SVGAnimatedAngleAnimator handles both SVGAnimatedAngle & SVGAnimatedEnumeration for markers orient attribute.
                if (expectedPropertyType == AnimatedAngle && it->properties[i]->animatedPropertyType() == AnimatedEnumeration)
                    return true;
                return false;
            }
        }
    }

    return true;
}
#endif

void SVGAnimateElement::resetAnimatedType()
{
    SVGAnimatedTypeAnimator* animator = ensureAnimator();
    ASSERT(m_animatedPropertyType == animator->type());

    SVGElement* targetElement = this->targetElement();
    const QualifiedName& attributeName = this->attributeName();
    ShouldApplyAnimation shouldApply = shouldApplyAnimation(targetElement, attributeName);

    if (shouldApply == DontApplyAnimation)
        return;

    if (shouldApply == ApplyXMLAnimation) {
        // SVG DOM animVal animation code-path.
        m_animatedProperties = animator->findAnimatedPropertiesForAttributeName(targetElement, attributeName);
        ASSERT(!m_animatedProperties.isEmpty());

        ASSERT(propertyTypesAreConsistent(m_animatedPropertyType, m_animatedProperties));
        if (!m_animatedType)
            m_animatedType = animator->startAnimValAnimation(m_animatedProperties);
        else {
            animator->resetAnimValToBaseVal(m_animatedProperties, m_animatedType.get());
            animator->animValDidChange(m_animatedProperties);
        }
        return;
    }

    // CSS properties animation code-path.
    ASSERT(m_animatedProperties.isEmpty());
    String baseValue;

    if (shouldApply == ApplyCSSAnimation) {
        ASSERT(SVGAnimationElement::isTargetAttributeCSSProperty(targetElement, attributeName));
        computeCSSPropertyValue(targetElement, cssPropertyID(attributeName.localName()), baseValue);
    }

    if (!m_animatedType)
        m_animatedType = animator->constructFromString(baseValue);
    else
        m_animatedType->setValueAsString(attributeName, baseValue);
}

static inline void applyCSSPropertyToTarget(SVGElement* targetElement, CSSPropertyID id, const String& value)
{
    ASSERT(!targetElement->m_deletionHasBegun);

    MutableStylePropertySet* propertySet = targetElement->ensureAnimatedSMILStyleProperties();
    if (!propertySet->setProperty(id, value, false, 0))
        return;

    targetElement->setNeedsStyleRecalc(SyntheticStyleChange);
}

static inline void removeCSSPropertyFromTarget(SVGElement* targetElement, CSSPropertyID id)
{
    ASSERT(!targetElement->m_deletionHasBegun);
    targetElement->ensureAnimatedSMILStyleProperties()->removeProperty(id);
    targetElement->setNeedsStyleRecalc(SyntheticStyleChange);
}

static inline void applyCSSPropertyToTargetAndInstances(SVGElement* targetElement, const QualifiedName& attributeName, const String& valueAsString)
{
    ASSERT(targetElement);
    if (attributeName == anyQName() || !targetElement->inDocument() || !targetElement->parentNode())
        return;

    CSSPropertyID id = cssPropertyID(attributeName.localName());

    SVGElementInstance::InstanceUpdateBlocker blocker(targetElement);
    applyCSSPropertyToTarget(targetElement, id, valueAsString);

    // If the target element has instances, update them as well, w/o requiring the <use> tree to be rebuilt.
    const HashSet<SVGElementInstance*>& instances = targetElement->instancesForElement();
    const HashSet<SVGElementInstance*>::const_iterator end = instances.end();
    for (HashSet<SVGElementInstance*>::const_iterator it = instances.begin(); it != end; ++it) {
        if (SVGElement* shadowTreeElement = (*it)->shadowTreeElement())
            applyCSSPropertyToTarget(shadowTreeElement, id, valueAsString);
    }
}

static inline void removeCSSPropertyFromTargetAndInstances(SVGElement* targetElement, const QualifiedName& attributeName)
{
    ASSERT(targetElement);
    if (attributeName == anyQName() || !targetElement->inDocument() || !targetElement->parentNode())
        return;

    CSSPropertyID id = cssPropertyID(attributeName.localName());

    SVGElementInstance::InstanceUpdateBlocker blocker(targetElement);
    removeCSSPropertyFromTarget(targetElement, id);

    // If the target element has instances, update them as well, w/o requiring the <use> tree to be rebuilt.
    const HashSet<SVGElementInstance*>& instances = targetElement->instancesForElement();
    const HashSet<SVGElementInstance*>::const_iterator end = instances.end();
    for (HashSet<SVGElementInstance*>::const_iterator it = instances.begin(); it != end; ++it) {
        if (SVGElement* shadowTreeElement = (*it)->shadowTreeElement())
            removeCSSPropertyFromTarget(shadowTreeElement, id);
    }
}

static inline void notifyTargetAboutAnimValChange(SVGElement* targetElement, const QualifiedName& attributeName)
{
    ASSERT(!targetElement->m_deletionHasBegun);
    targetElement->svgAttributeChanged(attributeName);
}

static inline void notifyTargetAndInstancesAboutAnimValChange(SVGElement* targetElement, const QualifiedName& attributeName)
{
    ASSERT(targetElement);
    if (attributeName == anyQName() || !targetElement->inDocument() || !targetElement->parentNode())
        return;

    SVGElementInstance::InstanceUpdateBlocker blocker(targetElement);
    notifyTargetAboutAnimValChange(targetElement, attributeName);

    // If the target element has instances, update them as well, w/o requiring the <use> tree to be rebuilt.
    const HashSet<SVGElementInstance*>& instances = targetElement->instancesForElement();
    const HashSet<SVGElementInstance*>::const_iterator end = instances.end();
    for (HashSet<SVGElementInstance*>::const_iterator it = instances.begin(); it != end; ++it) {
        if (SVGElement* shadowTreeElement = (*it)->shadowTreeElement())
            notifyTargetAboutAnimValChange(shadowTreeElement, attributeName);
    }
}

void SVGAnimateElement::clearAnimatedType(SVGElement* targetElement)
{
    if (!m_animatedType)
        return;

    if (!targetElement) {
        m_animatedType.clear();
        return;
    }

    if (m_animatedProperties.isEmpty()) {
        // CSS properties animation code-path.
        removeCSSPropertyFromTargetAndInstances(targetElement, attributeName());
        m_animatedType.clear();
        return;
    }

    // SVG DOM animVal animation code-path.
    if (m_animator) {
        m_animator->stopAnimValAnimation(m_animatedProperties);
        notifyTargetAndInstancesAboutAnimValChange(targetElement, attributeName());
    }

    m_animatedProperties.clear();
    m_animatedType.clear();
}

void SVGAnimateElement::applyResultsToTarget()
{
    ASSERT(m_animatedPropertyType != AnimatedTransformList || hasTagName(SVGNames::animateTransformTag));
    ASSERT(m_animatedPropertyType != AnimatedUnknown);
    ASSERT(m_animator);

    // Early exit if our animated type got destructed by a previous endedActiveInterval().
    if (!m_animatedType)
        return;

    if (m_animatedProperties.isEmpty()) {
        // CSS properties animation code-path.
        // Convert the result of the animation to a String and apply it as CSS property on the target & all instances.
        applyCSSPropertyToTargetAndInstances(targetElement(), attributeName(), m_animatedType->valueAsString());
        return;
    }

    // SVG DOM animVal animation code-path.
    // At this point the SVG DOM values are already changed, unlike for CSS.
    // We only have to trigger update notifications here.
    m_animator->animValDidChange(m_animatedProperties);
    notifyTargetAndInstancesAboutAnimValChange(targetElement(), attributeName());
}

bool SVGAnimateElement::isAdditive() const
{
    if (animationMode() == ByAnimation || animationMode() == FromByAnimation) {
        // Spec: http://www.w3.org/TR/SVG/animate.html#AnimationAttributesAndProperties.
        switch (m_animatedPropertyType) {
        case AnimatedBoolean:
        case AnimatedEnumeration:
        case AnimatedPreserveAspectRatio:
        case AnimatedString:
        case AnimatedUnknown:
            return false;
        default:
            break;
        }
    }

    return SVGAnimationElement::isAdditive();
}

float SVGAnimateElement::calculateDistance(const String& fromString, const String& toString)
{
    // FIXME: A return value of float is not enough to support paced animations on lists.
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return -1;

    return ensureAnimator()->calculateDistance(fromString, toString);
}

void SVGAnimateElement::setTargetElement(SVGElement* target)
{
    SVGAnimationElement::setTargetElement(target);
    resetAnimatedPropertyType();
}

void SVGAnimateElement::setAttributeName(const QualifiedName& attributeName)
{
    SVGAnimationElement::setAttributeName(attributeName);
    resetAnimatedPropertyType();
}

void SVGAnimateElement::resetAnimatedPropertyType()
{
    ASSERT(!m_animatedType);
    m_fromType.clear();
    m_toType.clear();
    m_toAtEndOfDurationType.clear();
    m_animator.clear();
    m_animatedPropertyType = targetElement() ? determineAnimatedPropertyType(targetElement()) : AnimatedString;
}

SVGAnimatedTypeAnimator* SVGAnimateElement::ensureAnimator()
{
    if (!m_animator)
        m_animator = SVGAnimatorFactory::create(this, targetElement(), m_animatedPropertyType);
    ASSERT(m_animatedPropertyType == m_animator->type());
    return m_animator.get();
}

}

#endif // ENABLE(SVG)
