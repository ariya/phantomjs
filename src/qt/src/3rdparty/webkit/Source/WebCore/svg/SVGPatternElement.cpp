/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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

#if ENABLE(SVG)
#include "SVGPatternElement.h"

#include "AffineTransform.h"
#include "Attribute.h"
#include "Document.h"
#include "FloatConversion.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "PatternAttributes.h"
#include "RenderSVGContainer.h"
#include "RenderSVGResourcePattern.h"
#include "SVGNames.h"
#include "SVGRenderSupport.h"
#include "SVGSVGElement.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTransformable.h"
#include "SVGUnitTypes.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGPatternElement, SVGNames::xAttr, X, x)
DEFINE_ANIMATED_LENGTH(SVGPatternElement, SVGNames::yAttr, Y, y)
DEFINE_ANIMATED_LENGTH(SVGPatternElement, SVGNames::widthAttr, Width, width)
DEFINE_ANIMATED_LENGTH(SVGPatternElement, SVGNames::heightAttr, Height, height)
DEFINE_ANIMATED_ENUMERATION(SVGPatternElement, SVGNames::patternUnitsAttr, PatternUnits, patternUnits)
DEFINE_ANIMATED_ENUMERATION(SVGPatternElement, SVGNames::patternContentUnitsAttr, PatternContentUnits, patternContentUnits)
DEFINE_ANIMATED_TRANSFORM_LIST(SVGPatternElement, SVGNames::patternTransformAttr, PatternTransform, patternTransform) 
DEFINE_ANIMATED_STRING(SVGPatternElement, XLinkNames::hrefAttr, Href, href)
DEFINE_ANIMATED_BOOLEAN(SVGPatternElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)
DEFINE_ANIMATED_RECT(SVGPatternElement, SVGNames::viewBoxAttr, ViewBox, viewBox)
DEFINE_ANIMATED_PRESERVEASPECTRATIO(SVGPatternElement, SVGNames::preserveAspectRatioAttr, PreserveAspectRatio, preserveAspectRatio) 

inline SVGPatternElement::SVGPatternElement(const QualifiedName& tagName, Document* document)
    : SVGStyledElement(tagName, document)
    , m_x(LengthModeWidth)
    , m_y(LengthModeHeight)
    , m_width(LengthModeWidth)
    , m_height(LengthModeHeight)
    , m_patternUnits(SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX)
    , m_patternContentUnits(SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE)
{
}

PassRefPtr<SVGPatternElement> SVGPatternElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGPatternElement(tagName, document));
}

void SVGPatternElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::patternUnitsAttr) {
        if (attr->value() == "userSpaceOnUse")
            setPatternUnitsBaseValue(SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE);
        else if (attr->value() == "objectBoundingBox")
            setPatternUnitsBaseValue(SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
    } else if (attr->name() == SVGNames::patternContentUnitsAttr) {
        if (attr->value() == "userSpaceOnUse")
            setPatternContentUnitsBaseValue(SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE);
        else if (attr->value() == "objectBoundingBox")
            setPatternContentUnitsBaseValue(SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
    } else if (attr->name() == SVGNames::patternTransformAttr) {
        SVGTransformList newList;
        if (!SVGTransformable::parseTransformAttribute(newList, attr->value()))
            newList.clear();

        detachAnimatedPatternTransformListWrappers(newList.size());
        setPatternTransformBaseValue(newList);
    } else if (attr->name() == SVGNames::xAttr)
        setXBaseValue(SVGLength(LengthModeWidth, attr->value()));
    else if (attr->name() == SVGNames::yAttr)
        setYBaseValue(SVGLength(LengthModeHeight, attr->value()));
    else if (attr->name() == SVGNames::widthAttr) {
        setWidthBaseValue(SVGLength(LengthModeWidth, attr->value()));
        if (widthBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for pattern attribute <width> is not allowed");
    } else if (attr->name() == SVGNames::heightAttr) {
        setHeightBaseValue(SVGLength(LengthModeHeight, attr->value()));
        if (heightBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for pattern attribute <height> is not allowed");
    } else {
        if (SVGURIReference::parseMappedAttribute(attr))
            return;
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        if (SVGFitToViewBox::parseMappedAttribute(document(), attr))
            return;

        SVGStyledElement::parseMappedAttribute(attr);
    }
}

void SVGPatternElement::svgAttributeChanged(const QualifiedName& attrName)
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
        || attrName == SVGNames::patternUnitsAttr
        || attrName == SVGNames::patternContentUnitsAttr
        || attrName == SVGNames::patternTransformAttr
        || SVGURIReference::isKnownAttribute(attrName)
        || SVGTests::isKnownAttribute(attrName)
        || SVGLangSpace::isKnownAttribute(attrName)
        || SVGExternalResourcesRequired::isKnownAttribute(attrName)
        || SVGFitToViewBox::isKnownAttribute(attrName)
        || SVGStyledElement::isKnownAttribute(attrName))
        object->setNeedsLayout(true);
}

void SVGPatternElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizePatternUnits();
        synchronizePatternContentUnits();
        synchronizePatternTransform();
        synchronizeX();
        synchronizeY();
        synchronizeWidth();
        synchronizeHeight();
        synchronizeExternalResourcesRequired();
        synchronizeViewBox();
        synchronizePreserveAspectRatio();
        synchronizeHref();
        SVGTests::synchronizeProperties(this, attrName);
        return;
    }

    if (attrName == SVGNames::patternUnitsAttr)
        synchronizePatternUnits();
    else if (attrName == SVGNames::patternContentUnitsAttr)
        synchronizePatternContentUnits();
    else if (attrName == SVGNames::patternTransformAttr)
        synchronizePatternTransform();
    else if (attrName == SVGNames::xAttr)
        synchronizeX();
    else if (attrName == SVGNames::yAttr)
        synchronizeY();
    else if (attrName == SVGNames::widthAttr)
        synchronizeWidth();
    else if (attrName == SVGNames::heightAttr)
        synchronizeHeight();
    else if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
    else if (attrName == SVGNames::viewBoxAttr)
        synchronizeViewBox();
    else if (attrName == SVGNames::preserveAspectRatioAttr)
        synchronizePreserveAspectRatio();
    else if (SVGURIReference::isKnownAttribute(attrName))
        synchronizeHref();
    else if (SVGTests::isKnownAttribute(attrName))
        SVGTests::synchronizeProperties(this, attrName);
}

