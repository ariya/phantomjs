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
#include "SVGElementInstance.h"
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

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGTextPositioningElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(x)
    REGISTER_LOCAL_ANIMATED_PROPERTY(y)
    REGISTER_LOCAL_ANIMATED_PROPERTY(dx)
    REGISTER_LOCAL_ANIMATED_PROPERTY(dy)
    REGISTER_LOCAL_ANIMATED_PROPERTY(rotate)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGTextContentElement)
END_REGISTER_ANIMATED_PROPERTIES

SVGTextPositioningElement::SVGTextPositioningElement(const QualifiedName& tagName, Document* document)
    : SVGTextContentElement(tagName, document)
{
    registerAnimatedPropertiesForSVGTextPositioningElement();
}

bool SVGTextPositioningElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        supportedAttributes.add(SVGNames::xAttr);
        supportedAttributes.add(SVGNames::yAttr);
        supportedAttributes.add(SVGNames::dxAttr);
        supportedAttributes.add(SVGNames::dyAttr);
        supportedAttributes.add(SVGNames::rotateAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGTextPositioningElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGTextContentElement::parseAttribute(name, value);
        return;
    }

    if (name == SVGNames::xAttr) {
        SVGLengthList newList;
        newList.parse(value, LengthModeWidth);
        detachAnimatedXListWrappers(newList.size());
        setXBaseValue(newList);
        return;
    }

    if (name == SVGNames::yAttr) {
        SVGLengthList newList;
        newList.parse(value, LengthModeHeight);
        detachAnimatedYListWrappers(newList.size());
        setYBaseValue(newList);
        return;
    }

    if (name == SVGNames::dxAttr) {
        SVGLengthList newList;
        newList.parse(value, LengthModeWidth);
        detachAnimatedDxListWrappers(newList.size());
        setDxBaseValue(newList);
        return;
    }

    if (name == SVGNames::dyAttr) {
        SVGLengthList newList;
        newList.parse(value, LengthModeHeight);
        detachAnimatedDyListWrappers(newList.size());
        setDyBaseValue(newList);
        return;
    }

    if (name == SVGNames::rotateAttr) {
        SVGNumberList newList;
        newList.parse(value);
        detachAnimatedRotateListWrappers(newList.size());
        setRotateBaseValue(newList);
        return;
    }

    ASSERT_NOT_REACHED();
}

void SVGTextPositioningElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGTextContentElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);

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

    ASSERT_NOT_REACHED();
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
