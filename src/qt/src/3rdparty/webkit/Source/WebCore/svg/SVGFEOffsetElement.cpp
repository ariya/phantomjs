/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
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
#include "SVGFEOffsetElement.h"

#include "Attribute.h"
#include "FilterEffect.h"
#include "SVGFilterBuilder.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGFEOffsetElement, SVGNames::inAttr, In1, in1)
DEFINE_ANIMATED_NUMBER(SVGFEOffsetElement, SVGNames::dxAttr, Dx, dx)
DEFINE_ANIMATED_NUMBER(SVGFEOffsetElement, SVGNames::dyAttr, Dy, dy)

inline SVGFEOffsetElement::SVGFEOffsetElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
{
}

PassRefPtr<SVGFEOffsetElement> SVGFEOffsetElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFEOffsetElement(tagName, document));
}

void SVGFEOffsetElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::dxAttr)
        setDxBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::dyAttr)
        setDyBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::inAttr)
        setIn1BaseValue(value);
    else
        SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(attr);
}

void SVGFEOffsetElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);

    if (attrName == SVGNames::inAttr
        || attrName == SVGNames::dxAttr
        || attrName == SVGNames::dyAttr)
        invalidate();
}

void SVGFEOffsetElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeDx();
        synchronizeDy();
        synchronizeIn1();
        return;
    }

    if (attrName == SVGNames::dxAttr)
        synchronizeDx();
    else if (attrName == SVGNames::dyAttr)
        synchronizeDy();
    else if (attrName == SVGNames::inAttr)
        synchronizeIn1();
}

AttributeToPropertyTypeMap& SVGFEOffsetElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGFEOffsetElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGFilterPrimitiveStandardAttributes::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::inAttr, AnimatedString);
    attributeToPropertyTypeMap.set(SVGNames::dxAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::dyAttr, AnimatedNumber);
}

PassRefPtr<FilterEffect> SVGFEOffsetElement::build(SVGFilterBuilder* filterBuilder, Filter* filter)
{
    FilterEffect* input1 = filterBuilder->getEffectById(in1());

    if (!input1)
        return 0;

    RefPtr<FilterEffect> effect = FEOffset::create(filter, dx(), dy());
    effect->inputEffects().append(input1);
    return effect.release();
}

}

#endif // ENABLE(SVG)
