/*
 * Copyright (C) 2006 Oliver Hunt <oliver@nerget.com>
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

#ifndef SVGFEDisplacementMapElement_h
#define SVGFEDisplacementMapElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FEDisplacementMap.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGAnimatedNumber.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {
    
class SVGFEDisplacementMapElement : public SVGFilterPrimitiveStandardAttributes {
public:
    static PassRefPtr<SVGFEDisplacementMapElement> create(const QualifiedName&, Document*);

    static ChannelSelectorType stringToChannel(const String&);
    
private:
    SVGFEDisplacementMapElement(const QualifiedName& tagName, Document*);
    
    virtual void parseMappedAttribute(Attribute*);
    virtual bool setFilterEffectAttribute(FilterEffect*, const QualifiedName& attrName);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void synchronizeProperty(const QualifiedName&);
    virtual void fillAttributeToPropertyTypeMap();
    virtual AttributeToPropertyTypeMap& attributeToPropertyTypeMap();
    virtual PassRefPtr<FilterEffect> build(SVGFilterBuilder*, Filter*);

    // Animated property declarations
    DECLARE_ANIMATED_STRING(In1, in1)
    DECLARE_ANIMATED_STRING(In2, in2)
    DECLARE_ANIMATED_ENUMERATION(XChannelSelector, xChannelSelector)
    DECLARE_ANIMATED_ENUMERATION(YChannelSelector, yChannelSelector)
    DECLARE_ANIMATED_NUMBER(Scale, scale)
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGFEDisplacementMapElement_h
