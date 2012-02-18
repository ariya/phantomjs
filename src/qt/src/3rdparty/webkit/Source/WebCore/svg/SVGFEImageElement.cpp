/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2010 Dirk Schulze <krit@webkit.org>
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

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFEImageElement.h"

#include "Attr.h"
#include "CachedImage.h"
#include "CachedResourceLoader.h"
#include "ColorSpace.h"
#include "Document.h"
#include "Image.h"
#include "RenderObject.h"
#include "RenderSVGResource.h"
#include "SVGImageBufferTools.h"
#include "SVGNames.h"
#include "SVGPreserveAspectRatio.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_PRESERVEASPECTRATIO(SVGFEImageElement, SVGNames::preserveAspectRatioAttr, PreserveAspectRatio, preserveAspectRatio)
DEFINE_ANIMATED_STRING(SVGFEImageElement, XLinkNames::hrefAttr, Href, href)
DEFINE_ANIMATED_BOOLEAN(SVGFEImageElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

inline SVGFEImageElement::SVGFEImageElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
{
}

PassRefPtr<SVGFEImageElement> SVGFEImageElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFEImageElement(tagName, document));
}

SVGFEImageElement::~SVGFEImageElement()
{
    if (m_cachedImage)
        m_cachedImage->removeClient(this);
}

void SVGFEImageElement::requestImageResource()
{
    if (m_cachedImage) {
        m_cachedImage->removeClient(this);
        m_cachedImage = 0;
    }

    Element* hrefElement = treeScope()->getElementById(SVGURIReference::getTarget(href()));
    if (hrefElement && hrefElement->isSVGElement() && hrefElement->renderer())
        return;

    m_cachedImage = ownerDocument()->cachedResourceLoader()->requestImage(href());

    if (m_cachedImage)
        m_cachedImage->addClient(this);
}

void SVGFEImageElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::preserveAspectRatioAttr)
        SVGPreserveAspectRatio::parsePreserveAspectRatio(this, value);
    else {
        if (SVGURIReference::parseMappedAttribute(attr)) {
            requestImageResource();
            return;
        }
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;

        SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(attr);
    }
}

void SVGFEImageElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);

    if (attrName == SVGNames::preserveAspectRatioAttr)
        invalidate();
}

void SVGFEImageElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizePreserveAspectRatio();
        synchronizeHref();
        synchronizeExternalResourcesRequired();
        return;
    }

    if (attrName == SVGNames::preserveAspectRatioAttr)
        synchronizePreserveAspectRatio();
    else if (SVGURIReference::isKnownAttribute(attrName))
        synchronizeHref();
    else if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
}

AttributeToPropertyTypeMap& SVGFEImageElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGFEImageElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGFilterPrimitiveStandardAttributes::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::preserveAspectRatioAttr, AnimatedPreserveAspectRatio);
    attributeToPropertyTypeMap.set(XLinkNames::hrefAttr, AnimatedString);
}

void SVGFEImageElement::notifyFinished(CachedResource*)
{
    if (!inDocument())
        return;

    Element* parent = parentElement();
    ASSERT(parent);

    if (!parent->hasTagName(SVGNames::filterTag) || !parent->renderer())
        return;

    RenderSVGResource::markForLayoutAndParentResourceInvalidation(parent->renderer());
}

PassRefPtr<FilterEffect> SVGFEImageElement::build(SVGFilterBuilder*, Filter* filter)
{
    if (!m_cachedImage && !m_targetImage) {
        Element* hrefElement = treeScope()->getElementById(SVGURIReference::getTarget(href()));
        if (!hrefElement || !hrefElement->isSVGElement())
            return 0;

        RenderObject* renderer = hrefElement->renderer();
        if (!renderer)
            return 0;

        IntRect targetRect = enclosingIntRect(renderer->objectBoundingBox());
        m_targetImage = ImageBuffer::create(targetRect.size(), ColorSpaceLinearRGB);

        AffineTransform contentTransformation;
        SVGImageBufferTools::renderSubtreeToImageBuffer(m_targetImage.get(), renderer, contentTransformation);
    }

    return FEImage::create(filter, m_targetImage ? m_targetImage->copyImage() : m_cachedImage->image(), preserveAspectRatio());
}

void SVGFEImageElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    SVGFilterPrimitiveStandardAttributes::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, document()->completeURL(href()));
}

}

#endif // ENABLE(SVG)
