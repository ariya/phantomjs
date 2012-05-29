/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Oliver Hunt <oliver@nerget.com>
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
#include "SVGFELightElement.h"

#include "Attribute.h"
#include "RenderObject.h"
#include "RenderSVGResource.h"
#include "SVGFEDiffuseLightingElement.h"
#include "SVGFESpecularLightingElement.h"
#include "SVGFilterElement.h"
#include "SVGFilterPrimitiveStandardAttributes.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_NUMBER(SVGFELightElement, SVGNames::azimuthAttr, Azimuth, azimuth)
DEFINE_ANIMATED_NUMBER(SVGFELightElement, SVGNames::elevationAttr, Elevation, elevation)
DEFINE_ANIMATED_NUMBER(SVGFELightElement, SVGNames::xAttr, X, x)
DEFINE_ANIMATED_NUMBER(SVGFELightElement, SVGNames::yAttr, Y, y)
DEFINE_ANIMATED_NUMBER(SVGFELightElement, SVGNames::zAttr, Z, z)
DEFINE_ANIMATED_NUMBER(SVGFELightElement, SVGNames::pointsAtXAttr, PointsAtX, pointsAtX)
DEFINE_ANIMATED_NUMBER(SVGFELightElement, SVGNames::pointsAtYAttr, PointsAtY, pointsAtY)
DEFINE_ANIMATED_NUMBER(SVGFELightElement, SVGNames::pointsAtZAttr, PointsAtZ, pointsAtZ)
DEFINE_ANIMATED_NUMBER(SVGFELightElement, SVGNames::specularExponentAttr, SpecularExponent, specularExponent)
DEFINE_ANIMATED_NUMBER(SVGFELightElement, SVGNames::limitingConeAngleAttr, LimitingConeAngle, limitingConeAngle)

SVGFELightElement::SVGFELightElement(const QualifiedName& tagName, Document* document)
    : SVGElement(tagName, document)
    , m_specularExponent(1)
{
}

SVGFELightElement* SVGFELightElement::findLightElement(const SVGElement* svgElement)
{
    for (Node* node = svgElement->firstChild(); node; node = node->nextSibling()) {
        if (node->hasTagName(SVGNames::feDistantLightTag)
            || node->hasTagName(SVGNames::fePointLightTag)
            || node->hasTagName(SVGNames::feSpotLightTag)) {
            return static_cast<SVGFELightElement*>(node);
        }
    }
    return 0;
}

PassRefPtr<LightSource> SVGFELightElement::findLightSource(const SVGElement* svgElement)
{
    SVGFELightElement* lightNode = findLightElement(svgElement);
    if (!lightNode)
        return 0;
    return lightNode->lightSource();
}

void SVGFELightElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::azimuthAttr)
        setAzimuthBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::elevationAttr)
        setElevationBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::xAttr)
        setXBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::yAttr)
        setYBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::zAttr)
        setZBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::pointsAtXAttr)
        setPointsAtXBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::pointsAtYAttr)
        setPointsAtYBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::pointsAtZAttr)
        setPointsAtZBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::specularExponentAttr)
        setSpecularExponentBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::limitingConeAngleAttr)
        setLimitingConeAngleBaseValue(value.toFloat());
    else
        SVGElement::parseMappedAttribute(attr);
}

void SVGFELightElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGElement::svgAttributeChanged(attrName);

    if (attrName == SVGNames::azimuthAttr
        || attrName == SVGNames::elevationAttr
        || attrName == SVGNames::xAttr
        || attrName == SVGNames::yAttr
        || attrName == SVGNames::zAttr
        || attrName == SVGNames::pointsAtXAttr
        || attrName == SVGNames::pointsAtYAttr
        || attrName == SVGNames::pointsAtZAttr
        || attrName == SVGNames::specularExponentAttr
        || attrName == SVGNames::limitingConeAngleAttr) {
        ContainerNode* parent = parentNode();
        if (!parent)
            return;

        RenderObject* renderer = parent->renderer();
        if (!renderer || !renderer->isSVGResourceFilterPrimitive())
            return;

        if (parent->hasTagName(SVGNames::feDiffuseLightingTag)) {
            SVGFEDiffuseLightingElement* diffuseLighting = static_cast<SVGFEDiffuseLightingElement*>(parent);
            diffuseLighting->lightElementAttributeChanged(this, attrName);
            return;
        } else if (parent->hasTagName(SVGNames::feSpecularLightingTag)) {
            SVGFESpecularLightingElement* specularLighting = static_cast<SVGFESpecularLightingElement*>(parent);
            specularLighting->lightElementAttributeChanged(this, attrName);
            return;
        }
    }
}

void SVGFELightElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeAzimuth();
        synchronizeElevation();
        synchronizeX();
        synchronizeY();
        synchronizeZ();
        synchronizePointsAtX();
        synchronizePointsAtY();
        synchronizePointsAtZ();
        synchronizeSpecularExponent();
        synchronizeLimitingConeAngle();
        return;
    }

    if (attrName == SVGNames::azimuthAttr)
        synchronizeAzimuth();
    else if (attrName == SVGNames::elevationAttr)
        synchronizeElevation();
    else if (attrName == SVGNames::xAttr)
        synchronizeX();
    else if (attrName == SVGNames::yAttr)
        synchronizeY();
    else if (attrName == SVGNames::zAttr)
        synchronizeZ();
    else if (attrName == SVGNames::pointsAtXAttr)
        synchronizePointsAtX();
    else if (attrName == SVGNames::pointsAtYAttr)
        synchronizePointsAtY();
    else if (attrName == SVGNames::pointsAtZAttr)
        synchronizePointsAtZ();
    else if (attrName == SVGNames::specularExponentAttr)
        synchronizeSpecularExponent();
    else if (attrName == SVGNames::limitingConeAngleAttr)
        synchronizeLimitingConeAngle();
}

AttributeToPropertyTypeMap& SVGFELightElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGFELightElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();
    attributeToPropertyTypeMap.set(SVGNames::azimuthAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::elevationAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::xAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::yAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::zAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::pointsAtXAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::pointsAtYAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::pointsAtZAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::specularExponentAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::limitingConeAngleAttr, AnimatedNumber);
}

void SVGFELightElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    SVGElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);

    if (!changedByParser) {
        if (ContainerNode* parent = parentNode()) {
            RenderObject* renderer = parent->renderer();
            if (renderer && renderer->isSVGResourceFilterPrimitive())
                RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
        }
    }
}

}

#endif // ENABLE(SVG)
