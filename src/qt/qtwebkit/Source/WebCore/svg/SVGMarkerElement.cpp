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
#include "SVGElementInstance.h"
#include "SVGFitToViewBox.h"
#include "SVGNames.h"
#include "SVGSVGElement.h"

namespace WebCore {
 
// Define custom animated property 'orientType'.
const SVGPropertyInfo* SVGMarkerElement::orientTypePropertyInfo()
{
    static const SVGPropertyInfo* s_propertyInfo = 0;
    if (!s_propertyInfo) {
        s_propertyInfo = new SVGPropertyInfo(AnimatedEnumeration,
                                             PropertyIsReadWrite,
                                             SVGNames::orientAttr,
                                             orientTypeIdentifier(),
                                             &SVGMarkerElement::synchronizeOrientType,
                                             &SVGMarkerElement::lookupOrCreateOrientTypeWrapper);
    }
    return s_propertyInfo;
}

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGMarkerElement, SVGNames::refXAttr, RefX, refX)
DEFINE_ANIMATED_LENGTH(SVGMarkerElement, SVGNames::refYAttr, RefY, refY)
DEFINE_ANIMATED_LENGTH(SVGMarkerElement, SVGNames::markerWidthAttr, MarkerWidth, markerWidth)
DEFINE_ANIMATED_LENGTH(SVGMarkerElement, SVGNames::markerHeightAttr, MarkerHeight, markerHeight)
DEFINE_ANIMATED_ENUMERATION(SVGMarkerElement, SVGNames::markerUnitsAttr, MarkerUnits, markerUnits, SVGMarkerUnitsType)
DEFINE_ANIMATED_ANGLE_AND_ENUMERATION(SVGMarkerElement, SVGNames::orientAttr, orientAngleIdentifier(), OrientAngle, orientAngle)
DEFINE_ANIMATED_BOOLEAN(SVGMarkerElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)
DEFINE_ANIMATED_RECT(SVGMarkerElement, SVGNames::viewBoxAttr, ViewBox, viewBox)
DEFINE_ANIMATED_PRESERVEASPECTRATIO(SVGMarkerElement, SVGNames::preserveAspectRatioAttr, PreserveAspectRatio, preserveAspectRatio)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGMarkerElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(refX)
    REGISTER_LOCAL_ANIMATED_PROPERTY(refY)
    REGISTER_LOCAL_ANIMATED_PROPERTY(markerWidth)
    REGISTER_LOCAL_ANIMATED_PROPERTY(markerHeight)
    REGISTER_LOCAL_ANIMATED_PROPERTY(markerUnits)
    REGISTER_LOCAL_ANIMATED_PROPERTY(orientAngle)
    REGISTER_LOCAL_ANIMATED_PROPERTY(orientType)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_LOCAL_ANIMATED_PROPERTY(viewBox)
    REGISTER_LOCAL_ANIMATED_PROPERTY(preserveAspectRatio)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGStyledElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGMarkerElement::SVGMarkerElement(const QualifiedName& tagName, Document* document)
    : SVGStyledElement(tagName, document)
    , m_refX(LengthModeWidth)
    , m_refY(LengthModeHeight)
    , m_markerWidth(LengthModeWidth, "3")
    , m_markerHeight(LengthModeHeight, "3") 
    , m_markerUnits(SVGMarkerUnitsStrokeWidth)
    , m_orientType(SVGMarkerOrientAngle)
{
    // Spec: If the markerWidth/markerHeight attribute is not specified, the effect is as if a value of "3" were specified.
    ASSERT(hasTagName(SVGNames::markerTag));
    registerAnimatedPropertiesForSVGMarkerElement();
}

PassRefPtr<SVGMarkerElement> SVGMarkerElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGMarkerElement(tagName, document));
}

const AtomicString& SVGMarkerElement::orientTypeIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGOrientType", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

const AtomicString& SVGMarkerElement::orientAngleIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGOrientAngle", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

AffineTransform SVGMarkerElement::viewBoxToViewTransform(float viewWidth, float viewHeight) const
{
    return SVGFitToViewBox::viewBoxToViewTransform(viewBox(), preserveAspectRatio(), viewWidth, viewHeight);
}

