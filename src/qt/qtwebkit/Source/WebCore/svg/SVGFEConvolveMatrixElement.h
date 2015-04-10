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

#ifndef SVGFEConvolveMatrixElement_h
#define SVGFEConvolveMatrixElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FEConvolveMatrix.h"
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGAnimatedInteger.h"
#include "SVGAnimatedNumber.h"
#include "SVGAnimatedNumberList.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {

template<>
struct SVGPropertyTraits<EdgeModeType> {
    static unsigned highestEnumValue() { return EDGEMODE_NONE; }

    static String toString(EdgeModeType type)
    {
        switch (type) {
        case EDGEMODE_UNKNOWN:
            return emptyString();
        case EDGEMODE_DUPLICATE:
            return "duplicate";
        case EDGEMODE_WRAP:
            return "wrap";
        case EDGEMODE_NONE:
            return "none";
        }

        ASSERT_NOT_REACHED();
        return emptyString();
    }

    static EdgeModeType fromString(const String& value)
    {
        if (value == "duplicate")
            return EDGEMODE_DUPLICATE;
        if (value == "wrap")
            return EDGEMODE_WRAP;
        if (value == "none")
            return EDGEMODE_NONE;
        return EDGEMODE_UNKNOWN;
    }
};

class SVGFEConvolveMatrixElement FINAL : public SVGFilterPrimitiveStandardAttributes {
public:
    static PassRefPtr<SVGFEConvolveMatrixElement> create(const QualifiedName&, Document*);

    void setOrder(float orderX, float orderY);
    void setKernelUnitLength(float kernelUnitLengthX, float kernelUnitLengthY);

private:
    SVGFEConvolveMatrixElement(const QualifiedName&, Document*);

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual bool setFilterEffectAttribute(FilterEffect*, const QualifiedName&);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual PassRefPtr<FilterEffect> build(SVGFilterBuilder*, Filter*);

    static const AtomicString& orderXIdentifier();
    static const AtomicString& orderYIdentifier();
    static const AtomicString& kernelUnitLengthXIdentifier();
    static const AtomicString& kernelUnitLengthYIdentifier();

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGFEConvolveMatrixElement)
        DECLARE_ANIMATED_STRING(In1, in1)
        DECLARE_ANIMATED_INTEGER(OrderX, orderX)
        DECLARE_ANIMATED_INTEGER(OrderY, orderY)
        DECLARE_ANIMATED_NUMBER_LIST(KernelMatrix, kernelMatrix)
        DECLARE_ANIMATED_NUMBER(Divisor, divisor)
        DECLARE_ANIMATED_NUMBER(Bias, bias)
        DECLARE_ANIMATED_INTEGER(TargetX, targetX)
        DECLARE_ANIMATED_INTEGER(TargetY, targetY)
        DECLARE_ANIMATED_ENUMERATION(EdgeMode, edgeMode, EdgeModeType)
        DECLARE_ANIMATED_NUMBER(KernelUnitLengthX, kernelUnitLengthX)
        DECLARE_ANIMATED_NUMBER(KernelUnitLengthY, kernelUnitLengthY)
        DECLARE_ANIMATED_BOOLEAN(PreserveAlpha, preserveAlpha)
    END_DECLARE_ANIMATED_PROPERTIES
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
