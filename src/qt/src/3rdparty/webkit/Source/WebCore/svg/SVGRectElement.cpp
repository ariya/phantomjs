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
#include "SVGRectElement.h"

#include "Attribute.h"
#include "RenderSVGPath.h"
#include "RenderSVGResource.h"
#include "SVGLength.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGRectElement, SVGNames::xAttr, X, x)
DEFINE_ANIMATED_LENGTH(SVGRectElement, SVGNames::yAttr, Y, y)
DEFINE_ANIMATED_LENGTH(SVGRectElement, SVGNames::widthAttr, Width, width)
DEFINE_ANIMATED_LENGTH(SVGRectElement, SVGNames::heightAttr, Height, height)
DEFINE_ANIMATED_LENGTH(SVGRectElement, SVGNames::rxAttr, Rx, rx)
DEFINE_ANIMATED_LENGTH(SVGRectElement, SVGNames::ryAttr, Ry, ry)
DEFINE_ANIMATED_BOOLEAN(SVGRectElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

inline SVGRectElement::SVGRectElement(const QualifiedName& tagName, Document* document)
    : SVGStyledTransformableElement(tagName, document)
    , m_x(LengthModeWidth)
    , m_y(LengthModeHeight)
    , m_width(LengthModeWidth)
    , m_height(LengthModeHeight)
    , m_rx(LengthModeWidth)
    , m_ry(LengthModeHeight)
{
}

PassRefPtr<SVGRectElement> SVGRectElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGRectElement(tagName, document));
}

void SVGRectElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::xAttr)
        setXBaseValue(SVGLength(LengthModeWidth, attr->value()));
    else if (attr->name() == SVGNames::yAttr)
        setYBaseValue(SVGLength(LengthModeHeight, attr->value()));
    else if (attr->name() == SVGNames::rxAttr) {
        setRxBaseValue(SVGLength(LengthModeWidth, attr->value()));
        if (rxBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for rect <rx> is not allowed");
    } else if (attr->name() == SVGNames::ryAttr) {
        setRyBaseValue(SVGLength(LengthModeHeight, attr->value()));
        if (ryBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for rect <ry> is not allowed");
    } else if (attr->name() == SVGNames::widthAttr) {
        setWidthBaseValue(SVGLength(LengthModeWidth, attr->value()));
        if (widthBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for rect <width> is not allowed");
    } else if (attr->name() == SVGNames::heightAttr) {
        setHeightBaseValue(SVGLength(LengthModeHeight, attr->value()));
        if (heightBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for rect <height> is not allowed");
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

void SVGRectElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGStyledTransformableElement::svgAttributeChanged(attrName);

    bool isLengthAttribute = attrName == SVGNames::xAttr
                          || attrName == SVGNames::yAttr
                          || attrName == SVGNames::widthAttr
                          || attrName == SVGNames::heightAttr
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

void SVGRectElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledTransformableElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeX();
        synchronizeY();
        synchronizeWidth();
        synchronizeHeight();
        synchronizeRx();
        synchronizeRy();
        synchronizeExternalResourcesRequired();
        SVGTests::synchronizeProperties(this, attrName);
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
    else if (attrName == SVGNames::rxAttr)
        synchronizeRx();
    else if (attrName == SVGNames::ryAttr)
        synchronizeRy();
    else if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
    else if (SVGTests::isKnownAttribute(attrName))
        SVGTests::synchronizeProperties(this, attrName);
}

AttributeToPropertyTypeMap& SVGRectElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGRectElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGStyledTransformableElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::xAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::yAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::widthAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::heightAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::rxAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::ryAttr, AnimatedLength);
}

void SVGRectElement::toPathData(Path& path) const
{
    ASSERT(path.isEmpty());

    float widthValue = width().value(this);
    if (widthValue <= 0)
        return;

    float heightValue = height().value(this);
    if (heightValue <= 0)
        return;

    float xValue = x().value(this);
    float yValue = y().value(this);

    FloatRect rect(xValue, yValue, widthValue, heightValue);

    bool hasRx = hasAttribute(SVGNames::rxAttr);
    bool hasRy = hasAttribute(SVGNames::ryAttr);
    if (hasRx || hasRy) {
        float rxValue = rx().value(this);
        float ryValue = ry().value(this);
        if (!hasRx)
            rxValue = ryValue;
        else if (!hasRy)
            ryValue = rxValue;
        path.addRoundedRect(rect, FloatSize(rxValue, ryValue));
        return;
    }

    path.addRect(rect);
}

bool SVGRectElement::selfHasRelativeLengths() const
{
    return x().isRelative()
        || y().isRelative()
        || width().isRelative()
        || height().isRelative()
        || rx().isRelative()
        || ry().isRelative();
}

}

#endif // ENABLE(SVG)
