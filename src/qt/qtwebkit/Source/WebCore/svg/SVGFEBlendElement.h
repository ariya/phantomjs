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

template<>
struct SVGPropertyTraits<BlendModeType> {
    static unsigned highestEnumValue() { return FEBLEND_MODE_LIGHTEN; }

    static String toString(BlendModeType type)
    {
        switch (type) {
        case FEBLEND_MODE_UNKNOWN:
            return emptyString();
        case FEBLEND_MODE_NORMAL:
            return "normal";
        case FEBLEND_MODE_MULTIPLY:
            return "multiply";
        case FEBLEND_MODE_SCREEN:
            return "screen";
        case FEBLEND_MODE_DARKEN:
            return "darken";
        case FEBLEND_MODE_LIGHTEN:
            return "lighten";
        }

        ASSERT_NOT_REACHED();
        return emptyString();
    }

    static BlendModeType fromString(const String& value)
    {
        if (value == "normal")
            return FEBLEND_MODE_NORMAL;
        if (value == "multiply")
            return FEBLEND_MODE_MULTIPLY;
        if (value == "screen")
            return FEBLEND_MODE_SCREEN;
        if (value == "darken")
            return FEBLEND_MODE_DARKEN;
        if (value == "lighten")
            return FEBLEND_MODE_LIGHTEN;
        return FEBLEND_MODE_UNKNOWN;
    }
};

class SVGFEBlendElement FINAL : public SVGFilterPrimitiveStandardAttributes {
public:
    static PassRefPtr<SVGFEBlendElement> create(const QualifiedName&, Document*);

private:
    SVGFEBlendElement(const QualifiedName&, Document*);

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual bool setFilterEffectAttribute(FilterEffect*, const QualifiedName& attrName);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual PassRefPtr<FilterEffect> build(SVGFilterBuilder*, Filter*);

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGFEBlendElement)
        DECLARE_ANIMATED_STRING(In1, in1)
        DECLARE_ANIMATED_STRING(In2, in2)
        DECLARE_ANIMATED_ENUMERATION(Mode, mode, BlendModeType)
    END_DECLARE_ANIMATED_PROPERTIES
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
