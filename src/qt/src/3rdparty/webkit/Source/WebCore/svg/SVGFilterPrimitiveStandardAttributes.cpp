/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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
#include "SVGFilterPrimitiveStandardAttributes.h"

#include "Attribute.h"
#include "FilterEffect.h"
#include "RenderSVGResourceFilterPrimitive.h"
#include "SVGFilterBuilder.h"
#include "SVGLength.h"
#include "SVGNames.h"
#include "SVGStyledElement.h"
#include "SVGUnitTypes.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGFilterPrimitiveStandardAttributes, SVGNames::xAttr, X, x)
DEFINE_ANIMATED_LENGTH(SVGFilterPrimitiveStandardAttributes, SVGNames::yAttr, Y, y)
DEFINE_ANIMATED_LENGTH(SVGFilterPrimitiveStandardAttributes, SVGNames::widthAttr, Width, width)
DEFINE_ANIMATED_LENGTH(SVGFilterPrimitiveStandardAttributes, SVGNames::heightAttr, Height, height)
DEFINE_ANIMATED_STRING(SVGFilterPrimitiveStandardAttributes, SVGNames::resultAttr, Result, result)

SVGFilterPrimitiveStandardAttributes::SVGFilterPrimitiveStandardAttributes(const QualifiedName& tagName, Document* document)
    : SVGStyledElement(tagName, document)
    , m_x(LengthModeWidth, "0%")
    , m_y(LengthModeHeight, "0%")
    , m_width(LengthModeWidth, "100%")
    , m_height(LengthModeHeight, "100%")
{
    // Spec: If the x/y attribute is not specified, the effect is as if a value of "0%" were specified.
    // Spec: If the width/height attribute is not specified, the effect is as if a value of "100%" were specified.
}

void SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(Attribute* attr)
{
    const AtomicString& value = attr->value();
    if (attr->name() == SVGNames::xAttr)
        setXBaseValue(SVGLength(LengthModeWidth, value));
    else if (attr->name() == SVGNames::yAttr)
        setYBaseValue(SVGLength(LengthModeHeight, value));
    else if (attr->name() == SVGNames::widthAttr)
        setWidthBaseValue(SVGLength(LengthModeWidth, value));
    else if (attr->name() == SVGNames::heightAttr)
        setHeightBaseValue(SVGLength(LengthModeHeight, value));
    else if (attr->name() == SVGNames::resultAttr)
        setResultBaseValue(value);
    else
        return SVGStyledElement::parseMappedAttribute(attr);
}

bool SVGFilterPrimitiveStandardAttributes::setFilterEffectAttribute(FilterEffect*, const QualifiedName&)
{
    // When all filters support this method, it will be changed to a pure virtual method.
    ASSERT_NOT_REACHED();
    return false;
}

void SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGStyledElement::svgAttributeChanged(attrName);

    if (attrName == SVGNames::xAttr
        || attrName == SVGNames::yAttr
        || attrName == SVGNames::widthAttr
        || attrName == SVGNames::heightAttr
        || attrName == SVGNames::resultAttr)
        invalidate();
}

void SVGFilterPrimitiveStandardAttributes::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeX();
        synchronizeY();
        synchronizeWidth();
        synchronizeHeight();
        synchronizeResult();
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
    else if (attrName == SVGNames::resultAttr)
        synchronizeResult();
}

void SVGFilterPrimitiveStandardAttributes::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    SVGStyledElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);

    if (!changedByParser)
        invalidate();
}

void SVGFilterPrimitiveStandardAttributes::setStandardAttributes(bool primitiveBoundingBoxMode, FilterEffect* filterEffect) const
{
    ASSERT(filterEffect);
    if (!filterEffect)
        return;

    if (this->hasAttribute(SVGNames::xAttr))
        filterEffect->setHasX(true);
    if (this->hasAttribute(SVGNames::yAttr))
        filterEffect->setHasY(true);
    if (this->hasAttribute(SVGNames::widthAttr))
        filterEffect->setHasWidth(true);
    if (this->hasAttribute(SVGNames::heightAttr))
        filterEffect->setHasHeight(true);

    FloatRect effectBBox;
    if (primitiveBoundingBoxMode)
        effectBBox = FloatRect(x().valueAsPercentage(),
                               y().valueAsPercentage(),
                               width().valueAsPercentage(),
                               height().valueAsPercentage());
    else
        effectBBox = FloatRect(x().value(this),
                               y().value(this),
                               width().value(this),
                               height().value(this));

    filterEffect->setEffectBoundaries(effectBBox);
}

void SVGFilterPrimitiveStandardAttributes::fillPassedAttributeToPropertyTypeMap(AttributeToPropertyTypeMap& attributeToPropertyTypeMap)
{
    SVGStyledElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    
    attributeToPropertyTypeMap.set(SVGNames::xAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::yAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::widthAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::heightAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::resultAttr, AnimatedString);
}

RenderObject* SVGFilterPrimitiveStandardAttributes::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGResourceFilterPrimitive(this);
}

bool SVGFilterPrimitiveStandardAttributes::rendererIsNeeded(RenderStyle* style)
{
    if (parentNode() && (parentNode()->hasTagName(SVGNames::filterTag)))
        return SVGStyledElement::rendererIsNeeded(style);

    return false;
}

}

#endif // ENABLE(SVG)
