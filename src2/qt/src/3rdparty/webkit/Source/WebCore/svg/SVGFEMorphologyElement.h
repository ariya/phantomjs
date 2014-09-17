/*
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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

#ifndef SVGFEMorphologyElement_h
#define SVGFEMorphologyElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FEMorphology.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGAnimatedNumber.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {

class SVGFEMorphologyElement : public SVGFilterPrimitiveStandardAttributes {
public:
    static PassRefPtr<SVGFEMorphologyElement> create(const QualifiedName&, Document*);

    void setRadius(float radiusX, float radiusY);

private:
    SVGFEMorphologyElement(const QualifiedName&, Document*);

    virtual void parseMappedAttribute(Attribute*);
    virtual bool setFilterEffectAttribute(FilterEffect*, const QualifiedName&);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void synchronizeProperty(const QualifiedName&);
    virtual void fillAttributeToPropertyTypeMap();
    virtual AttributeToPropertyTypeMap& attributeToPropertyTypeMap();
    virtual PassRefPtr<FilterEffect> build(SVGFilterBuilder*, Filter*);

    static const AtomicString& radiusXIdentifier();
    static const AtomicString& radiusYIdentifier();

    // Animated property declarations
    DECLARE_ANIMATED_STRING(In1, in1)
    DECLARE_ANIMATED_ENUMERATION(_operator, _operator)
    DECLARE_ANIMATED_NUMBER(RadiusX, radiusX)
    DECLARE_ANIMATED_NUMBER(RadiusY, radiusY)
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
