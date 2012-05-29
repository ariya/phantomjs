/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#ifndef SVGFEBlendElement_h
#define SVGFEBlendElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FEBlend.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {

class SVGFEBlendElement : public SVGFilterPrimitiveStandardAttributes {
public:
    static PassRefPtr<SVGFEBlendElement> create(const QualifiedName&, Document*);

private:
    SVGFEBlendElement(const QualifiedName&, Document*);

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
    DECLARE_ANIMATED_ENUMERATION(Mode, mode)
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
