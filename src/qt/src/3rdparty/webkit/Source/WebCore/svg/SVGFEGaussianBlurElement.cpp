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
#include "SVGFEGaussianBlurElement.h"

#include "Attribute.h"
#include "FilterEffect.h"
#include "SVGFilterBuilder.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGFEGaussianBlurElement, SVGNames::inAttr, In1, in1)
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFEGaussianBlurElement, SVGNames::stdDeviationAttr, stdDeviationXIdentifier(), StdDeviationX, stdDeviationX)
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFEGaussianBlurElement, SVGNames::stdDeviationAttr, stdDeviationYIdentifier(), StdDeviationY, stdDeviationY)

inline SVGFEGaussianBlurElement::SVGFEGaussianBlurElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
{
}

PassRefPtr<SVGFEGaussianBlurElement> SVGFEGaussianBlurElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFEGaussianBlurElement(tagName, document));
}

const AtomicString& SVGFEGaussianBlurElement::stdDeviationXIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGStdDeviationX"));
    return s_identifier;
}

const AtomicString& SVGFEGaussianBlurElement::stdDeviationYIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGStdDeviationY"));
    return s_identifier;
}

void SVGFEGaussianBlurElement::setStdDeviation(float x, float y)
{
    setStdDeviationXBaseValue(x);
    setStdDeviationYBaseValue(y);
    invalidate();
}

void SVGFEGaussianBlurElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::stdDeviationAttr) {
        float x, y;
        if (parseNumberOptionalNumber(value, x, y)) {
            setStdDeviationXBaseValue(x);
            setStdDeviationYBaseValue(y);
        }
    } else if (attr->name() == SVGNames::inAttr)
        setIn1BaseValue(value);
    else
        SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(attr);
}

void SVGFEGaussianBlurElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);

    if (attrName == SVGNames::inAttr
        || attrName == SVGNames::stdDeviationAttr)
        invalidate();
}

void SVGFEGaussianBlurElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeStdDeviationX();
        synchronizeStdDeviationY();
        synchronizeIn1();
        return;
    }

    if (attrName == SVGNames::stdDeviationAttr) {
        synchronizeStdDeviationX();
        synchronizeStdDeviationY();
    } else if (attrName == SVGNames::inAttr)
        synchronizeIn1();
}

AttributeToPropertyTypeMap& SVGFEGaussianBlurElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGFEGaussianBlurElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGFilterPrimitiveStandardAttributes::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::inAttr, AnimatedString);
    attributeToPropertyTypeMap.set(SVGNames::stdDeviationAttr, AnimatedNumberOptionalNumber);
}

PassRefPtr<FilterEffect> SVGFEGaussianBlurElement::build(SVGFilterBuilder* filterBuilder, Filter* filter)
{
    FilterEffect* input1 = filterBuilder->getEffectById(in1());

    if (!input1)
        return 0;

    RefPtr<FilterEffect> effect = FEGaussianBlur::create(filter, stdDeviationX(), stdDeviationY());
    effect->inputEffects().append(input1);
    return effect.release();
}

}

#endif // ENABLE(SVG)
