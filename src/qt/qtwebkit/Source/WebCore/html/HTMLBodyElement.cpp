/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann (hausmann@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
#include "HTMLBodyElement.h"

#include "Attribute.h"
#include "CSSImageValue.h"
#include "CSSParser.h"
#include "CSSValueKeywords.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLFrameElementBase.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "Page.h"
#include "ScriptEventListener.h"
#include "StylePropertySet.h"

namespace WebCore {

using namespace HTMLNames;

HTMLBodyElement::HTMLBodyElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(bodyTag));
}

PassRefPtr<HTMLBodyElement> HTMLBodyElement::create(Document* document)
{
    return adoptRef(new HTMLBodyElement(bodyTag, document));
}

PassRefPtr<HTMLBodyElement> HTMLBodyElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLBodyElement(tagName, document));
}

HTMLBodyElement::~HTMLBodyElement()
{
}

bool HTMLBodyElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == backgroundAttr || name == marginwidthAttr || name == leftmarginAttr || name == marginheightAttr || name == topmarginAttr || name == bgcolorAttr || name == textAttr || name == bgpropertiesAttr)
        return true;
    return HTMLElement::isPresentationAttribute(name);
}

void HTMLBodyElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStylePropertySet* style)
{
    if (name == backgroundAttr) {
        String url = stripLeadingAndTrailingHTMLSpaces(value);
        if (!url.isEmpty()) {
            RefPtr<CSSImageValue> imageValue = CSSImageValue::create(document()->completeURL(url).string());
            imageValue->setInitiator(localName());
            style->setProperty(CSSProperty(CSSPropertyBackgroundImage, imageValue.release()));
        }
    } else if (name == marginwidthAttr || name == leftmarginAttr) {
        addHTMLLengthToStyle(style, CSSPropertyMarginRight, value);
        addHTMLLengthToStyle(style, CSSPropertyMarginLeft, value);
    } else if (name == marginheightAttr || name == topmarginAttr) {
        addHTMLLengthToStyle(style, CSSPropertyMarginBottom, value);
        addHTMLLengthToStyle(style, CSSPropertyMarginTop, value);
    } else if (name == bgcolorAttr) {
        addHTMLColorToStyle(style, CSSPropertyBackgroundColor, value);
    } else if (name == textAttr) {
        addHTMLColorToStyle(style, CSSPropertyColor, value);
    } else if (name == bgpropertiesAttr) {
        if (equalIgnoringCase(value, "fixed"))
           addPropertyToPresentationAttributeStyle(style, CSSPropertyBackgroundAttachment, CSSValueFixed);
    } else
        HTMLElement::collectStyleForPresentationAttribute(name, value, style);
}

void HTMLBodyElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == vlinkAttr || name == alinkAttr || name == linkAttr) {
        if (value.isNull()) {
            if (name == linkAttr)
                document()->resetLinkColor();
            else if (name == vlinkAttr)
                document()->resetVisitedLinkColor();
            else
                document()->resetActiveLinkColor();
        } else {
            RGBA32 color;
            if (CSSParser::parseColor(color, value, !document()->inQuirksMode())) {
                if (name == linkAttr)
                    document()->setLinkColor(color);
                else if (name == vlinkAttr)
                    document()->setVisitedLinkColor(color);
                else
                    document()->setActiveLinkColor(color);
            }
        }

        setNeedsStyleRecalc();
    } else if (name == onloadAttr)
        document()->setWindowAttributeEventListener(eventNames().loadEvent, createAttributeEventListener(document()->frame(), name, value));
    else if (name == onbeforeunloadAttr)
        document()->setWindowAttributeEventListener(eventNames().beforeunloadEvent, createAttributeEventListener(document()->frame(), name, value));
    else if (name == onunloadAttr)
        document()->setWindowAttributeEventListener(eventNames().unloadEvent, createAttributeEventListener(document()->frame(), name, value));
    else if (name == onpagehideAttr)
        document()->setWindowAttributeEventListener(eventNames().pagehideEvent, createAttributeEventListener(document()->frame(), name, value));
    else if (name == onpageshowAttr)
        document()->setWindowAttributeEventListener(eventNames().pageshowEvent, createAttributeEventListener(document()->frame(), name, value));
    else if (name == onpopstateAttr)
        document()->setWindowAttributeEventListener(eventNames().popstateEvent, createAttributeEventListener(document()->frame(), name, value));
    else if (name == onblurAttr)
        document()->setWindowAttributeEventListener(eventNames().blurEvent, createAttributeEventListener(document()->frame(), name, value));
    else if (name == onfocusAttr)
        document()->setWindowAttributeEventListener(eventNames().focusEvent, createAttributeEventListener(document()->frame(), name, value));
#if ENABLE(ORIENTATION_EVENTS)
    else if (name == onorientationchangeAttr)
        document()->setWindowAttributeEventListener(eventNames().orientationchangeEvent, createAttributeEventListener(document()->frame(), name, value));
#endif
    else if (name == onhashchangeAttr)
        document()->setWindowAttributeEventListener(eventNames().hashchangeEvent, createAttributeEventListener(document()->frame(), name, value));
    else if (name == onresizeAttr)
        document()->setWindowAttributeEventListener(eventNames().resizeEvent, createAttributeEventListener(document()->frame(), name, value));
    else if (name == onscrollAttr)
        document()->setWindowAttributeEventListener(eventNames().scrollEvent, createAttributeEventListener(document()->frame(), name, value));
    else if (name == onselectionchangeAttr)
        document()->setAttributeEventListener(eventNames().selectionchangeEvent, createAttributeEventListener(document()->frame(), name, value));
    else if (name == onstorageAttr)
        document()->setWindowAttributeEventListener(eventNames().storageEvent, createAttributeEventListener(document()->frame(), name, value));
    else if (name == ononlineAttr)
        document()->setWindowAttributeEventListener(eventNames().onlineEvent, createAttributeEventListener(document()->frame(), name, value));
    else if (name == onofflineAttr)
        document()->setWindowAttributeEventListener(eventNames().offlineEvent, createAttributeEventListener(document()->frame(), name, value));
    else
        HTMLElement::parseAttribute(name, value);
}

