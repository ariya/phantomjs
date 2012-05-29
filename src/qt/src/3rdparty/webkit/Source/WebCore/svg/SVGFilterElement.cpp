/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
#include "SVGFilterElement.h"

#include "Attr.h"
#include "RenderSVGResourceFilter.h"
#include "SVGFilterBuilder.h"
#include "SVGFilterPrimitiveStandardAttributes.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include "SVGUnitTypes.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_ENUMERATION(SVGFilterElement, SVGNames::filterUnitsAttr, FilterUnits, filterUnits)
DEFINE_ANIMATED_ENUMERATION(SVGFilterElement, SVGNames::primitiveUnitsAttr, PrimitiveUnits, primitiveUnits)
DEFINE_ANIMATED_LENGTH(SVGFilterElement, SVGNames::xAttr, X, x)
DEFINE_ANIMATED_LENGTH(SVGFilterElement, SVGNames::yAttr, Y, y)
DEFINE_ANIMATED_LENGTH(SVGFilterElement, SVGNames::widthAttr, Width, width)
DEFINE_ANIMATED_LENGTH(SVGFilterElement, SVGNames::heightAttr, Height, height)
DEFINE_ANIMATED_INTEGER_MULTIPLE_WRAPPERS(SVGFilterElement, SVGNames::filterResAttr, filterResXIdentifier(), FilterResX, filterResX)
DEFINE_ANIMATED_INTEGER_MULTIPLE_WRAPPERS(SVGFilterElement, SVGNames::filterResAttr, filterResYIdentifier(), FilterResY, filterResY)
DEFINE_ANIMATED_STRING(SVGFilterElement, XLinkNames::hrefAttr, Href, href)
DEFINE_ANIMATED_BOOLEAN(SVGFilterElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

inline SVGFilterElement::SVGFilterElement(const QualifiedName& tagName, Document* document)
    : SVGStyledElement(tagName, document)
    , SVGURIReference()
    , SVGLangSpace()
    , SVGExternalResourcesRequired()
    , m_filterUnits(SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX)
    , m_primitiveUnits(SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE)
    , m_x(LengthModeWidth, "-10%")
    , m_y(LengthModeHeight, "-10%")
    , m_width(LengthModeWidth, "120%")
    , m_height(LengthModeHeight, "120%")
{
    // Spec: If the x/y attribute is not specified, the effect is as if a value of "-10%" were specified.
    // Spec: If the width/height attribute is not specified, the effect is as if a value of "120%" were specified.
}

PassRefPtr<SVGFilterElement> SVGFilterElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFilterElement(tagName, document));
}

const AtomicString& SVGFilterElement::filterResXIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGFilterResX"));
    return s_identifier;
}

const AtomicString& SVGFilterElement::filterResYIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGFilterResY"));
    return s_identifier;
}

void SVGFilterElement::setFilterRes(unsigned long filterResX, unsigned long filterResY)
{
    setFilterResXBaseValue(filterResX);
    setFilterResYBaseValue(filterResY);

    if (RenderObject* object = renderer())
        object->setNeedsLayout(true);
}

void SVGFilterElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::filterUnitsAttr) {
        if (value == "userSpaceOnUse")
            setFilterUnitsBaseValue(SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE);
        else if (value == "objectBoundingBox")
            setFilterUnitsBaseValue(SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
    } else if (attr->name() == SVGNames::primitiveUnitsAttr) {
        if (value == "userSpaceOnUse")
            setPrimitiveUnitsBaseValue(SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE);
        else if (value == "objectBoundingBox")
            setPrimitiveUnitsBaseValue(SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
    } else if (attr->name() == SVGNames::xAttr)
        setXBaseValue(SVGLength(LengthModeWidth, value));
    else if (attr->name() == SVGNames::yAttr)
        setYBaseValue(SVGLength(LengthModeHeight, value));
    else if (attr->name() == SVGNames::widthAttr)
        setWidthBaseValue(SVGLength(LengthModeWidth, value));
    else if (attr->name() == SVGNames::heightAttr)
        setHeightBaseValue(SVGLength(LengthModeHeight, value));
    else if (attr->name() == SVGNames::filterResAttr) {
        float x, y;
        if (parseNumberOptionalNumber(value, x, y)) {
            setFilterResXBaseValue(x);
            setFilterResYBaseValue(y);
        }
    } else {
        if (SVGURIReference::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;

        SVGStyledElement::parseMappedAttribute(attr);
    }
}

void SVGFilterElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGStyledElement::svgAttributeChanged(attrName);

    bool invalidateClients = false;
    if (attrName == SVGNames::xAttr
        || attrName == SVGNames::yAttr
        || attrName == SVGNames::widthAttr
        || attrName == SVGNames::heightAttr) {
        invalidateClients = true;
        updateRelativeLengthsInformation();
    }

    RenderObject* object = renderer();
    if (!object)
        return;

    if (invalidateClients
        || attrName == SVGNames::filterUnitsAttr
        || attrName == SVGNames::primitiveUnitsAttr
        || attrName == SVGNames::filterResAttr
        || SVGStyledElement::isKnownAttribute(attrName)
        || SVGURIReference::isKnownAttribute(attrName)
        || SVGLangSpace::isKnownAttribute(attrName)
        || SVGExternalResourcesRequired::isKnownAttribute(attrName))
        object->setNeedsLayout(true);
}

void SVGFilterElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeX();
        synchronizeY();
        synchronizeWidth();
        synchronizeHeight();
        synchronizeFilterUnits();
        synchronizePrimitiveUnits();
        synchronizeFilterResX();
        synchronizeFilterResY();
        synchronizeExternalResourcesRequired();
        synchronizeHref();
        return;
    }

    if (attrName == SVGNames::xAttr)
        synchronizeX();
    else if (attrName == SVGNames::yAttr)
        synchronizeY();
    else if (attrName == SVGNames::widthAttr)
        synchronizeWidth();
    else if (attrName == SVGNames::heightAttr)
        synchronizeHeight();
    else if (attrName == SVGNames::filterUnitsAttr)
        synchronizeFilterUnits();
    else if (attrName == SVGNames::primitiveUnitsAttr)
        synchronizePrimitiveUnits();
    else if (attrName == SVGNames::filterResAttr) {
        synchronizeFilterResX();
        synchronizeFilterResY();
    } else if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
    else if (SVGURIReference::isKnownAttribute(attrName))
        synchronizeHref();
}

AttributeToPropertyTypeMap& SVGFilterElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGFilterElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGStyledElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::filterUnitsAttr, AnimatedEnumeration);
    attributeToPropertyTypeMap.set(SVGNames::primitiveUnitsAttr, AnimatedEnumeration);
    attributeToPropertyTypeMap.set(SVGNames::xAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::yAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::widthAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::heightAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::filterResAttr, AnimatedNumberOptionalNumber);
    attributeToPropertyTypeMap.set(XLinkNames::hrefAttr, AnimatedString);
}

void SVGFilterElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    SVGStyledElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);

    if (changedByParser)
        return;

    if (RenderObject* object = renderer())
        object->setNeedsLayout(true);
}

FloatRect SVGFilterElement::filterBoundingBox(const FloatRect& objectBoundingBox) const
{
    FloatRect filterBBox;
    if (filterUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX)
        filterBBox = FloatRect(x().valueAsPercentage() * objectBoundingBox.width() + objectBoundingBox.x(),
                               y().valueAsPercentage() * objectBoundingBox.height() + objectBoundingBox.y(),
                               width().valueAsPercentage() * objectBoundingBox.width(),
                               height().valueAsPercentage() * objectBoundingBox.height());
    else
        filterBBox = FloatRect(x().value(this),
                               y().value(this),
                               width().value(this),
                               height().value(this));

    return filterBBox;
}

RenderObject* SVGFilterElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGResourceFilter(this);
}

bool SVGFilterElement::selfHasRelativeLengths() const
{
    return x().isRelative()
        || y().isRelative()
        || width().isRelative()
        || height().isRelative();
}

}

#endif
