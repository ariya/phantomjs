/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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
#include "SVGEllipseElement.h"

#include "Attribute.h"
#include "FloatPoint.h"
#include "RenderSVGPath.h"
#include "RenderSVGResource.h"
#include "SVGLength.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGEllipseElement, SVGNames::cxAttr, Cx, cx)
DEFINE_ANIMATED_LENGTH(SVGEllipseElement, SVGNames::cyAttr, Cy, cy)
DEFINE_ANIMATED_LENGTH(SVGEllipseElement, SVGNames::rxAttr, Rx, rx)
DEFINE_ANIMATED_LENGTH(SVGEllipseElement, SVGNames::ryAttr, Ry, ry)
DEFINE_ANIMATED_BOOLEAN(SVGEllipseElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

inline SVGEllipseElement::SVGEllipseElement(const QualifiedName& tagName, Document* document)
    : SVGStyledTransformableElement(tagName, document)
    , m_cx(LengthModeWidth)
    , m_cy(LengthModeHeight)
    , m_rx(LengthModeWidth)
    , m_ry(LengthModeHeight)
{
}    

PassRefPtr<SVGEllipseElement> SVGEllipseElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGEllipseElement(tagName, document));
}

void SVGEllipseElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::cxAttr)
        setCxBaseValue(SVGLength(LengthModeWidth, attr->value()));
    else if (attr->name() == SVGNames::cyAttr)
        setCyBaseValue(SVGLength(LengthModeHeight, attr->value()));
    else if (attr->name() == SVGNames::rxAttr) {
        setRxBaseValue(SVGLength(LengthModeWidth, attr->value()));
        if (rxBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for ellipse <rx> is not allowed");
    } else if (attr->name() == SVGNames::ryAttr) {
        setRyBaseValue(SVGLength(LengthModeHeight, attr->value()));
        if (ryBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for ellipse <ry> is not allowed");
    } else {
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        SVGStyledTransformableElement::parseMappedAttribute(attr);
    }
}

void SVGEllipseElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGStyledTransformableElement::svgAttributeChanged(attrName);

    bool isLengthAttribute = attrName == SVGNames::cxAttr
                          || attrName == SVGNames::cyAttr
                          || attrName == SVGNames::rxAttr
                          || attrName == SVGNames::ryAttr;

    if (isLengthAttribute)
        updateRelativeLengthsInformation();
 
    if (SVGTests::handleAttributeChange(this, attrName))
        return;

    RenderSVGPath* renderer = static_cast<RenderSVGPath*>(this->renderer());
    if (!renderer)
        return;

    if (isLengthAttribute) {
        renderer->setNeedsPathUpdate();
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
        return;
    }

    if (SVGLangSpace::isKnownAttribute(attrName)
        || SVGExternalResourcesRequired::isKnownAttribute(attrName))
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
}

void SVGEllipseElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledTransformableElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeCx();
        synchronizeCy();
        synchronizeRx();
        synchronizeRy();
        synchronizeExternalResourcesRequired();
        SVGTests::synchronizeProperties(this, attrName);
        return;
    }

    if (attrName == SVGNames::cxAttr)
        synchronizeCx();
    else if (attrName == SVGNames::cyAttr)
        synchronizeCy();
    else if (attrName == SVGNames::rxAttr)
        synchronizeRx();
    else if (attrName == SVGNames::ryAttr)
        synchronizeRy();
    else if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
    else if (SVGTests::isKnownAttribute(attrName))
        SVGTests::synchronizeProperties(this, attrName);
}

AttributeToPropertyTypeMap& SVGEllipseElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGEllipseElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGStyledTransformableElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);    
    attributeToPropertyTypeMap.set(SVGNames::cxAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::cyAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::rxAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::ryAttr, AnimatedLength);
}

void SVGEllipseElement::toPathData(Path& path) const
{
    ASSERT(path.isEmpty());

    float radiusX = rx().value(this);
    if (radiusX <= 0)
        return;

    float radiusY = ry().value(this);
    if (radiusY <= 0)
        return;

    path.addEllipse(FloatRect(cx().value(this) - radiusX, cy().value(this) - radiusY, radiusX * 2, radiusY * 2));
}
 
bool SVGEllipseElement::selfHasRelativeLengths() const
{
    return cx().isRelative()
        || cy().isRelative()
        || rx().isRelative()
        || ry().isRelative();
}

}

#endif // ENABLE(SVG)
