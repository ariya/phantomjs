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
#include "SVGAttributeToPropertyMap.h"

#include "SVGAnimatedProperty.h"
#include "SVGPropertyInfo.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

void SVGAttributeToPropertyMap::addProperties(const SVGAttributeToPropertyMap& map)
{
    AttributeToPropertiesMap::const_iterator end = map.m_map.end();
    for (AttributeToPropertiesMap::const_iterator it = map.m_map.begin(); it != end; ++it) {
        const PropertiesVector* vector = it->value.get();
        ASSERT(vector);

        // FIXME: This looks up the attribute name in the hash table for each property, even though all the
        // properties in a single vector are guaranteed to have the same attribute name.
        // FIXME: This grows the vector one item at a time, even though we know up front exactly how many
        // elements we are adding to the vector.
        PropertiesVector::const_iterator vectorEnd = vector->end();
        for (PropertiesVector::const_iterator vectorIt = vector->begin(); vectorIt != vectorEnd; ++vectorIt)
            addProperty(*vectorIt);
    }
}

void SVGAttributeToPropertyMap::addProperty(const SVGPropertyInfo* info)
{
    ASSERT(info);
    ASSERT(info->attributeName != anyQName());
    if (PropertiesVector* vector = m_map.get(info->attributeName)) {
        vector->append(info);
        return;
    }
    // FIXME: This does a second hash table lookup, but with HashMap::add we could instead do only one.
    OwnPtr<PropertiesVector> vector = adoptPtr(new PropertiesVector);
    vector->append(info);
    m_map.set(info->attributeName, vector.release());
}

void SVGAttributeToPropertyMap::animatedPropertiesForAttribute(SVGElement* ownerType, const QualifiedName& attributeName, Vector<RefPtr<SVGAnimatedProperty> >& properties)
{
    ASSERT(ownerType);
    PropertiesVector* vector = m_map.get(attributeName);
    if (!vector)
        return;

    PropertiesVector::iterator vectorEnd = vector->end();
    for (PropertiesVector::iterator vectorIt = vector->begin(); vectorIt != vectorEnd; ++vectorIt)
        properties.append(animatedProperty(ownerType, attributeName, *vectorIt));
}

void SVGAttributeToPropertyMap::animatedPropertyTypeForAttribute(const QualifiedName& attributeName, Vector<AnimatedPropertyType>& propertyTypes)
{
    PropertiesVector* vector = m_map.get(attributeName);
    if (!vector)
        return;

    PropertiesVector::iterator vectorEnd = vector->end();
    for (PropertiesVector::iterator vectorIt = vector->begin(); vectorIt != vectorEnd; ++vectorIt)
        propertyTypes.append((*vectorIt)->animatedPropertyType);
}

void SVGAttributeToPropertyMap::synchronizeProperties(SVGElement* contextElement)
{
    ASSERT(contextElement);
    AttributeToPropertiesMap::iterator end = m_map.end();
    for (AttributeToPropertiesMap::iterator it = m_map.begin(); it != end; ++it) {
        PropertiesVector* vector = it->value.get();
        ASSERT(vector);

        PropertiesVector::iterator vectorEnd = vector->end();
        for (PropertiesVector::iterator vectorIt = vector->begin(); vectorIt != vectorEnd; ++vectorIt)
            synchronizeProperty(contextElement, it->key, *vectorIt);
    } 
}

bool SVGAttributeToPropertyMap::synchronizeProperty(SVGElement* contextElement, const QualifiedName& attributeName)
{
    ASSERT(contextElement);
    PropertiesVector* vector = m_map.get(attributeName);
    if (!vector)
        return false;

    PropertiesVector::iterator vectorEnd = vector->end();
    for (PropertiesVector::iterator vectorIt = vector->begin(); vectorIt != vectorEnd; ++vectorIt)
        synchronizeProperty(contextElement, attributeName, *vectorIt);

    return true;
}

void SVGAttributeToPropertyMap::synchronizeProperty(SVGElement* contextElement, const QualifiedName& attributeName, const SVGPropertyInfo* info)
{
    ASSERT(info);
    ASSERT_UNUSED(attributeName, attributeName == info->attributeName);
    ASSERT(info->synchronizeProperty);
    (*info->synchronizeProperty)(contextElement);
}

PassRefPtr<SVGAnimatedProperty> SVGAttributeToPropertyMap::animatedProperty(SVGElement* contextElement, const QualifiedName& attributeName, const SVGPropertyInfo* info)
{
    ASSERT(info);
    ASSERT_UNUSED(attributeName, attributeName == info->attributeName);
    ASSERT(info->lookupOrCreateWrapperForAnimatedProperty);
    return (*info->lookupOrCreateWrapperForAnimatedProperty)(contextElement);
}

}

#endif
