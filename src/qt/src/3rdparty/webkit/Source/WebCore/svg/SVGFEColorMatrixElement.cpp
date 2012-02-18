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
#include "SVGFEColorMatrixElement.h"

#include "Attribute.h"
#include "FilterEffect.h"
#include "SVGFilterBuilder.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGFEColorMatrixElement, SVGNames::inAttr, In1, in1)
DEFINE_ANIMATED_ENUMERATION(SVGFEColorMatrixElement, SVGNames::typeAttr, Type, type)
DEFINE_ANIMATED_NUMBER_LIST(SVGFEColorMatrixElement, SVGNames::valuesAttr, Values, values)

inline SVGFEColorMatrixElement::SVGFEColorMatrixElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
    , m_type(FECOLORMATRIX_TYPE_UNKNOWN)
{
}

PassRefPtr<SVGFEColorMatrixElement> SVGFEColorMatrixElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFEColorMatrixElement(tagName, document));
}

void SVGFEColorMatrixElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::typeAttr) {
        if (value == "matrix")
            setTypeBaseValue(FECOLORMATRIX_TYPE_MATRIX);
        else if (value == "saturate")
            setTypeBaseValue(FECOLORMATRIX_TYPE_SATURATE);
        else if (value == "hueRotate")
            setTypeBaseValue(FECOLORMATRIX_TYPE_HUEROTATE);
        else if (value == "luminanceToAlpha")
            setTypeBaseValue(FECOLORMATRIX_TYPE_LUMINANCETOALPHA);
    } else if (attr->name() == SVGNames::inAttr)
        setIn1BaseValue(value);
    else if (attr->name() == SVGNames::valuesAttr) {
        SVGNumberList newList;
        newList.parse(value);
        detachAnimatedValuesListWrappers(newList.size());
        setValuesBaseValue(newList);
    } else
        SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(attr);
}

bool SVGFEColorMatrixElement::setFilterEffectAttribute(FilterEffect* effect, const QualifiedName& attrName)
{
    FEColorMatrix* colorMatrix = static_cast<FEColorMatrix*>(effect);
    if (attrName == SVGNames::typeAttr)
        return colorMatrix->setType(static_cast<ColorMatrixType>(type()));
    if (attrName == SVGNames::valuesAttr)
        return colorMatrix->setValues(values());

    ASSERT_NOT_REACHED();
    return false;
}

void SVGFEColorMatrixElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);

    if (attrName == SVGNames::typeAttr
        || attrName == SVGNames::valuesAttr)
        primitiveAttributeChanged(attrName);
    if (attrName == SVGNames::inAttr)
        invalidate();
}

void SVGFEColorMatrixElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeType();
        synchronizeIn1();
        synchronizeValues();
        return;
    }

    if (attrName == SVGNames::typeAttr)
        synchronizeType();
    else if (attrName == SVGNames::inAttr)
        synchronizeIn1();
    else if (attrName == SVGNames::valuesAttr)
        synchronizeValues();
}

AttributeToPropertyTypeMap& SVGFEColorMatrixElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGFEColorMatrixElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGFilterPrimitiveStandardAttributes::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::inAttr, AnimatedString);
    attributeToPropertyTypeMap.set(SVGNames::typeAttr, AnimatedEnumeration);
    attributeToPropertyTypeMap.set(SVGNames::valuesAttr, AnimatedNumberList);
}

PassRefPtr<FilterEffect> SVGFEColorMatrixElement::build(SVGFilterBuilder* filterBuilder, Filter* filter)
{
    FilterEffect* input1 = filterBuilder->getEffectById(in1());

    if (!input1)
        return 0;

    Vector<float> filterValues;
    const ColorMatrixType filterType(static_cast<ColorMatrixType>(type()));

    // Use defaults if values is empty (SVG 1.1 15.10).
    if (!hasAttribute(SVGNames::valuesAttr)) {
        switch (filterType) {
        case FECOLORMATRIX_TYPE_MATRIX:
            for (size_t i = 0; i < 20; i++)
                filterValues.append((i % 6) ? 0.0f : 1.0f);
            break;
        case FECOLORMATRIX_TYPE_HUEROTATE:
            filterValues.append(0.0f);
            break;
        case FECOLORMATRIX_TYPE_SATURATE:
            filterValues.append(1.0f);
            break;
        default:
            break;
        }
    } else {
        filterValues = values();
        unsigned size = filterValues.size();

        if ((filterType == FECOLORMATRIX_TYPE_MATRIX && size != 20)
            || (filterType == FECOLORMATRIX_TYPE_HUEROTATE && size != 1)
            || (filterType == FECOLORMATRIX_TYPE_SATURATE && (size != 1
                || filterValues[0] < 0.0f || filterValues[0] > 1.0f)))
            return 0;
    }

    RefPtr<FilterEffect> effect = FEColorMatrix::create(filter, filterType, filterValues);
    effect->inputEffects().append(input1);
    return effect.release();
}

} // namespace WebCore

#endif // ENABLE(SVG)
