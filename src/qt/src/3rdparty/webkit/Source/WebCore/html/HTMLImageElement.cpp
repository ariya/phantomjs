/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "HTMLImageElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "EventNames.h"
#include "FrameView.h"
#include "HTMLDocument.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "RenderImage.h"
#include "ScriptEventListener.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

HTMLImageElement::HTMLImageElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
    : HTMLElement(tagName, document)
    , m_imageLoader(this)
    , ismap(false)
    , m_form(form)
    , m_compositeOperator(CompositeSourceOver)
{
    ASSERT(hasTagName(imgTag));
    if (form)
        form->registerImgElement(this);
}

PassRefPtr<HTMLImageElement> HTMLImageElement::create(Document* document)
{
    return adoptRef(new HTMLImageElement(imgTag, document));
}

PassRefPtr<HTMLImageElement> HTMLImageElement::create(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
{
    return adoptRef(new HTMLImageElement(tagName, document, form));
}

HTMLImageElement::~HTMLImageElement()
{
    if (m_form)
        m_form->removeImgElement(this);
}

PassRefPtr<HTMLImageElement> HTMLImageElement::createForJSConstructor(Document* document, const int* optionalWidth, const int* optionalHeight)
{
    RefPtr<HTMLImageElement> image = adoptRef(new HTMLImageElement(imgTag, document));
    if (optionalWidth)
        image->setWidth(*optionalWidth);
    if (optionalHeight > 0)
        image->setHeight(*optionalHeight);
    return image.release();
}

bool HTMLImageElement::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == widthAttr ||
        attrName == heightAttr ||
        attrName == vspaceAttr ||
        attrName == hspaceAttr ||
        attrName == valignAttr) {
        result = eUniversal;
        return false;
    }

    if (attrName == borderAttr || attrName == alignAttr) {
        result = eReplaced; // Shared with embed and iframe elements.
        return false;
    }

    return HTMLElement::mapToEntry(attrName, result);
}

void HTMLImageElement::parseMappedAttribute(Attribute* attr)
{
    const QualifiedName& attrName = attr->name();
    if (attrName == altAttr) {
        if (renderer() && renderer()->isImage())
            toRenderImage(renderer())->updateAltText();
    } else if (attrName == srcAttr)
        m_imageLoader.updateFromElementIgnoringPreviousError();
    else if (attrName == widthAttr)
        addCSSLength(attr, CSSPropertyWidth, attr->value());
    else if (attrName == heightAttr)
        addCSSLength(attr, CSSPropertyHeight, attr->value());
    else if (attrName == borderAttr) {
        // border="noborder" -> border="0"
        addCSSLength(attr, CSSPropertyBorderWidth, attr->value().toInt() ? attr->value() : "0");
        addCSSProperty(attr, CSSPropertyBorderTopStyle, CSSValueSolid);
        addCSSProperty(attr, CSSPropertyBorderRightStyle, CSSValueSolid);
        addCSSProperty(attr, CSSPropertyBorderBottomStyle, CSSValueSolid);
        addCSSProperty(attr, CSSPropertyBorderLeftStyle, CSSValueSolid);
    } else if (attrName == vspaceAttr) {
        addCSSLength(attr, CSSPropertyMarginTop, attr->value());
        addCSSLength(attr, CSSPropertyMarginBottom, attr->value());
    } else if (attrName == hspaceAttr) {
        addCSSLength(attr, CSSPropertyMarginLeft, attr->value());
        addCSSLength(attr, CSSPropertyMarginRight, attr->value());
    } else if (attrName == alignAttr)
        addHTMLAlignment(attr);
    else if (attrName == valignAttr)
        addCSSProperty(attr, CSSPropertyVerticalAlign, attr->value());
    else if (attrName == usemapAttr) {
        if (attr->value().string()[0] == '#')
            usemap = attr->value();
        else
            usemap = document()->completeURL(stripLeadingAndTrailingHTMLSpaces(attr->value())).string();
        setIsLink(!attr->isNull());
    } else if (attrName == ismapAttr)
        ismap = true;
    else if (attrName == onabortAttr)
        setAttributeEventListener(eventNames().abortEvent, createAttributeEventListener(this, attr));
    else if (attrName == onloadAttr)
        setAttributeEventListener(eventNames().loadEvent, createAttributeEventListener(this, attr));
    else if (attrName == onbeforeloadAttr)
        setAttributeEventListener(eventNames().beforeloadEvent, createAttributeEventListener(this, attr));
    else if (attrName == compositeAttr) {
        if (!parseCompositeOperator(attr->value(), m_compositeOperator))
            m_compositeOperator = CompositeSourceOver;
    } else if (attrName == nameAttr) {
        const AtomicString& newName = attr->value();
        if (inDocument() && document()->isHTMLDocument()) {
            HTMLDocument* document = static_cast<HTMLDocument*>(this->document());
            document->removeNamedItem(m_name);
            document->addNamedItem(newName);
        }
        m_name = newName;
    } else if (isIdAttributeName(attr->name())) {
        const AtomicString& newId = attr->value();
        if (inDocument() && document()->isHTMLDocument()) {
            HTMLDocument* document = static_cast<HTMLDocument*>(this->document());
            document->removeExtraNamedItem(m_id);
            document->addExtraNamedItem(newId);
        }
        m_id = newId;
        // also call superclass
        HTMLElement::parseMappedAttribute(attr);
    } else
        HTMLElement::parseMappedAttribute(attr);
}

