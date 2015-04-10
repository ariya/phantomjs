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

#ifndef SVGFETurbulenceElement_h
#define SVGFETurbulenceElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FETurbulence.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGAnimatedInteger.h"
#include "SVGAnimatedNumber.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {

enum SVGStitchOptions {
    SVG_STITCHTYPE_UNKNOWN  = 0,
    SVG_STITCHTYPE_STITCH   = 1,
    SVG_STITCHTYPE_NOSTITCH = 2
};

template<>
struct SVGPropertyTraits<SVGStitchOptions> {
    static unsigned highestEnumValue() { return SVG_STITCHTYPE_NOSTITCH; }

    static String toString(SVGStitchOptions type)
    {
        switch (type) {
        case SVG_STITCHTYPE_UNKNOWN:
            return emptyString();
        case SVG_STITCHTYPE_STITCH:
            return "stitch";
        case SVG_STITCHTYPE_NOSTITCH:
            return "noStitch";
        }

        ASSERT_NOT_REACHED();
        return emptyString();
    }

    static SVGStitchOptions fromString(const String& value)
    {
        if (value == "stitch")
            return SVG_STITCHTYPE_STITCH;
        if (value == "noStitch")
            return SVG_STITCHTYPE_NOSTITCH;
        return SVG_STITCHTYPE_UNKNOWN;
    }
};

template<>
struct SVGPropertyTraits<TurbulenceType> {
    static unsigned highestEnumValue() { return FETURBULENCE_TYPE_TURBULENCE; }

    static String toString(TurbulenceType type)
    {
        switch (type) {
        case FETURBULENCE_TYPE_UNKNOWN:
            return emptyString();
        case FETURBULENCE_TYPE_FRACTALNOISE:
            return "fractalNoise";
        case FETURBULENCE_TYPE_TURBULENCE:
            return "turbulence";
        }

        ASSERT_NOT_REACHED();
        return emptyString();
    }

    static TurbulenceType fromString(const String& value)
    {
        if (value == "fractalNoise")
            return FETURBULENCE_TYPE_FRACTALNOISE;
        if (value == "turbulence")
            return FETURBULENCE_TYPE_TURBULENCE;
        return FETURBULENCE_TYPE_UNKNOWN;
    }
};

class SVGFETurbulenceElement FINAL : public SVGFilterPrimitiveStandardAttributes {
public:
    static PassRefPtr<SVGFETurbulenceElement> create(const QualifiedName&, Document*);

private:
    SVGFETurbulenceElement(const QualifiedName&, Document*);

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual bool setFilterEffectAttribute(FilterEffect*, const QualifiedName& attrName);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual PassRefPtr<FilterEffect> build(SVGFilterBuilder*, Filter*);

    static const AtomicString& baseFrequencyXIdentifier();
    static const AtomicString& baseFrequencyYIdentifier();

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGFETurbulenceElement)
        DECLARE_ANIMATED_NUMBER(BaseFrequencyX, baseFrequencyX)
        DECLARE_ANIMATED_NUMBER(BaseFrequencyY, baseFrequencyY)
        DECLARE_ANIMATED_INTEGER(NumOctaves, numOctaves)
        DECLARE_ANIMATED_NUMBER(Seed, seed)
        DECLARE_ANIMATED_ENUMERATION(StitchTiles, stitchTiles, SVGStitchOptions)
        DECLARE_ANIMATED_ENUMERATION(Type, type, TurbulenceType)
    END_DECLARE_ANIMATED_PROPERTIES
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
