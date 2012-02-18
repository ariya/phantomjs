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
#include "SVGLineElement.h"

#include "Attribute.h"
#include "FloatPoint.h"
#include "RenderSVGPath.h"
#include "RenderSVGResource.h"
#include "SVGLength.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGLineElement, SVGNames::x1Attr, X1, x1)
DEFINE_ANIMATED_LENGTH(SVGLineElement, SVGNames::y1Attr, Y1, y1)
DEFINE_ANIMATED_LENGTH(SVGLineElement, SVGNames::x2Attr, X2, x2)
DEFINE_ANIMATED_LENGTH(SVGLineElement, SVGNames::y2Attr, Y2, y2)
DEFINE_ANIMATED_BOOLEAN(SVGLineElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

inline SVGLineElement::SVGLineElement(const QualifiedName& tagName, Document* document)
    : SVGStyledTransformableElement(tagName, document)
    , m_x1(LengthModeWidth)
    , m_y1(LengthModeHeight)
    , m_x2(LengthModeWidth)
    , m_y2(LengthModeHeight)
{
}

PassRefPtr<SVGLineElement> SVGLineElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGLineElement(tagName, document));
}

void SVGLineElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::x1Attr)
        setX1BaseValue(SVGLength(LengthModeWidth, attr->value()));
    else if (attr->name() == SVGNames::y1Attr)
        setY1BaseValue(SVGLength(LengthModeHeight, attr->value()));
    else if (attr->name() == SVGNames::x2Attr)
        setX2BaseValue(SVGLength(LengthModeWidth, attr->value()));
    else if (attr->name() == SVGNames::y2Attr)
        setY2BaseValue(SVGLength(LengthModeHeight, attr->value()));
    else {
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        SVGStyledTransformableElement::parseMappedAttribute(attr);
    }
}

void SVGLineElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGStyledTransformableElement::svgAttributeChanged(attrName);

    bool isLengthAttribute = attrName == SVGNames::x1Attr
                          || attrName == SVGNames::y1Attr
                          || attrName == SVGNames::x2Attr
                          || attrName == SVGNames::y2Attr;

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

void SVGLineElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledTransformableElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeX1();
        synchronizeY1();
        synchronizeX2();
        synchronizeY2();
        synchronizeExternalResourcesRequired();
        SVGTests::synchronizeProperties(this, attrName);
        return;
    }

    if (attrName == SVGNames::x1Attr)
        synchronizeX1();
    else if (attrName == SVGNames::y1Attr)
        synchronizeY1();
    else if (attrName == SVGNames::x2Attr)
        synchronizeX2();
    else if (attrName == SVGNames::y2Attr)
        synchronizeY2();
    else if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
    else if (SVGTests::isKnownAttribute(attrName))
        SVGTests::synchronizeProperties(this, attrName);
}

AttributeToPropertyTypeMap& SVGLineElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGLineElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGStyledTransformableElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::x1Attr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::y1Attr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::x2Attr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::y2Attr, AnimatedLength);
}

void SVGLineElement::toPathData(Path& path) const
{
    ASSERT(path.isEmpty());

    path.moveTo(FloatPoint(x1().value(this), y1().value(this)));
    path.addLineTo(FloatPoint(x2().value(this), y2().value(this)));
}

bool SVGLineElement::selfHasRelativeLengths() const
{
    return x1().isRelative()
        || y1().isRelative()
        || x2().isRelative()
        || y2().isRelative();
}

}

#endif // ENABLE(SVG)
