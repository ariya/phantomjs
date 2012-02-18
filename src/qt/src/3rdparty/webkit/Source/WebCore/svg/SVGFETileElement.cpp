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
#include "SVGFETileElement.h"

#include "Attribute.h"
#include "FilterEffect.h"
#include "SVGFilterBuilder.h"
#include "SVGNames.h"
#include "SVGRenderStyle.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGFETileElement, SVGNames::inAttr, In1, in1)

inline SVGFETileElement::SVGFETileElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
{
}

PassRefPtr<SVGFETileElement> SVGFETileElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFETileElement(tagName, document));
}

void SVGFETileElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::inAttr)
        setIn1BaseValue(value);
    else
        SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(attr);
}

void SVGFETileElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);
    
    if (attrName == SVGNames::inAttr)
        invalidate();
}

void SVGFETileElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::synchronizeProperty(attrName);

    if (attrName == anyQName() || attrName == SVGNames::inAttr)
        synchronizeIn1();
}

AttributeToPropertyTypeMap& SVGFETileElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGFETileElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGFilterPrimitiveStandardAttributes::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::inAttr, AnimatedString);
}

PassRefPtr<FilterEffect> SVGFETileElement::build(SVGFilterBuilder* filterBuilder, Filter* filter)
{
    FilterEffect* input1 = filterBuilder->getEffectById(in1());

    if (!input1)
        return 0;

    RefPtr<FilterEffect> effect = FETile::create(filter);
    effect->inputEffects().append(input1);
    return effect.release();
}

}

#endif // ENABLE(SVG)
