/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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
#include "SVGGElement.h"

#include "RenderSVGHiddenContainer.h"
#include "RenderSVGResource.h"
#include "RenderSVGTransformableContainer.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property declarations
DEFINE_ANIMATED_BOOLEAN(SVGGElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

SVGGElement::SVGGElement(const QualifiedName& tagName, Document* document)
    : SVGStyledTransformableElement(tagName, document)
{
}

PassRefPtr<SVGGElement> SVGGElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGGElement(tagName, document));
}

void SVGGElement::parseMappedAttribute(Attribute* attr)
{
    if (SVGTests::parseMappedAttribute(attr))
        return;
    if (SVGLangSpace::parseMappedAttribute(attr))
        return;
    if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
        return;

    SVGStyledTransformableElement::parseMappedAttribute(attr);
}

void SVGGElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGStyledTransformableElement::svgAttributeChanged(attrName);

    if (SVGTests::handleAttributeChange(this, attrName))
        return;

    RenderObject* renderer = this->renderer();
    if (!renderer)
        return;

    if (SVGLangSpace::isKnownAttribute(attrName)
        || SVGExternalResourcesRequired::isKnownAttribute(attrName))
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
}

void SVGGElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledTransformableElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeExternalResourcesRequired();
        SVGTests::synchronizeProperties(this, attrName);
        return;
    }

    if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
    else if (SVGTests::isKnownAttribute(attrName))
        SVGTests::synchronizeProperties(this, attrName);
}

AttributeToPropertyTypeMap& SVGGElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGGElement::fillAttributeToPropertyTypeMap()
{
    SVGStyledTransformableElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap());
}

RenderObject* SVGGElement::createRenderer(RenderArena* arena, RenderStyle* style)
{
    // SVG 1.1 testsuite explicitely uses constructs like <g display="none"><linearGradient>
    // We still have to create renderers for the <g> & <linearGradient> element, though the
    // subtree may be hidden - we only want the resource renderers to exist so they can be
    // referenced from somewhere else.
    if (style->display() == NONE)
        return new (arena) RenderSVGHiddenContainer(this);

    return new (arena) RenderSVGTransformableContainer(this);
}

bool SVGGElement::rendererIsNeeded(RenderStyle*)
{
    return parentNode() && parentNode()->isSVGElement(); 
}

}

#endif // ENABLE(SVG)
