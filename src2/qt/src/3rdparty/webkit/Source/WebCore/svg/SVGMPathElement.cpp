/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
#include "SVGMPathElement.h"

#include "Document.h"
#include "SVGNames.h"
#include "SVGPathElement.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGMPathElement, XLinkNames::hrefAttr, Href, href)
DEFINE_ANIMATED_BOOLEAN(SVGMPathElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

inline SVGMPathElement::SVGMPathElement(const QualifiedName& tagName, Document* document)
    : SVGElement(tagName, document)
{
}

PassRefPtr<SVGMPathElement> SVGMPathElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGMPathElement(tagName, document));
}

void SVGMPathElement::parseMappedAttribute(Attribute* attr)
{
    if (SVGURIReference::parseMappedAttribute(attr))
        return;
    SVGElement::parseMappedAttribute(attr);
}

void SVGMPathElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeExternalResourcesRequired();
        synchronizeHref();
        return;
    }

    if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
    else if (SVGURIReference::isKnownAttribute(attrName))
        synchronizeHref();
}

AttributeToPropertyTypeMap& SVGMPathElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGMPathElement::fillAttributeToPropertyTypeMap()
{
    attributeToPropertyTypeMap().set(XLinkNames::hrefAttr, AnimatedString);
}

SVGPathElement* SVGMPathElement::pathElement()
{
    Element* target = treeScope()->getElementById(getTarget(href()));
    if (target && target->hasTagName(SVGNames::pathTag))
        return static_cast<SVGPathElement*>(target);
    return 0;
}

} // namespace WebCore

#endif // ENABLE(SVG)
