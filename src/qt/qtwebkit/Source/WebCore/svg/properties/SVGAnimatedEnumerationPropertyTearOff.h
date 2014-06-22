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

#ifndef SVGAnimatedEnumerationPropertyTearOff_h
#define SVGAnimatedEnumerationPropertyTearOff_h

#if ENABLE(SVG)
#include "SVGAnimatedStaticPropertyTearOff.h"
#include "SVGException.h"
#include "SVGPropertyTraits.h"

namespace WebCore {

template<typename EnumType>
class SVGAnimatedEnumerationPropertyTearOff : public SVGAnimatedStaticPropertyTearOff<unsigned> {
public:
    virtual void setBaseVal(const unsigned& property, ExceptionCode& ec)
    {
        // All SVG enumeration values, that are allowed to be set via SVG DOM start with 1, 0 corresponds to unknown and is not settable through SVG DOM.
        if (!property || property > SVGPropertyTraits<EnumType>::highestEnumValue()) {
            ec = SVGException::SVG_INVALID_VALUE_ERR;
            return;
        }
        SVGAnimatedStaticPropertyTearOff<unsigned>::setBaseVal(property, ec);
    }

    static PassRefPtr<SVGAnimatedEnumerationPropertyTearOff<EnumType> > create(SVGElement* contextElement, const QualifiedName& attributeName, AnimatedPropertyType animatedPropertyType, EnumType& property)
    {
        ASSERT(contextElement);
        return adoptRef(new SVGAnimatedEnumerationPropertyTearOff<EnumType>(contextElement, attributeName, animatedPropertyType, reinterpret_cast<unsigned&>(property)));
    }

    EnumType& currentAnimatedValue()
    {
        unsigned& animatedValue = SVGAnimatedStaticPropertyTearOff<unsigned>::currentAnimatedValue();
        ASSERT(animatedValue <= SVGPropertyTraits<EnumType>::highestEnumValue());
        return reinterpret_cast<EnumType&>(animatedValue);
    }

private:
    SVGAnimatedEnumerationPropertyTearOff(SVGElement* contextElement, const QualifiedName& attributeName, AnimatedPropertyType animatedPropertyType, unsigned& property)
        : SVGAnimatedStaticPropertyTearOff<unsigned>(contextElement, attributeName, animatedPropertyType, property)
    {
    }
};

}

#endif // ENABLE(SVG)
#endif // SVGAnimatedEnumerationPropertyTearOff_h
