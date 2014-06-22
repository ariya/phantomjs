/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Rob Buis <buis@kde.org>
 * Copyright (C) 2006 Alexander Kellett <lypanov@kde.org>
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
#include "SVGImageElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "RenderImageResource.h"
#include "RenderSVGImage.h"
#include "RenderSVGResource.h"
#include "SVGElementInstance.h"
#include "SVGNames.h"
#include "SVGSVGElement.h"
#include "XLinkNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGImageElement, SVGNames::xAttr, X, x)
DEFINE_ANIMATED_LENGTH(SVGImageElement, SVGNames::yAttr, Y, y)
DEFINE_ANIMATED_LENGTH(SVGImageElement, SVGNames::widthAttr, Width, width)
DEFINE_ANIMATED_LENGTH(SVGImageElement, SVGNames::heightAttr, Height, height)
DEFINE_ANIMATED_PRESERVEASPECTRATIO(SVGImageElement, SVGNames::preserveAspectRatioAttr, PreserveAspectRatio, preserveAspectRatio)
DEFINE_ANIMATED_STRING(SVGImageElement, XLinkNames::hrefAttr, Href, href)
DEFINE_ANIMATED_BOOLEAN(SVGImageElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGImageElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(x)
    REGISTER_LOCAL_ANIMATED_PROPERTY(y)
    REGISTER_LOCAL_ANIMATED_PROPERTY(width)
    REGISTER_LOCAL_ANIMATED_PROPERTY(height)
    REGISTER_LOCAL_ANIMATED_PROPERTY(preserveAspectRatio)
    REGISTER_LOCAL_ANIMATED_PROPERTY(href)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGGraphicsElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGImageElement::SVGImageElement(const QualifiedName& tagName, Document* document)
    : SVGGraphicsElement(tagName, document)
    , m_x(LengthModeWidth)
    , m_y(LengthModeHeight)
    , m_width(LengthModeWidth)
    , m_height(LengthModeHeight)
    , m_imageLoader(this)
{
    ASSERT(isSVGImageElement(this));
    registerAnimatedPropertiesForSVGImageElement();
}

PassRefPtr<SVGImageElement> SVGImageElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGImageElement(tagName, document));
}

bool SVGImageElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        SVGLangSpace::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        SVGURIReference::addSupportedAttributes(supportedAttributes);
        supportedAttributes.add(SVGNames::xAttr);
        supportedAttributes.add(SVGNames::yAttr);
        supportedAttributes.add(SVGNames::widthAttr);
        supportedAttributes.add(SVGNames::heightAttr);
        supportedAttributes.add(SVGNames::preserveAspectRatioAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

bool SVGImageElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == SVGNames::widthAttr || name == SVGNames::heightAttr)
        return true;
    return SVGGraphicsElement::isPresentationAttribute(name);
}

void SVGImageElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStylePropertySet* style)
{
    if (!isSupportedAttribute(name))
        SVGGraphicsElement::collectStyleForPresentationAttribute(name, value, style);
    else if (name == SVGNames::widthAttr)
        addPropertyToPresentationAttributeStyle(style, CSSPropertyWidth, value);
    else if (name == SVGNames::heightAttr)
        addPropertyToPresentationAttributeStyle(style, CSSPropertyHeight, value);
}

void SVGImageElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    SVGParsingError parseError = NoError;

    if (!isSupportedAttribute(name))
        SVGGraphicsElement::parseAttribute(name, value);
    else if (name == SVGNames::xAttr)
        setXBaseValue(SVGLength::construct(LengthModeWidth, value, parseError));
    else if (name == SVGNames::yAttr)
        setYBaseValue(SVGLength::construct(LengthModeHeight, value, parseError));
    else if (name == SVGNames::preserveAspectRatioAttr) {
        SVGPreserveAspectRatio preserveAspectRatio;
        preserveAspectRatio.parse(value);
        setPreserveAspectRatioBaseValue(preserveAspectRatio);
    } else if (name == SVGNames::widthAttr)
        setWidthBaseValue(SVGLength::construct(LengthModeWidth, value, parseError, ForbidNegativeLengths));
    else if (name == SVGNames::heightAttr)
        setHeightBaseValue(SVGLength::construct(LengthModeHeight, value, parseError, ForbidNegativeLengths));
    else if (SVGLangSpace::parseAttribute(name, value)
             || SVGExternalResourcesRequired::parseAttribute(name, value)
             || SVGURIReference::parseAttribute(name, value)) {
    } else
        ASSERT_NOT_REACHED();

    reportAttributeParsingError(parseError, name, value);
}

void SVGImageElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGGraphicsElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);
    
    bool isLengthAttribute = attrName == SVGNames::xAttr
                          || attrName == SVGNames::yAttr
                          || attrName == SVGNames::widthAttr
                          || attrName == SVGNames::heightAttr;

    if (isLengthAttribute)
        updateRelativeLengthsInformation();

    if (SVGURIReference::isKnownAttribute(attrName)) {
        m_imageLoader.updateFromElementIgnoringPreviousError();
        return;
    }

    RenderObject* renderer = this->renderer();
    if (!renderer)
        return;

    if (isLengthAttribute) {
        if (toRenderSVGImage(renderer)->updateImageViewport())
            RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
        return;
    }

    if (attrName == SVGNames::preserveAspectRatioAttr
        || SVGLangSpace::isKnownAttribute(attrName)
        || SVGExternalResourcesRequired::isKnownAttribute(attrName)) {
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
        return;
    }

    ASSERT_NOT_REACHED();
}

bool SVGImageElement::selfHasRelativeLengths() const
{
    return x().isRelative()
        || y().isRelative()
        || width().isRelative()
        || height().isRelative();
}

RenderObject* SVGImageElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGImage(this);
}

bool SVGImageElement::haveLoadedRequiredResources()
{
    return !externalResourcesRequiredBaseValue() || !m_imageLoader.hasPendingActivity();
}

void SVGImageElement::attach(const AttachContext& context)
{
    SVGGraphicsElement::attach(context);

    if (RenderSVGImage* imageObj = toRenderSVGImage(renderer())) {
        if (imageObj->imageResource()->hasImage())
            return;

        imageObj->imageResource()->setCachedImage(m_imageLoader.image());
    }
}

Node::InsertionNotificationRequest SVGImageElement::insertedInto(ContainerNode* rootParent)
{
    SVGGraphicsElement::insertedInto(rootParent);
    if (!rootParent->inDocument())
        return InsertionDone;
    // Update image loader, as soon as we're living in the tree.
    // We can only resolve base URIs properly, after that!
    m_imageLoader.updateFromElement();
    return InsertionDone;
}

const AtomicString& SVGImageElement::imageSourceURL() const
{
    return getAttribute(XLinkNames::hrefAttr);
}

void SVGImageElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    SVGGraphicsElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, document()->completeURL(href()));
}

void SVGImageElement::didMoveToNewDocument(Document* oldDocument)
{
    m_imageLoader.elementDidMoveToNewDocument();
    SVGGraphicsElement::didMoveToNewDocument(oldDocument);
}

}

#endif // ENABLE(SVG)
