/*
 * Copyright (C) Research In Motion Limited 2011-2012. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
#include "SVGAnimatedTypeAnimator.h"

#include "SVGAttributeToPropertyMap.h"
#include "SVGElement.h"

namespace WebCore {

SVGElementAnimatedProperties::SVGElementAnimatedProperties()
    : element(0)
{ }

SVGElementAnimatedProperties::SVGElementAnimatedProperties(SVGElement* element, Vector<RefPtr<SVGAnimatedProperty> >& properties)
    : element(element)
    , properties(properties)
{ }

SVGAnimatedTypeAnimator::SVGAnimatedTypeAnimator(AnimatedPropertyType type, SVGAnimationElement* animationElement, SVGElement* contextElement)
    : m_type(type)
    , m_animationElement(animationElement)
    , m_contextElement(contextElement)
{
}

SVGAnimatedTypeAnimator::~SVGAnimatedTypeAnimator()
{ }

void SVGAnimatedTypeAnimator::calculateFromAndToValues(OwnPtr<SVGAnimatedType>& from, OwnPtr<SVGAnimatedType>& to, const String& fromString, const String& toString)
{
    from = constructFromString(fromString);
    to = constructFromString(toString);
}

void SVGAnimatedTypeAnimator::calculateFromAndByValues(OwnPtr<SVGAnimatedType>& from, OwnPtr<SVGAnimatedType>& to, const String& fromString, const String& byString)
{
    from = constructFromString(fromString);
    to = constructFromString(byString);
    addAnimatedTypes(from.get(), to.get());
}

SVGElementAnimatedPropertyList SVGAnimatedTypeAnimator::findAnimatedPropertiesForAttributeName(SVGElement* targetElement, const QualifiedName& attributeName)
{
    ASSERT(targetElement);

    SVGElementAnimatedPropertyList propertiesByInstance;

    Vector<RefPtr<SVGAnimatedProperty> > targetProperties;
    targetElement->localAttributeToPropertyMap().animatedPropertiesForAttribute(targetElement, attributeName, targetProperties);

    if (!SVGAnimatedType::supportsAnimVal(m_type))
        return SVGElementAnimatedPropertyList();

    SVGElementAnimatedProperties propertiesPair(targetElement, targetProperties);
    propertiesByInstance.append(propertiesPair);

    const HashSet<SVGElementInstance*>& instances = targetElement->instancesForElement();
    const HashSet<SVGElementInstance*>::const_iterator end = instances.end();
    for (HashSet<SVGElementInstance*>::const_iterator it = instances.begin(); it != end; ++it) {
        SVGElement* shadowTreeElement = (*it)->shadowTreeElement();
        if (!shadowTreeElement)
            continue;

        Vector<RefPtr<SVGAnimatedProperty> > instanceProperties;
        targetElement->localAttributeToPropertyMap().animatedPropertiesForAttribute(shadowTreeElement, attributeName, instanceProperties);

        SVGElementAnimatedProperties instancePropertiesPair(shadowTreeElement, instanceProperties);
        propertiesByInstance.append(instancePropertiesPair);
    }

#if !ASSERT_DISABLED
    SVGElementAnimatedPropertyList::const_iterator propertiesEnd = propertiesByInstance.end();
    for (SVGElementAnimatedPropertyList::const_iterator it = propertiesByInstance.begin(); it != propertiesEnd; ++it) {
        size_t propertiesSize = it->properties.size();
        for (size_t i = 0; i < propertiesSize; ++i) {
            RefPtr<SVGAnimatedProperty> property = it->properties[i];
            if (property->animatedPropertyType() != m_type) {
                ASSERT(m_type == AnimatedAngle);
                ASSERT(property->animatedPropertyType() == AnimatedEnumeration);
            }
        }
    }
#endif

    return propertiesByInstance;
}

} // namespace WebCore

#endif // ENABLE(SVG)
