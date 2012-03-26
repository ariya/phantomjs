/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
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

#if ENABLE(SVG)
#include "SVGMarkerElement.h"

#include "Attribute.h"
#include "RenderSVGResourceMarker.h"
#include "SVGFitToViewBox.h"
#include "SVGNames.h"
#include "SVGSVGElement.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGMarkerElement, SVGNames::refXAttr, RefX, refX)
DEFINE_ANIMATED_LENGTH(SVGMarkerElement, SVGNames::refYAttr, RefY, refY)
DEFINE_ANIMATED_LENGTH(SVGMarkerElement, SVGNames::markerWidthAttr, MarkerWidth, markerWidth)
DEFINE_ANIMATED_LENGTH(SVGMarkerElement, SVGNames::markerHeightAttr, MarkerHeight, markerHeight)
DEFINE_ANIMATED_ENUMERATION(SVGMarkerElement, SVGNames::markerUnitsAttr, MarkerUnits, markerUnits)
DEFINE_ANIMATED_ENUMERATION_MULTIPLE_WRAPPERS(SVGMarkerElement, SVGNames::orientAttr, orientTypeIdentifier(), OrientType, orientType)
DEFINE_ANIMATED_ANGLE_MULTIPLE_WRAPPERS(SVGMarkerElement, SVGNames::orientAttr, orientAngleIdentifier(), OrientAngle, orientAngle)
DEFINE_ANIMATED_BOOLEAN(SVGMarkerElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)
DEFINE_ANIMATED_RECT(SVGMarkerElement, SVGNames::viewBoxAttr, ViewBox, viewBox)
DEFINE_ANIMATED_PRESERVEASPECTRATIO(SVGMarkerElement, SVGNames::preserveAspectRatioAttr, PreserveAspectRatio, preserveAspectRatio)

inline SVGMarkerElement::SVGMarkerElement(const QualifiedName& tagName, Document* document)
    : SVGStyledElement(tagName, document)
    , m_refX(LengthModeWidth)
    , m_refY(LengthModeHeight)
    , m_markerWidth(LengthModeWidth, "3")
    , m_markerHeight(LengthModeHeight, "3") 
    , m_markerUnits(SVG_MARKERUNITS_STROKEWIDTH)
    , m_orientType(SVG_MARKER_ORIENT_ANGLE)
{
    // Spec: If the markerWidth/markerHeight attribute is not specified, the effect is as if a value of "3" were specified.
}

PassRefPtr<SVGMarkerElement> SVGMarkerElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGMarkerElement(tagName, document));
}

const AtomicString& SVGMarkerElement::orientTypeIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGOrientType"));
    return s_identifier;
}

const AtomicString& SVGMarkerElement::orientAngleIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGOrientAngle"));
    return s_identifier;
}

AffineTransform SVGMarkerElement::viewBoxToViewTransform(float viewWidth, float viewHeight) const
{
    return SVGFitToViewBox::viewBoxToViewTransform(viewBox(), preserveAspectRatio(), viewWidth, viewHeight);
}

void SVGMarkerElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::markerUnitsAttr) {
        if (attr->value() == "userSpaceOnUse")
            setMarkerUnitsBaseValue(SVG_MARKERUNITS_USERSPACEONUSE);
        else if (attr->value() == "strokeWidth")
            setMarkerUnitsBaseValue(SVG_MARKERUNITS_STROKEWIDTH);
    } else if (attr->name() == SVGNames::refXAttr)
        setRefXBaseValue(SVGLength(LengthModeWidth, attr->value()));
    else if (attr->name() == SVGNames::refYAttr)
        setRefYBaseValue(SVGLength(LengthModeHeight, attr->value()));
    else if (attr->name() == SVGNames::markerWidthAttr)
        setMarkerWidthBaseValue(SVGLength(LengthModeWidth, attr->value()));
    else if (attr->name() == SVGNames::markerHeightAttr)
        setMarkerHeightBaseValue(SVGLength(LengthModeHeight, attr->value()));
    else if (attr->name() == SVGNames::orientAttr) {
        SVGAngle angle;

        if (attr->value() == "auto")
            setOrientTypeBaseValue(SVG_MARKER_ORIENT_AUTO);
        else {
            ExceptionCode ec = 0;
            angle.setValueAsString(attr->value(), ec);
            setOrientTypeBaseValue(SVG_MARKER_ORIENT_ANGLE);
        }

        setOrientAngleBaseValue(angle);
    } else {
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        if (SVGFitToViewBox::parseMappedAttribute(document(), attr))
            return;

        SVGStyledElement::parseMappedAttribute(attr);
    }
}

void SVGMarkerElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGStyledElement::svgAttributeChanged(attrName);

    bool invalidateClients = false;
    if (attrName == SVGNames::refXAttr
        || attrName == SVGNames::refYAttr
        || attrName == SVGNames::markerWidthAttr
        || attrName == SVGNames::markerHeightAttr) {
        invalidateClients = true;
        updateRelativeLengthsInformation();
    }

    RenderObject* object = renderer();
    if (!object)
        return;

    if (invalidateClients
        || attrName == SVGNames::markerUnitsAttr
        || attrName == SVGNames::orientAttr
        || SVGLangSpace::isKnownAttribute(attrName)
        || SVGExternalResourcesRequired::isKnownAttribute(attrName)
        || SVGFitToViewBox::isKnownAttribute(attrName)
        || SVGStyledElement::isKnownAttribute(attrName))
        object->setNeedsLayout(true);
}

void SVGMarkerElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeMarkerUnits();
        synchronizeRefX();
        synchronizeRefY();
        synchronizeMarkerWidth();
        synchronizeMarkerHeight();
        synchronizeOrientAngle();
        synchronizeOrientType();
        synchronizeExternalResourcesRequired();
        synchronizeViewBox();
        synchronizePreserveAspectRatio();
        return;
    }

    if (attrName == SVGNames::markerUnitsAttr)
        synchronizeMarkerUnits();
    else if (attrName == SVGNames::refXAttr)
        synchronizeRefX();
    else if (attrName == SVGNames::refYAttr)
        synchronizeRefY();
    else if (attrName == SVGNames::markerWidthAttr)
        synchronizeMarkerWidth();
    else if (attrName == SVGNames::markerHeightAttr)
        synchronizeMarkerHeight();
    else if (attrName == SVGNames::orientAttr) {
        synchronizeOrientAngle();
        synchronizeOrientType();
    } else if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
    else if (SVGFitToViewBox::isKnownAttribute(attrName)) {
        synchronizeViewBox();
        synchronizePreserveAspectRatio();
    }
}

AttributeToPropertyTypeMap& SVGMarkerElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGMarkerElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGStyledElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::refXAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::refYAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::markerWidthAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::markerHeightAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::markerUnitsAttr, AnimatedEnumeration);
    attributeToPropertyTypeMap.set(SVGNames::orientAttr, AnimatedAngle);
    attributeToPropertyTypeMap.set(SVGNames::viewBoxAttr, AnimatedRect);
}

void SVGMarkerElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    SVGStyledElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);

    if (changedByParser)
        return;

    if (RenderObject* object = renderer())
        object->setNeedsLayout(true);
}

void SVGMarkerElement::setOrientToAuto()
{
    setOrientTypeBaseValue(SVG_MARKER_ORIENT_AUTO);
    setOrientAngleBaseValue(SVGAngle());

    if (RenderObject* object = renderer())
        object->setNeedsLayout(true);
}

void SVGMarkerElement::setOrientToAngle(const SVGAngle& angle)
{
    setOrientTypeBaseValue(SVG_MARKER_ORIENT_ANGLE);
    setOrientAngleBaseValue(angle);

    if (RenderObject* object = renderer())
        object->setNeedsLayout(true);
}

RenderObject* SVGMarkerElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGResourceMarker(this);
}

bool SVGMarkerElement::selfHasRelativeLengths() const
{
    return refX().isRelative()
        || refY().isRelative()
        || markerWidth().isRelative()
        || markerHeight().isRelative();
}

}

#endif
