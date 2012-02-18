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
#include "SVGFEComponentTransferElement.h"

#include "Attr.h"
#include "FilterEffect.h"
#include "SVGFEFuncAElement.h"
#include "SVGFEFuncBElement.h"
#include "SVGFEFuncGElement.h"
#include "SVGFEFuncRElement.h"
#include "SVGFilterBuilder.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property declarations
DEFINE_ANIMATED_STRING(SVGFEComponentTransferElement, SVGNames::inAttr, In1, in1)

inline SVGFEComponentTransferElement::SVGFEComponentTransferElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
{
}

PassRefPtr<SVGFEComponentTransferElement> SVGFEComponentTransferElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFEComponentTransferElement(tagName, document));
}

void SVGFEComponentTransferElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::inAttr)
        setIn1BaseValue(value);
    else
        SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(attr);
}

void SVGFEComponentTransferElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::synchronizeProperty(attrName);

    if (attrName == anyQName() || attrName == SVGNames::inAttr)
        synchronizeIn1();
}

AttributeToPropertyTypeMap& SVGFEComponentTransferElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGFEComponentTransferElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGFilterPrimitiveStandardAttributes::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::inAttr, AnimatedString);
}

PassRefPtr<FilterEffect> SVGFEComponentTransferElement::build(SVGFilterBuilder* filterBuilder, Filter* filter)
{
    FilterEffect* input1 = filterBuilder->getEffectById(in1());
    
    if (!input1)
        return 0;

    ComponentTransferFunction red;
    ComponentTransferFunction green;
    ComponentTransferFunction blue;
    ComponentTransferFunction alpha;
    
    for (Node* node = firstChild(); node; node = node->nextSibling()) {
        if (node->hasTagName(SVGNames::feFuncRTag))
            red = static_cast<SVGFEFuncRElement*>(node)->transferFunction();
        else if (node->hasTagName(SVGNames::feFuncGTag))
            green = static_cast<SVGFEFuncGElement*>(node)->transferFunction();
        else if (node->hasTagName(SVGNames::feFuncBTag))
           blue = static_cast<SVGFEFuncBElement*>(node)->transferFunction();
        else if (node->hasTagName(SVGNames::feFuncATag))
            alpha = static_cast<SVGFEFuncAElement*>(node)->transferFunction();
    }
    
    RefPtr<FilterEffect> effect = FEComponentTransfer::create(filter, red, green, blue, alpha);
    effect->inputEffects().append(input1);
    return effect.release();
}

}

#endif
