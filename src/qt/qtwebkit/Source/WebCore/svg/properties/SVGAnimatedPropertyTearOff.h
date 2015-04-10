/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#ifndef SVGAnimatedPropertyTearOff_h
#define SVGAnimatedPropertyTearOff_h

#if ENABLE(SVG)
#include "SVGAnimatedProperty.h"
#include "SVGPropertyTearOff.h"

namespace WebCore {

template<typename PropertyType>
class SVGAnimatedPropertyTearOff : public SVGAnimatedProperty {
public:
    typedef SVGPropertyTearOff<PropertyType> PropertyTearOff;
    typedef PropertyType ContentType;

    virtual ~SVGAnimatedPropertyTearOff()
    {
        if (m_baseVal) {
            ASSERT(m_baseVal->animatedProperty() == this);
            m_baseVal->setAnimatedProperty(0);
        }
        if (m_animVal) {
            ASSERT(m_animVal->animatedProperty() == this);
            m_animVal->setAnimatedProperty(0);
        }
    }

    PropertyTearOff* baseVal()
    {
        if (!m_baseVal)
            m_baseVal = PropertyTearOff::create(this, BaseValRole, m_property);
        return m_baseVal.get();
    }

    PropertyTearOff* animVal()
    {
        if (!m_animVal)
            m_animVal = PropertyTearOff::create(this, AnimValRole, m_property);
        return m_animVal.get();
    }

    static PassRefPtr<SVGAnimatedPropertyTearOff<PropertyType> > create(SVGElement* contextElement, const QualifiedName& attributeName, AnimatedPropertyType animatedPropertyType, PropertyType& property)
    {
        ASSERT(contextElement);
        return adoptRef(new SVGAnimatedPropertyTearOff<PropertyType>(contextElement, attributeName, animatedPropertyType, property));
    }

    PropertyType& currentAnimatedValue()
    {
        ASSERT(m_isAnimating);
        ASSERT(m_animVal);
        return m_animVal->propertyReference();
    }

    const PropertyType& currentBaseValue() const
    {
        return m_property;
    }

    void animationStarted(PropertyType* newAnimVal)
    {
        ASSERT(!m_isAnimating);
        ASSERT(newAnimVal);
        animVal()->setValue(*newAnimVal);
        m_isAnimating = true;
    }

    void animationEnded()
    {
        ASSERT(m_isAnimating);
        ASSERT(m_animVal);
        m_animVal->setValue(m_property);
        m_isAnimating = false;
    }

    void animValWillChange()
    {
        // no-op for non list types.
        ASSERT(m_isAnimating);
        ASSERT(m_animVal);
    }

    void animValDidChange()
    {
        // no-op for non list types.
        ASSERT(m_isAnimating);
        ASSERT(m_animVal);
    }

private:
    SVGAnimatedPropertyTearOff(SVGElement* contextElement, const QualifiedName& attributeName, AnimatedPropertyType animatedPropertyType, PropertyType& property)
        : SVGAnimatedProperty(contextElement, attributeName, animatedPropertyType)
        , m_property(property)
    {
    }

    PropertyType& m_property;
    RefPtr<PropertyTearOff> m_baseVal;
    RefPtr<PropertyTearOff> m_animVal;
};

}

#endif // ENABLE(SVG)
#endif // SVGAnimatedPropertyTearOff_h
