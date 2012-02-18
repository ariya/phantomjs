/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Rob Buis <buis@kde.org>
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
#include "SVGTextPositioningElement.h"

#include "Attribute.h"
#include "RenderSVGResource.h"
#include "RenderSVGText.h"
#include "SVGLengthList.h"
#include "SVGNames.h"
#include "SVGNumberList.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH_LIST(SVGTextPositioningElement, SVGNames::xAttr, X, x)
DEFINE_ANIMATED_LENGTH_LIST(SVGTextPositioningElement, SVGNames::yAttr, Y, y)
DEFINE_ANIMATED_LENGTH_LIST(SVGTextPositioningElement, SVGNames::dxAttr, Dx, dx)
DEFINE_ANIMATED_LENGTH_LIST(SVGTextPositioningElement, SVGNames::dyAttr, Dy, dy)
DEFINE_ANIMATED_NUMBER_LIST(SVGTextPositioningElement, SVGNames::rotateAttr, Rotate, rotate)

SVGTextPositioningElement::SVGTextPositioningElement(const QualifiedName& tagName, Document* document)
    : SVGTextContentElement(tagName, document)
{
}

void SVGTextPositioningElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::xAttr) {
        SVGLengthList newList;
        newList.parse(attr->value(), LengthModeWidth);
        detachAnimatedXListWrappers(newList.size());
        setXBaseValue(newList);
    } else if (attr->name() == SVGNames::yAttr) {
        SVGLengthList newList;
        newList.parse(attr->value(), LengthModeHeight);
        detachAnimatedYListWrappers(newList.size());
        setYBaseValue(newList);
    } else if (attr->name() == SVGNames::dxAttr) {
        SVGLengthList newList;
        newList.parse(attr->value(), LengthModeWidth);
        detachAnimatedDxListWrappers(newList.size());
        setDxBaseValue(newList);
    } else if (attr->name() == SVGNames::dyAttr) {
        SVGLengthList newList;
        newList.parse(attr->value(), LengthModeHeight);
        detachAnimatedDyListWrappers(newList.size());
        setDyBaseValue(newList);
    } else if (attr->name() == SVGNames::rotateAttr) {
        SVGNumberList newList;
        newList.parse(attr->value());
        detachAnimatedRotateListWrappers(newList.size());
        setRotateBaseValue(newList);
    } else
        SVGTextContentElement::parseMappedAttribute(attr);
}

void SVGTextPositioningElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGTextContentElement::svgAttributeChanged(attrName);

    bool updateRelativeLengths = attrName == SVGNames::xAttr
                              || attrName == SVGNames::yAttr
                              || attrName == SVGNames::dxAttr
                              || attrName == SVGNames::dyAttr;

    if (updateRelativeLengths)
        updateRelativeLengthsInformation();

    RenderObject* renderer = this->renderer();
    if (!renderer)
        return;

    if (updateRelativeLengths || attrName == SVGNames::rotateAttr) {
        if (RenderSVGText* textRenderer = RenderSVGText::locateRenderSVGTextAncestor(renderer))
            textRenderer->setNeedsPositioningValuesUpdate();
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
        return;
    }
}

void SVGTextPositioningElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGTextContentElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeX();
        synchronizeY();
        synchronizeDx();
        synchronizeDy();
        synchronizeRotate();
        return;
    }

    if (attrName == SVGNames::xAttr)
        synchronizeX();
    else if (attrName == SVGNames::yAttr)
        synchronizeY();
    else if (attrName == SVGNames::dxAttr)
        synchronizeDx();
    else if (attrName == SVGNames::dyAttr)
        synchronizeDy();
    else if (attrName == SVGNames::rotateAttr)
        synchronizeRotate();
}

void SVGTextPositioningElement::fillPassedAttributeToPropertyTypeMap(AttributeToPropertyTypeMap& attributeToPropertyTypeMap)
{
    SVGTextContentElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);

    attributeToPropertyTypeMap.set(SVGNames::xAttr, AnimatedNumberList);
    attributeToPropertyTypeMap.set(SVGNames::yAttr, AnimatedNumberList);
    attributeToPropertyTypeMap.set(SVGNames::dxAttr, AnimatedNumberList);
    attributeToPropertyTypeMap.set(SVGNames::dyAttr, AnimatedNumberList);
    attributeToPropertyTypeMap.set(SVGNames::rotateAttr, AnimatedNumberList);
}

SVGTextPositioningElement* SVGTextPositioningElement::elementFromRenderer(RenderObject* renderer)
{
    if (!renderer)
        return 0;

    if (!renderer->isSVGText() && !renderer->isSVGInline())
        return 0;

    Node* node = renderer->node();
    ASSERT(node);
    ASSERT(node->isSVGElement());

    if (!node->hasTagName(SVGNames::textTag)
        && !node->hasTagName(SVGNames::tspanTag)
#if ENABLE(SVG_FONTS)
        && !node->hasTagName(SVGNames::altGlyphTag)
#endif
        && !node->hasTagName(SVGNames::trefTag))
        return 0;

    return static_cast<SVGTextPositioningElement*>(node);
}

}

#endif // ENABLE(SVG)
