/*
 * Copyright (C) Research In Motion Limited 2010-2012. All rights reserved.
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

#ifndef SVGAnimatedStaticPropertyTearOff_h
#define SVGAnimatedStaticPropertyTearOff_h

#if ENABLE(SVG)
#include "ExceptionCode.h"
#include "SVGAnimatedProperty.h"

namespace WebCore {

template<typename PropertyType>
class SVGAnimatedStaticPropertyTearOff : public SVGAnimatedProperty {
public:
    typedef PropertyType ContentType;

    PropertyType& baseVal()
    {
        return m_property;
    }

    PropertyType& animVal()
    {
        if (m_animatedProperty)
            return *m_animatedProperty;
        return m_property;
    }

    virtual void setBaseVal(const PropertyType& property, ExceptionCode&)
    {
        m_property = property;
        commitChange();
    }

    static PassRefPtr<SVGAnimatedStaticPropertyTearOff<PropertyType> > create(SVGElement* contextElement, const QualifiedName& attributeName, AnimatedPropertyType animatedPropertyType, PropertyType& property)
    {
        ASSERT(contextElement);
        return adoptRef(new SVGAnimatedStaticPropertyTearOff<PropertyType>(contextElement, attributeName, animatedPropertyType, property));
    }

    PropertyType& currentAnimatedValue()
    {
        ASSERT(m_isAnimating);
        ASSERT(m_animatedProperty);
        return *m_animatedProperty;
    }

    const PropertyType& currentBaseValue() const
    {
        return m_property;
    }

    void animationStarted(PropertyType* newAnimVal)
    {
        ASSERT(!m_isAnimating);
        ASSERT(!m_animatedProperty);
        ASSERT(newAnimVal);
        m_animatedProperty = newAnimVal;
        m_isAnimating = true;
    }

    void animationEnded()
    {
        ASSERT(m_isAnimating);
        ASSERT(m_animatedProperty);
        m_animatedProperty = 0;
        m_isAnimating = false;
    }

    void animValWillChange()
    {
        // no-op for non list types.
        ASSERT(m_isAnimating);
        ASSERT(m_animatedProperty);
    }

    void animValDidChange()
    {
        // no-op for non list types.
        ASSERT(m_isAnimating);
        ASSERT(m_animatedProperty);
    }

protected:
    SVGAnimatedStaticPropertyTearOff(SVGElement* contextElement, const QualifiedName& attributeName, AnimatedPropertyType animatedPropertyType, PropertyType& property)
        : SVGAnimatedProperty(contextElement, attributeName, animatedPropertyType)
        , m_property(property)
        , m_animatedProperty(0)
    {
    }

private:
    PropertyType& m_property;
    PropertyType* m_animatedProperty;
};

}

#endif // ENABLE(SVG)
#endif // SVGAnimatedStaticPropertyTearOff_h