String HTMLImageElement::altText() const
{
    // lets figure out the alt text.. magic stuff
    // http://www.w3.org/TR/1998/REC-html40-19980424/appendix/notes.html#altgen
    // also heavily discussed by Hixie on bugzilla
    String alt = getAttribute(altAttr);
    // fall back to title attribute
    if (alt.isNull())
        alt = getAttribute(titleAttr);
    return alt;
}

RenderObject* HTMLImageElement::createRenderer(RenderArena* arena, RenderStyle* style)
{
    if (style->contentData())
        return RenderObject::createObject(this, style);

    RenderImage* image = new (arena) RenderImage(this);
    image->setImageResource(RenderImageResource::create());
    return image;
}

void HTMLImageElement::attach()
{
    HTMLElement::attach();

    if (renderer() && renderer()->isImage() && m_imageLoader.haveFiredBeforeLoadEvent()) {
        RenderImage* renderImage = toRenderImage(renderer());
        RenderImageResource* renderImageResource = renderImage->imageResource();
        if (renderImageResource->hasImage())
            return;
        renderImageResource->setCachedImage(m_imageLoader.image());

        // If we have no image at all because we have no src attribute, set
        // image height and width for the alt text instead.
        if (!m_imageLoader.image() && !renderImageResource->cachedImage())
            renderImage->setImageSizeForAltText();
    }
}

void HTMLImageElement::insertedIntoDocument()
{
    if (document()->isHTMLDocument()) {
        HTMLDocument* document = static_cast<HTMLDocument*>(this->document());
        document->addNamedItem(m_name);
        document->addExtraNamedItem(m_id);
    }

    // If we have been inserted from a renderer-less document,
    // our loader may have not fetched the image, so do it now.
    if (!m_imageLoader.image())
        m_imageLoader.updateFromElement();

    HTMLElement::insertedIntoDocument();
}

void HTMLImageElement::removedFromDocument()
{
    if (document()->isHTMLDocument()) {
        HTMLDocument* document = static_cast<HTMLDocument*>(this->document());
        document->removeNamedItem(m_name);
        document->removeExtraNamedItem(m_id);
    }

    HTMLElement::removedFromDocument();
}

void HTMLImageElement::insertedIntoTree(bool deep)
{
    if (!m_form) {
        // m_form can be non-null if it was set in constructor.
        for (ContainerNode* ancestor = parentNode(); ancestor; ancestor = ancestor->parentNode()) {
            if (ancestor->hasTagName(formTag)) {
                m_form = static_cast<HTMLFormElement*>(ancestor);
                m_form->registerImgElement(this);
                break;
            }
        }
    }

    HTMLElement::insertedIntoTree(deep);
}