bool SVGMarkerElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        SVGLangSpace::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        SVGFitToViewBox::addSupportedAttributes(supportedAttributes);
        supportedAttributes.add(SVGNames::markerUnitsAttr);
        supportedAttributes.add(SVGNames::refXAttr);
        supportedAttributes.add(SVGNames::refYAttr);
        supportedAttributes.add(SVGNames::markerWidthAttr);
        supportedAttributes.add(SVGNames::markerHeightAttr);
        supportedAttributes.add(SVGNames::orientAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGMarkerElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    SVGParsingError parseError = NoError;

    if (!isSupportedAttribute(name))
        SVGStyledElement::parseAttribute(name, value);
    else if (name == SVGNames::markerUnitsAttr) {
        SVGMarkerUnitsType propertyValue = SVGPropertyTraits<SVGMarkerUnitsType>::fromString(value);
        if (propertyValue > 0)
            setMarkerUnitsBaseValue(propertyValue);
    } else if (name == SVGNames::refXAttr)
        setRefXBaseValue(SVGLength::construct(LengthModeWidth, value, parseError));
    else if (name == SVGNames::refYAttr)
        setRefYBaseValue(SVGLength::construct(LengthModeHeight, value, parseError));
    else if (name == SVGNames::markerWidthAttr)
        setMarkerWidthBaseValue(SVGLength::construct(LengthModeWidth, value, parseError));
    else if (name == SVGNames::markerHeightAttr)
        setMarkerHeightBaseValue(SVGLength::construct(LengthModeHeight, value, parseError));
    else if (name == SVGNames::orientAttr) {
        SVGAngle angle;
        SVGMarkerOrientType orientType = SVGPropertyTraits<SVGMarkerOrientType>::fromString(value, angle);
        if (orientType > 0)
            setOrientTypeBaseValue(orientType);
        if (orientType == SVGMarkerOrientAngle)
            setOrientAngleBaseValue(angle);
    } else if (SVGLangSpace::parseAttribute(name, value)
             || SVGExternalResourcesRequired::parseAttribute(name, value)
             || SVGFitToViewBox::parseAttribute(this, name, value)) {
    } else
        ASSERT_NOT_REACHED();

    reportAttributeParsingError(parseError, name, value);
}

void SVGMarkerElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGStyledElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);
    
    if (attrName == SVGNames::refXAttr
        || attrName == SVGNames::refYAttr
        || attrName == SVGNames::markerWidthAttr
        || attrName == SVGNames::markerHeightAttr)
        updateRelativeLengthsInformation();

    if (RenderObject* object = renderer())
        object->setNeedsLayout(true);
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
    setOrientTypeBaseValue(SVGMarkerOrientAuto);
    setOrientAngleBaseValue(SVGAngle());
 
    // Mark orientAttr dirty - the next XML DOM access of that attribute kicks in synchronization.
    m_orientAngle.shouldSynchronize = true;
    m_orientType.shouldSynchronize = true;
    invalidateSVGAttributes();
    svgAttributeChanged(orientAnglePropertyInfo()->attributeName);
}

void SVGMarkerElement::setOrientToAngle(const SVGAngle& angle)
{
    setOrientTypeBaseValue(SVGMarkerOrientAngle);
    setOrientAngleBaseValue(angle);

    // Mark orientAttr dirty - the next XML DOM access of that attribute kicks in synchronization.
    m_orientAngle.shouldSynchronize = true;
    m_orientType.shouldSynchronize = true;
    invalidateSVGAttributes();
    svgAttributeChanged(orientAnglePropertyInfo()->attributeName);
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

void SVGMarkerElement::synchronizeOrientType(SVGElement* contextElement)
{
    ASSERT(contextElement);
    SVGMarkerElement* ownerType = toSVGMarkerElement(contextElement);
    if (!ownerType->m_orientType.shouldSynchronize)
        return;

    // If orient is not auto, the previous call to synchronizeOrientAngle already set the orientAttr to the right angle.
    if (ownerType->m_orientType.value != SVGMarkerOrientAuto)
        return;

    DEFINE_STATIC_LOCAL(AtomicString, autoString, ("auto", AtomicString::ConstructFromLiteral));
    ownerType->m_orientType.synchronize(ownerType, orientTypePropertyInfo()->attributeName, autoString);
}

PassRefPtr<SVGAnimatedProperty> SVGMarkerElement::lookupOrCreateOrientTypeWrapper(SVGElement* contextElement)
{
    ASSERT(contextElement);
    SVGMarkerElement* ownerType = toSVGMarkerElement(contextElement);
    return SVGAnimatedProperty::lookupOrCreateWrapper<SVGMarkerElement, SVGAnimatedEnumerationPropertyTearOff<SVGMarkerOrientType>, SVGMarkerOrientType>
           (ownerType, orientTypePropertyInfo(), ownerType->m_orientType.value);
}
  
PassRefPtr<SVGAnimatedEnumerationPropertyTearOff<SVGMarkerOrientType> > SVGMarkerElement::orientTypeAnimated()
{
    m_orientType.shouldSynchronize = true;
    return static_pointer_cast<SVGAnimatedEnumerationPropertyTearOff<SVGMarkerOrientType> >(lookupOrCreateOrientTypeWrapper(this));
}

}

#endif
