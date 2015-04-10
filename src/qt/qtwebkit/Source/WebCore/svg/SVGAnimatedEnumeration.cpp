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
#include "SVGAnimatedEnumeration.h"

#include "SVGAnimationElement.h"
#include "SVGComponentTransferFunctionElement.h"
#include "SVGFEBlendElement.h"
#include "SVGFEColorMatrixElement.h"
#include "SVGFECompositeElement.h"
#include "SVGFEConvolveMatrixElement.h"
#include "SVGFEDisplacementMapElement.h"
#include "SVGFEMorphologyElement.h"
#include "SVGFETurbulenceElement.h"
#include "SVGGradientElement.h"
#include "SVGMarkerElement.h"
#include "SVGNames.h"
#include "SVGTextContentElement.h"
#include "SVGTextPathElement.h"
#include "SVGUnitTypes.h"

namespace WebCore {

static inline unsigned enumerationValueForTargetAttribute(SVGElement* targetElement, const QualifiedName& attrName, const String& value)
{
    ASSERT(targetElement);
    if (attrName == SVGNames::clipPathUnitsAttr
        || attrName == SVGNames::filterUnitsAttr
        || attrName == SVGNames::gradientUnitsAttr
        || attrName == SVGNames::maskContentUnitsAttr
        || attrName == SVGNames::maskUnitsAttr
        || attrName == SVGNames::patternContentUnitsAttr
        || attrName == SVGNames::patternUnitsAttr
        || attrName == SVGNames::primitiveUnitsAttr)
        return SVGPropertyTraits<SVGUnitTypes::SVGUnitType>::fromString(value);

    if (attrName == SVGNames::lengthAdjustAttr)
        return SVGPropertyTraits<SVGLengthAdjustType>::fromString(value);
    if (attrName == SVGNames::markerUnitsAttr)
        return SVGPropertyTraits<SVGMarkerUnitsType>::fromString(value);
    if (attrName == SVGNames::methodAttr)
        return SVGPropertyTraits<SVGTextPathMethodType>::fromString(value);
    if (attrName == SVGNames::spacingAttr)
        return SVGPropertyTraits<SVGTextPathSpacingType>::fromString(value);
    if (attrName == SVGNames::spreadMethodAttr)
        return SVGPropertyTraits<SVGSpreadMethodType>::fromString(value);

#if ENABLE(FILTERS)
    if (attrName == SVGNames::edgeModeAttr)
        return SVGPropertyTraits<EdgeModeType>::fromString(value);

    if (attrName == SVGNames::operatorAttr) {
        if (targetElement->hasTagName(SVGNames::feCompositeTag))
            return SVGPropertyTraits<CompositeOperationType>::fromString(value);
        ASSERT(targetElement->hasTagName(SVGNames::feMorphologyTag));
        return SVGPropertyTraits<MorphologyOperatorType>::fromString(value);
    }

    if (attrName == SVGNames::typeAttr) {
        if (targetElement->hasTagName(SVGNames::feColorMatrixTag))
            return SVGPropertyTraits<ColorMatrixType>::fromString(value);
        if (targetElement->hasTagName(SVGNames::feTurbulenceTag))
            return SVGPropertyTraits<TurbulenceType>::fromString(value);

        ASSERT(targetElement->hasTagName(SVGNames::feFuncATag)
               || targetElement->hasTagName(SVGNames::feFuncBTag)
               || targetElement->hasTagName(SVGNames::feFuncGTag)
               || targetElement->hasTagName(SVGNames::feFuncRTag));
        return SVGPropertyTraits<ComponentTransferType>::fromString(value);
    }

    if (attrName == SVGNames::modeAttr)
        return SVGPropertyTraits<BlendModeType>::fromString(value);
    if (attrName == SVGNames::stitchTilesAttr)
        return SVGPropertyTraits<SVGStitchOptions>::fromString(value);
    if (attrName == SVGNames::xChannelSelectorAttr)
        return SVGPropertyTraits<ChannelSelectorType>::fromString(value);
    if (attrName == SVGNames::yChannelSelectorAttr)
        return SVGPropertyTraits<ChannelSelectorType>::fromString(value);
#endif

    ASSERT_NOT_REACHED();
    return 0;
}

SVGAnimatedEnumerationAnimator::SVGAnimatedEnumerationAnimator(SVGAnimationElement* animationElement, SVGElement* contextElement)
    : SVGAnimatedTypeAnimator(AnimatedEnumeration, animationElement, contextElement)
{
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedEnumerationAnimator::constructFromString(const String& string)
{
    ASSERT(m_animationElement);
    OwnPtr<SVGAnimatedType> animatedType = SVGAnimatedType::createEnumeration(new unsigned);
    animatedType->enumeration() = enumerationValueForTargetAttribute(m_animationElement->targetElement(), m_animationElement->attributeName(), string);
    return animatedType.release();
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedEnumerationAnimator::startAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    return SVGAnimatedType::createEnumeration(constructFromBaseValue<SVGAnimatedEnumeration>(animatedTypes));
}

void SVGAnimatedEnumerationAnimator::stopAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    stopAnimValAnimationForType<SVGAnimatedEnumeration>(animatedTypes);
}

void SVGAnimatedEnumerationAnimator::resetAnimValToBaseVal(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType* type)
{
    resetFromBaseValue<SVGAnimatedEnumeration>(animatedTypes, type, &SVGAnimatedType::enumeration);
}

void SVGAnimatedEnumerationAnimator::animValWillChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValWillChangeForType<SVGAnimatedEnumeration>(animatedTypes);
}

void SVGAnimatedEnumerationAnimator::animValDidChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValDidChangeForType<SVGAnimatedEnumeration>(animatedTypes);
}

void SVGAnimatedEnumerationAnimator::addAnimatedTypes(SVGAnimatedType*, SVGAnimatedType*)
{
    ASSERT_NOT_REACHED();
}

void SVGAnimatedEnumerationAnimator::calculateAnimatedValue(float percentage, unsigned, SVGAnimatedType* from, SVGAnimatedType* to, SVGAnimatedType*, SVGAnimatedType* animated)
{
    ASSERT(m_animationElement);
    ASSERT(m_contextElement);

    unsigned fromEnumeration = m_animationElement->animationMode() == ToAnimation ? animated->enumeration() : from->enumeration();
    unsigned toEnumeration = to->enumeration();
    unsigned& animatedEnumeration = animated->enumeration();

    m_animationElement->animateDiscreteType<unsigned>(percentage, fromEnumeration, toEnumeration, animatedEnumeration);
}

float SVGAnimatedEnumerationAnimator::calculateDistance(const String&, const String&)
{
    // No paced animations for enumerations.
    return -1;
}

}

#endif // ENABLE(SVG)
