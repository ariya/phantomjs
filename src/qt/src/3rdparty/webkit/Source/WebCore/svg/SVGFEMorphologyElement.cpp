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

#include "config.h"

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFEMorphologyElement.h"

#include "Attribute.h"
#include "FilterEffect.h"
#include "SVGFilterBuilder.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGFEMorphologyElement, SVGNames::inAttr, In1, in1)
DEFINE_ANIMATED_ENUMERATION(SVGFEMorphologyElement, SVGNames::operatorAttr, _operator, _operator)
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFEMorphologyElement, SVGNames::radiusAttr, radiusXIdentifier(), RadiusX, radiusX)
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFEMorphologyElement, SVGNames::radiusAttr, radiusYIdentifier(), RadiusY, radiusY)

inline SVGFEMorphologyElement::SVGFEMorphologyElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
    , m__operator(FEMORPHOLOGY_OPERATOR_ERODE)
{
}

PassRefPtr<SVGFEMorphologyElement> SVGFEMorphologyElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFEMorphologyElement(tagName, document));
}

const AtomicString& SVGFEMorphologyElement::radiusXIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGRadiusX"));
    return s_identifier;
}

const AtomicString& SVGFEMorphologyElement::radiusYIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGRadiusY"));
    return s_identifier;
}

void SVGFEMorphologyElement::setRadius(float x, float y)
{
    setRadiusXBaseValue(x);
    setRadiusYBaseValue(y);
    invalidate();
}

void SVGFEMorphologyElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::operatorAttr) {
        if (value == "erode")
            set_operatorBaseValue(FEMORPHOLOGY_OPERATOR_ERODE);
        else if (value == "dilate")
            set_operatorBaseValue(FEMORPHOLOGY_OPERATOR_DILATE);
    } else if (attr->name() == SVGNames::inAttr)
        setIn1BaseValue(value);
    else if (attr->name() == SVGNames::radiusAttr) {
        float x, y;
        if (parseNumberOptionalNumber(value, x, y)) {
            setRadiusXBaseValue(x);
            setRadiusYBaseValue(y);
        }
    } else
        SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(attr);
}

bool SVGFEMorphologyElement::setFilterEffectAttribute(FilterEffect* effect, const QualifiedName& attrName)
{
    FEMorphology* morphology = static_cast<FEMorphology*>(effect);
    if (attrName == SVGNames::operatorAttr)
        return morphology->setMorphologyOperator(static_cast<MorphologyOperatorType>(_operator()));
    if (attrName == SVGNames::radiusAttr)
        return (morphology->setRadiusX(radiusX()) || morphology->setRadiusY(radiusY()));

    ASSERT_NOT_REACHED();
    return false;
}

void SVGFEMorphologyElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);

    if (attrName == SVGNames::operatorAttr
        || attrName == SVGNames::radiusAttr)
        primitiveAttributeChanged(attrName);

    if (attrName == SVGNames::inAttr)
        invalidate();
}

void SVGFEMorphologyElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronize_operator();
        synchronizeIn1();
        synchronizeRadiusX();
        synchronizeRadiusY();
        return;
    }

    if (attrName == SVGNames::operatorAttr)
        synchronize_operator();
    else if (attrName == SVGNames::inAttr)
        synchronizeIn1();
    else if (attrName == SVGNames::radiusAttr) {
        synchronizeRadiusX();
        synchronizeRadiusY();
    }
}

AttributeToPropertyTypeMap& SVGFEMorphologyElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGFEMorphologyElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGFilterPrimitiveStandardAttributes::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::inAttr, AnimatedString);
    attributeToPropertyTypeMap.set(SVGNames::operatorAttr, AnimatedEnumeration);
    attributeToPropertyTypeMap.set(SVGNames::radiusAttr, AnimatedNumberOptionalNumber);
}

PassRefPtr<FilterEffect> SVGFEMorphologyElement::build(SVGFilterBuilder* filterBuilder, Filter* filter)
{
    FilterEffect* input1 = filterBuilder->getEffectById(in1());
    float xRadius = radiusX();
    float yRadius = radiusY();

    if (!input1)
        return 0;

    if (xRadius < 0 || yRadius < 0)
        return 0;

    RefPtr<FilterEffect> effect = FEMorphology::create(filter, static_cast<MorphologyOperatorType>(_operator()), xRadius, yRadius);
    effect->inputEffects().append(input1);
    return effect.release();
}

} // namespace WebCore

#endif // ENABLE(SVG)