Node::InsertionNotificationRequest HTMLBodyElement::insertedInto(ContainerNode* insertionPoint)
{
    HTMLElement::insertedInto(insertionPoint);
    if (insertionPoint->inDocument())
        return InsertionShouldCallDidNotifySubtreeInsertions;
    return InsertionDone;
}

void HTMLBodyElement::didNotifySubtreeInsertions(ContainerNode* insertionPoint)
{
    ASSERT_UNUSED(insertionPoint, insertionPoint->inDocument());
    ASSERT(document());

    // FIXME: Perhaps this code should be in attach() instead of here.
    Element* ownerElement = document()->ownerElement();
    if (ownerElement && (ownerElement->hasTagName(frameTag) || ownerElement->hasTagName(iframeTag))) {
        HTMLFrameElementBase* ownerFrameElement = toHTMLFrameElementBase(ownerElement);
        int marginWidth = ownerFrameElement->marginWidth();
        if (marginWidth != -1)
            setAttribute(marginwidthAttr, String::number(marginWidth));
        int marginHeight = ownerFrameElement->marginHeight();
        if (marginHeight != -1)
            setAttribute(marginheightAttr, String::number(marginHeight));
    }

    // FIXME: This call to scheduleRelayout should not be needed here.
    // But without it we hang during WebKit tests; need to fix that and remove this.
    if (FrameView* view = document()->view())
        view->scheduleRelayout();
}

bool HTMLBodyElement::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name() == backgroundAttr || HTMLElement::isURLAttribute(attribute);
}

bool HTMLBodyElement::supportsFocus() const
{
    return rendererIsEditable() || HTMLElement::supportsFocus();
}

String HTMLBodyElement::aLink() const
{
    return getAttribute(alinkAttr);
}

void HTMLBodyElement::setALink(const String& value)
{
    setAttribute(alinkAttr, value);
}

String HTMLBodyElement::bgColor() const
{
    return getAttribute(bgcolorAttr);
}

void HTMLBodyElement::setBgColor(const String& value)
{
    setAttribute(bgcolorAttr, value);
}

String HTMLBodyElement::link() const
{
    return getAttribute(linkAttr);
}

void HTMLBodyElement::setLink(const String& value)
{
    setAttribute(linkAttr, value);
}

String HTMLBodyElement::text() const
{
    return getAttribute(textAttr);
}

void HTMLBodyElement::setText(const String& value)
{
    setAttribute(textAttr, value);
}

String HTMLBodyElement::vLink() const
{
    return getAttribute(vlinkAttr);
}

void HTMLBodyElement::setVLink(const String& value)
{
    setAttribute(vlinkAttr, value);
}

static int adjustForZoom(int value, Document* document)
{
    Frame* frame = document->frame();
    float zoomFactor = frame->pageZoomFactor() * frame->frameScaleFactor();
    if (zoomFactor == 1)
        return value;
    // Needed because of truncation (rather than rounding) when scaling up.
    if (zoomFactor > 1)
        value++;
    return static_cast<int>(value / zoomFactor);
}

int HTMLBodyElement::scrollLeft()
{
    // Update the document's layout.
    Document* document = this->document();
    document->updateLayoutIgnorePendingStylesheets();
    FrameView* view = document->view();
    return view ? adjustForZoom(view->scrollX(), document) : 0;
}

void HTMLBodyElement::setScrollLeft(int scrollLeft)
{
    Document* document = this->document();
    document->updateLayoutIgnorePendingStylesheets();
    Frame* frame = document->frame();
    if (!frame)
        return;
    FrameView* view = frame->view();
    if (!view)
        return;
    view->setScrollPosition(IntPoint(static_cast<int>(scrollLeft * frame->pageZoomFactor() * frame->frameScaleFactor()), view->scrollY()));
}

int HTMLBodyElement::scrollTop()
{
    // Update the document's layout.
    Document* document = this->document();
    document->updateLayoutIgnorePendingStylesheets();
    FrameView* view = document->view();
    return view ? adjustForZoom(view->scrollY(), document) : 0;
}

void HTMLBodyElement::setScrollTop(int scrollTop)
{
    Document* document = this->document();
    document->updateLayoutIgnorePendingStylesheets();
    Frame* frame = document->frame();
    if (!frame)
        return;
    FrameView* view = frame->view();
    if (!view)
        return;
    view->setScrollPosition(IntPoint(view->scrollX(), static_cast<int>(scrollTop * frame->pageZoomFactor() * frame->frameScaleFactor())));
}

int HTMLBodyElement::scrollHeight()
{
    // Update the document's layout.
    Document* document = this->document();
    document->updateLayoutIgnorePendingStylesheets();
    FrameView* view = document->view();
    return view ? adjustForZoom(view->contentsHeight(), document) : 0;    
}

int HTMLBodyElement::scrollWidth()
{
    // Update the document's layout.
    Document* document = this->document();
    document->updateLayoutIgnorePendingStylesheets();
    FrameView* view = document->view();
    return view ? adjustForZoom(view->contentsWidth(), document) : 0;    
}

void HTMLBodyElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    HTMLElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, document()->completeURL(getAttribute(backgroundAttr)));
}

} // namespace WebCore
