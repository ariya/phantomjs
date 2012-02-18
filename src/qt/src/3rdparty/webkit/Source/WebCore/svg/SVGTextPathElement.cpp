/*
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2010 Rob Buis <rwlbuis@gmail.com>
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
#include "SVGTextPathElement.h"

#include "Attribute.h"
#include "RenderSVGResource.h"
#include "RenderSVGTextPath.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGTextPathElement, SVGNames::startOffsetAttr, StartOffset, startOffset)
DEFINE_ANIMATED_ENUMERATION(SVGTextPathElement, SVGNames::methodAttr, Method, method)
DEFINE_ANIMATED_ENUMERATION(SVGTextPathElement, SVGNames::spacingAttr, Spacing, spacing)
DEFINE_ANIMATED_STRING(SVGTextPathElement, XLinkNames::hrefAttr, Href, href)

inline SVGTextPathElement::SVGTextPathElement(const QualifiedName& tagName, Document* document)
    : SVGTextContentElement(tagName, document)
    , m_startOffset(LengthModeOther)
    , m_method(SVG_TEXTPATH_METHODTYPE_ALIGN)
    , m_spacing(SVG_TEXTPATH_SPACINGTYPE_EXACT)
{
}

PassRefPtr<SVGTextPathElement> SVGTextPathElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGTextPathElement(tagName, document));
}

void SVGTextPathElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();

    if (attr->name() == SVGNames::startOffsetAttr)
        setStartOffsetBaseValue(SVGLength(LengthModeOther, value));
    else if (attr->name() == SVGNames::methodAttr) {
        if (value == "align")
            setSpacingBaseValue(SVG_TEXTPATH_METHODTYPE_ALIGN);
        else if (value == "stretch")
            setSpacingBaseValue(SVG_TEXTPATH_METHODTYPE_STRETCH);
    } else if (attr->name() == SVGNames::spacingAttr) {
        if (value == "auto")
            setMethodBaseValue(SVG_TEXTPATH_SPACINGTYPE_AUTO);
        else if (value == "exact")
            setMethodBaseValue(SVG_TEXTPATH_SPACINGTYPE_EXACT);
    } else {
        if (SVGURIReference::parseMappedAttribute(attr))
            return;
        SVGTextContentElement::parseMappedAttribute(attr);
    }
}

void SVGTextPathElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGTextContentElement::svgAttributeChanged(attrName);

    if (attrName == SVGNames::startOffsetAttr)
        updateRelativeLengthsInformation();

    if (!renderer())
        return;

    if (attrName == SVGNames::startOffsetAttr
        || SVGURIReference::isKnownAttribute(attrName))
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer());
}

void SVGTextPathElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGTextContentElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeStartOffset();
        synchronizeMethod();
        synchronizeSpacing();
        synchronizeHref();
        return;
    }

    if (attrName == SVGNames::startOffsetAttr)
        synchronizeStartOffset();
    else if (attrName == SVGNames::methodAttr)
        synchronizeMethod();
    else if (attrName == SVGNames::spacingAttr)
        synchronizeSpacing();
    else if (SVGURIReference::isKnownAttribute(attrName))
        synchronizeHref();
}

AttributeToPropertyTypeMap& SVGTextPathElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGTextPathElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGTextContentElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::startOffsetAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::methodAttr, AnimatedEnumeration);
    attributeToPropertyTypeMap.set(SVGNames::spacingAttr, AnimatedEnumeration);
    attributeToPropertyTypeMap.set(XLinkNames::hrefAttr, AnimatedString);
}

RenderObject* SVGTextPathElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGTextPath(this);
}

bool SVGTextPathElement::childShouldCreateRenderer(Node* child) const
{
    if (child->isTextNode()
        || child->hasTagName(SVGNames::aTag)
        || child->hasTagName(SVGNames::trefTag)
        || child->hasTagName(SVGNames::tspanTag))
        return true;

    return false;
}

bool SVGTextPathElement::rendererIsNeeded(RenderStyle* style)
{
    if (parentNode()
        && (parentNode()->hasTagName(SVGNames::aTag)
            || parentNode()->hasTagName(SVGNames::textTag)))
        return StyledElement::rendererIsNeeded(style);

    return false;
}

void SVGTextPathElement::insertedIntoDocument()
{
    SVGTextContentElement::insertedIntoDocument();

    String id = SVGURIReference::getTarget(href());
    Element* targetElement = treeScope()->getElementById(id);
    if (!targetElement) {
        document()->accessSVGExtensions()->addPendingResource(id, this);
        return;
    }
}

bool SVGTextPathElement::selfHasRelativeLengths() const
{
    return startOffset().isRelative()
        || SVGTextContentElement::selfHasRelativeLengths();
}

}

#endif // ENABLE(SVG)
