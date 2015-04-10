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

#ifndef SVGFECompositeElement_h
#define SVGFECompositeElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FEComposite.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGAnimatedNumber.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {

template<>
struct SVGPropertyTraits<CompositeOperationType> {
    static unsigned highestEnumValue() { return FECOMPOSITE_OPERATOR_ARITHMETIC; }

    static String toString(CompositeOperationType type)
    {
        switch (type) {
        case FECOMPOSITE_OPERATOR_UNKNOWN:
            return emptyString();
        case FECOMPOSITE_OPERATOR_OVER:
            return "over";
        case FECOMPOSITE_OPERATOR_IN:
            return "in";
        case FECOMPOSITE_OPERATOR_OUT:
            return "out";
        case FECOMPOSITE_OPERATOR_ATOP:
            return "atop";
        case FECOMPOSITE_OPERATOR_XOR:
            return "xor";
        case FECOMPOSITE_OPERATOR_ARITHMETIC:
            return "arithmetic";
        }

        ASSERT_NOT_REACHED();
        return emptyString();
    }

    static CompositeOperationType fromString(const String& value)
    {
        if (value == "over")
            return FECOMPOSITE_OPERATOR_OVER;
        if (value == "in")
            return FECOMPOSITE_OPERATOR_IN;
        if (value == "out")
            return FECOMPOSITE_OPERATOR_OUT;
        if (value == "atop")
            return FECOMPOSITE_OPERATOR_ATOP;
        if (value == "xor")
            return FECOMPOSITE_OPERATOR_XOR;
        if (value == "arithmetic")
            return FECOMPOSITE_OPERATOR_ARITHMETIC;
        return FECOMPOSITE_OPERATOR_UNKNOWN;
    }
};

class SVGFECompositeElement FINAL : public SVGFilterPrimitiveStandardAttributes {
public:
    static PassRefPtr<SVGFECompositeElement> create(const QualifiedName&, Document*);

private:
    SVGFECompositeElement(const QualifiedName&, Document*);

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual bool setFilterEffectAttribute(FilterEffect*, const QualifiedName&);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual PassRefPtr<FilterEffect> build(SVGFilterBuilder*, Filter*);

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGFECompositeElement)
        DECLARE_ANIMATED_STRING(In1, in1)
        DECLARE_ANIMATED_STRING(In2, in2)
        DECLARE_ANIMATED_ENUMERATION(SVGOperator, svgOperator, CompositeOperationType)
        DECLARE_ANIMATED_NUMBER(K1, k1)
        DECLARE_ANIMATED_NUMBER(K2, k2)
        DECLARE_ANIMATED_NUMBER(K3, k3)
        DECLARE_ANIMATED_NUMBER(K4, k4)
    END_DECLARE_ANIMATED_PROPERTIES
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
