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
#include "SVGComponentTransferFunctionElement.h"

#include "Attribute.h"
#include "SVGFEComponentTransferElement.h"
#include "SVGNames.h"
#include "SVGNumberList.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_ENUMERATION(SVGComponentTransferFunctionElement, SVGNames::typeAttr, Type, type)
DEFINE_ANIMATED_NUMBER_LIST(SVGComponentTransferFunctionElement, SVGNames::tableValuesAttr, TableValues, tableValues)
DEFINE_ANIMATED_NUMBER(SVGComponentTransferFunctionElement, SVGNames::slopeAttr, Slope, slope)
DEFINE_ANIMATED_NUMBER(SVGComponentTransferFunctionElement, SVGNames::interceptAttr, Intercept, intercept)
DEFINE_ANIMATED_NUMBER(SVGComponentTransferFunctionElement, SVGNames::amplitudeAttr, Amplitude, amplitude)
DEFINE_ANIMATED_NUMBER(SVGComponentTransferFunctionElement, SVGNames::exponentAttr, Exponent, exponent)
DEFINE_ANIMATED_NUMBER(SVGComponentTransferFunctionElement, SVGNames::offsetAttr, Offset, offset)

SVGComponentTransferFunctionElement::SVGComponentTransferFunctionElement(const QualifiedName& tagName, Document* document)
    : SVGElement(tagName, document)
    , m_type(FECOMPONENTTRANSFER_TYPE_UNKNOWN)
    , m_slope(1)
    , m_amplitude(1)
    , m_exponent(1)
{
}

void SVGComponentTransferFunctionElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::typeAttr) {
        if (value == "identity")
            setTypeBaseValue(FECOMPONENTTRANSFER_TYPE_IDENTITY);
        else if (value == "table")
            setTypeBaseValue(FECOMPONENTTRANSFER_TYPE_TABLE);
        else if (value == "discrete")
            setTypeBaseValue(FECOMPONENTTRANSFER_TYPE_DISCRETE);
        else if (value == "linear")
            setTypeBaseValue(FECOMPONENTTRANSFER_TYPE_LINEAR);
        else if (value == "gamma")
            setTypeBaseValue(FECOMPONENTTRANSFER_TYPE_GAMMA);
    } else if (attr->name() == SVGNames::tableValuesAttr) {
        SVGNumberList newList;
        newList.parse(value);
        detachAnimatedTableValuesListWrappers(newList.size());
        setTableValuesBaseValue(newList);
    } else if (attr->name() == SVGNames::slopeAttr)
        setSlopeBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::interceptAttr)
        setInterceptBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::amplitudeAttr)
        setAmplitudeBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::exponentAttr)
        setExponentBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::offsetAttr)
        setOffsetBaseValue(value.toFloat());
    else
        SVGElement::parseMappedAttribute(attr);
}

void SVGComponentTransferFunctionElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGElement::svgAttributeChanged(attrName);

    if (attrName == SVGNames::typeAttr) {
        ComponentTransferType componentType = static_cast<ComponentTransferType>(type());
        if (componentType < FECOMPONENTTRANSFER_TYPE_UNKNOWN || componentType > FECOMPONENTTRANSFER_TYPE_GAMMA)
            setTypeBaseValue(FECOMPONENTTRANSFER_TYPE_UNKNOWN);
    }
}

void SVGComponentTransferFunctionElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeType();
        synchronizeTableValues();
        synchronizeSlope();
        synchronizeIntercept();
        synchronizeAmplitude();
        synchronizeExponent();
        synchronizeOffset();
        return;
    }

    if (attrName == SVGNames::typeAttr)
        synchronizeType();
    else if (attrName == SVGNames::tableValuesAttr)
        synchronizeTableValues();
    else if (attrName == SVGNames::slopeAttr)
        synchronizeSlope();
    else if (attrName == SVGNames::interceptAttr)
        synchronizeIntercept();
    else if (attrName == SVGNames::amplitudeAttr)
        synchronizeAmplitude();
    else if (attrName == SVGNames::exponentAttr)
        synchronizeExponent();
    else if (attrName == SVGNames::offsetAttr)
        synchronizeOffset();
}

AttributeToPropertyTypeMap& SVGComponentTransferFunctionElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGComponentTransferFunctionElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();
    attributeToPropertyTypeMap.set(SVGNames::typeAttr, AnimatedEnumeration);
    attributeToPropertyTypeMap.set(SVGNames::tableValuesAttr, AnimatedNumberList);
    attributeToPropertyTypeMap.set(SVGNames::slopeAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::interceptAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::amplitudeAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::exponentAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::offsetAttr, AnimatedNumber);
}

ComponentTransferFunction SVGComponentTransferFunctionElement::transferFunction() const
{
    ComponentTransferFunction func;
    func.type = static_cast<ComponentTransferType>(type());
    func.slope = slope();
    func.intercept = intercept();
    func.amplitude = amplitude();
    func.exponent = exponent();
    func.offset = offset();
    func.tableValues = tableValues();
    return func;
}

}

#endif // ENABLE(SVG)
