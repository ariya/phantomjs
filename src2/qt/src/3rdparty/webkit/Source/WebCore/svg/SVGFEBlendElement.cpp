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

#include "config.h"

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFEBlendElement.h"

#include "Attribute.h"
#include "FilterEffect.h"
#include "SVGFilterBuilder.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGFEBlendElement, SVGNames::inAttr, In1, in1)
DEFINE_ANIMATED_STRING(SVGFEBlendElement, SVGNames::in2Attr, In2, in2)
DEFINE_ANIMATED_ENUMERATION(SVGFEBlendElement, SVGNames::modeAttr, Mode, mode)

inline SVGFEBlendElement::SVGFEBlendElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
    , m_mode(FEBLEND_MODE_NORMAL)
{
}

PassRefPtr<SVGFEBlendElement> SVGFEBlendElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFEBlendElement(tagName, document));
}

void SVGFEBlendElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::modeAttr) {
        if (value == "normal")
            setModeBaseValue(FEBLEND_MODE_NORMAL);
        else if (value == "multiply")
            setModeBaseValue(FEBLEND_MODE_MULTIPLY);
        else if (value == "screen")
            setModeBaseValue(FEBLEND_MODE_SCREEN);
        else if (value == "darken")
            setModeBaseValue(FEBLEND_MODE_DARKEN);
        else if (value == "lighten")
            setModeBaseValue(FEBLEND_MODE_LIGHTEN);
    } else if (attr->name() == SVGNames::inAttr)
        setIn1BaseValue(value);
    else if (attr->name() == SVGNames::in2Attr)
        setIn2BaseValue(value);
    else
        SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(attr);
}

bool SVGFEBlendElement::setFilterEffectAttribute(FilterEffect* effect, const QualifiedName& attrName)
{
    FEBlend* blend = static_cast<FEBlend*>(effect);
    if (attrName == SVGNames::modeAttr)
        return blend->setBlendMode(static_cast<BlendModeType>(mode()));

    ASSERT_NOT_REACHED();
    return false;
}

void SVGFEBlendElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);

    if (attrName == SVGNames::modeAttr)
        primitiveAttributeChanged(attrName);

    if (attrName == SVGNames::inAttr
        || attrName == SVGNames::in2Attr)
        invalidate();
}

void SVGFEBlendElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeMode();
        synchronizeIn1();
        synchronizeIn2();
        return;
    }

    if (attrName == SVGNames::modeAttr)
        synchronizeMode();
    else if (attrName == SVGNames::inAttr)
        synchronizeIn1();
    else if (attrName == SVGNames::in2Attr)
        synchronizeIn2();
}

AttributeToPropertyTypeMap& SVGFEBlendElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGFEBlendElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGFilterPrimitiveStandardAttributes::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);    
    attributeToPropertyTypeMap.set(SVGNames::inAttr, AnimatedString);
    attributeToPropertyTypeMap.set(SVGNames::in2Attr, AnimatedString);
    attributeToPropertyTypeMap.set(SVGNames::modeAttr, AnimatedEnumeration);
}

PassRefPtr<FilterEffect> SVGFEBlendElement::build(SVGFilterBuilder* filterBuilder, Filter* filter)
{
    FilterEffect* input1 = filterBuilder->getEffectById(in1());
    FilterEffect* input2 = filterBuilder->getEffectById(in2());

    if (!input1 || !input2)
        return 0;

    RefPtr<FilterEffect> effect = FEBlend::create(filter, static_cast<BlendModeType>(mode()));
    FilterEffectVector& inputEffects = effect->inputEffects();
    inputEffects.reserveCapacity(2);
    inputEffects.append(input1);
    inputEffects.append(input2);    
    return effect.release();
}

}

#endif // ENABLE(SVG)

// vim:ts=4:noet