void HTMLImageElement::removedFromTree(bool deep)
{
    if (m_form)
        m_form->removeImgElement(this);
    m_form = 0;
    HTMLElement::removedFromTree(deep);
}

int HTMLImageElement::width(bool ignorePendingStylesheets) const
{
    if (!renderer()) {
        // check the attribute first for an explicit pixel value
        bool ok;
        int width = getAttribute(widthAttr).toInt(&ok);
        if (ok)
            return width;

        // if the image is available, use its width
        if (m_imageLoader.image())
            return m_imageLoader.image()->imageSize(1.0f).width();
    }

    if (ignorePendingStylesheets)
        document()->updateLayoutIgnorePendingStylesheets();
    else
        document()->updateLayout();

    RenderBox* box = renderBox();
    return box ? adjustForAbsoluteZoom(box->contentWidth(), box) : 0;
}

int HTMLImageElement::height(bool ignorePendingStylesheets) const
{
    if (!renderer()) {
        // check the attribute first for an explicit pixel value
        bool ok;
        int height = getAttribute(heightAttr).toInt(&ok);
        if (ok)
            return height;

        // if the image is available, use its height
        if (m_imageLoader.image())
            return m_imageLoader.image()->imageSize(1.0f).height();
    }

    if (ignorePendingStylesheets)
        document()->updateLayoutIgnorePendingStylesheets();
    else
        document()->updateLayout();

    RenderBox* box = renderBox();
    return box ? adjustForAbsoluteZoom(box->contentHeight(), box) : 0;
}

int HTMLImageElement::naturalWidth() const
{
    if (!m_imageLoader.image())
        return 0;

    return m_imageLoader.image()->imageSize(1.0f).width();
}

int HTMLImageElement::naturalHeight() const
{
    if (!m_imageLoader.image())
        return 0;

    return m_imageLoader.image()->imageSize(1.0f).height();
}

bool HTMLImageElement::isURLAttribute(Attribute* attr) const
{
    return attr->name() == srcAttr
        || attr->name() == lowsrcAttr
        || attr->name() == longdescAttr
        || (attr->name() == usemapAttr && attr->value().string()[0] != '#');
}

const AtomicString& HTMLImageElement::alt() const
{
    return getAttribute(altAttr);
}

bool HTMLImageElement::draggable() const
{
    // Image elements are draggable by default.
    return !equalIgnoringCase(getAttribute(draggableAttr), "false");
}

void HTMLImageElement::setHeight(int value)
{
    setAttribute(heightAttr, String::number(value));
}

KURL HTMLImageElement::src() const
{
    return document()->completeURL(getAttribute(srcAttr));
}

void HTMLImageElement::setSrc(const String& value)
{
    setAttribute(srcAttr, value);
}

void HTMLImageElement::setWidth(int value)
{
    setAttribute(widthAttr, String::number(value));
}

int HTMLImageElement::x() const
{
    RenderObject* r = renderer();
    if (!r)
        return 0;

    // FIXME: This doesn't work correctly with transforms.
    FloatPoint absPos = r->localToAbsolute();
    return absPos.x();
}

int HTMLImageElement::y() const
{
    RenderObject* r = renderer();
    if (!r)
        return 0;

    // FIXME: This doesn't work correctly with transforms.
    FloatPoint absPos = r->localToAbsolute();
    return absPos.y();
}

bool HTMLImageElement::complete() const
{
    return m_imageLoader.imageComplete();
}

void HTMLImageElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    HTMLElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, src());
    // FIXME: What about when the usemap attribute begins with "#"?
    addSubresourceURL(urls, document()->completeURL(getAttribute(usemapAttr)));
}

void HTMLImageElement::willMoveToNewOwnerDocument()
{
    m_imageLoader.elementWillMoveToNewOwnerDocument();
    HTMLElement::willMoveToNewOwnerDocument();
}

}