AttributeToPropertyTypeMap& SVGPatternElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGPatternElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGStyledElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::xAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::yAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::widthAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::heightAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::patternUnitsAttr, AnimatedEnumeration);
    attributeToPropertyTypeMap.set(SVGNames::patternContentUnitsAttr, AnimatedEnumeration);
    attributeToPropertyTypeMap.set(SVGNames::patternTransformAttr, AnimatedTransformList);
    attributeToPropertyTypeMap.set(XLinkNames::hrefAttr, AnimatedString);
    attributeToPropertyTypeMap.set(SVGNames::viewBoxAttr, AnimatedRect);
    attributeToPropertyTypeMap.set(SVGNames::preserveAspectRatioAttr, AnimatedPreserveAspectRatio);
}

void SVGPatternElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    SVGStyledElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);

    if (changedByParser)
        return;

    if (RenderObject* object = renderer())
        object->setNeedsLayout(true);
}

RenderObject* SVGPatternElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGResourcePattern(this);
}

void SVGPatternElement::collectPatternAttributes(PatternAttributes& attributes) const
{
    HashSet<const SVGPatternElement*> processedPatterns;

    const SVGPatternElement* current = this;
    while (current) {
        if (!attributes.hasX() && current->hasAttribute(SVGNames::xAttr))
            attributes.setX(current->x());

        if (!attributes.hasY() && current->hasAttribute(SVGNames::yAttr))
            attributes.setY(current->y());

        if (!attributes.hasWidth() && current->hasAttribute(SVGNames::widthAttr))
            attributes.setWidth(current->width());

        if (!attributes.hasHeight() && current->hasAttribute(SVGNames::heightAttr))
            attributes.setHeight(current->height());

        if (!attributes.hasViewBox() && current->hasAttribute(SVGNames::viewBoxAttr))
            attributes.setViewBox(current->viewBox());

        if (!attributes.hasPreserveAspectRatio() && current->hasAttribute(SVGNames::preserveAspectRatioAttr))
            attributes.setPreserveAspectRatio(current->preserveAspectRatio());

        if (!attributes.hasBoundingBoxMode() && current->hasAttribute(SVGNames::patternUnitsAttr))
            attributes.setBoundingBoxMode(current->patternUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);

        if (!attributes.hasBoundingBoxModeContent() && current->hasAttribute(SVGNames::patternContentUnitsAttr))
            attributes.setBoundingBoxModeContent(current->patternContentUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);

        if (!attributes.hasPatternTransform() && current->hasAttribute(SVGNames::patternTransformAttr)) {
            AffineTransform transform;
            current->patternTransform().concatenate(transform);
            attributes.setPatternTransform(transform);
        }

        if (!attributes.hasPatternContentElement() && current->hasChildNodes())
            attributes.setPatternContentElement(current);

        processedPatterns.add(current);

        // Respect xlink:href, take attributes from referenced element
        Node* refNode = treeScope()->getElementById(SVGURIReference::getTarget(current->href()));
        if (refNode && refNode->hasTagName(SVGNames::patternTag)) {
            current = static_cast<const SVGPatternElement*>(const_cast<const Node*>(refNode));

            // Cycle detection
            if (processedPatterns.contains(current)) {
                current = 0;
                break;
            }
        } else
            current = 0;
    }
}

bool SVGPatternElement::selfHasRelativeLengths() const
{
    return x().isRelative()
        || y().isRelative()
        || width().isRelative()
        || height().isRelative();
}

}

#endif // ENABLE(SVG)
