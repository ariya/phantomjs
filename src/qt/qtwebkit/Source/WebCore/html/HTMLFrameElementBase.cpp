/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann (hausmann@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2006, 2008, 2009 Apple Inc. All rights reserved.
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
#include "HTMLFrameElementBase.h"

#include "Attribute.h"
#include "Document.h"
#include "EventNames.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "KURL.h"
#include "Page.h"
#include "RenderPart.h"
#include "ScriptController.h"
#include "ScriptEventListener.h"
#include "Settings.h"

namespace WebCore {

using namespace HTMLNames;

HTMLFrameElementBase::HTMLFrameElementBase(const QualifiedName& tagName, Document* document)
    : HTMLFrameOwnerElement(tagName, document)
    , m_scrolling(ScrollbarAuto)
    , m_marginWidth(-1)
    , m_marginHeight(-1)
    , m_viewSource(false)
{
}

bool HTMLFrameElementBase::isURLAllowed() const
{
    if (m_URL.isEmpty())
        return true;

    const KURL& completeURL = document()->completeURL(m_URL);

    if (protocolIsJavaScript(completeURL)) { 
        Document* contentDoc = this->contentDocument();
        if (contentDoc && !ScriptController::canAccessFromCurrentOrigin(contentDoc->frame()))
            return false;
    }

    Frame* parentFrame = document()->frame();
    if (parentFrame)
        return parentFrame->isURLAllowed(completeURL);

    return true;
}

void HTMLFrameElementBase::openURL(bool lockHistory, bool lockBackForwardList)
{
    if (!isURLAllowed())
        return;

    if (m_URL.isEmpty())
        m_URL = blankURL().string();

    Frame* parentFrame = document()->frame();
    if (!parentFrame)
        return;

    parentFrame->loader()->subframeLoader()->requestFrame(this, m_URL, m_frameName, lockHistory, lockBackForwardList);
    if (contentFrame())
        contentFrame()->setInViewSourceMode(viewSourceMode());
}

void HTMLFrameElementBase::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == srcdocAttr)
        setLocation("about:srcdoc");
    else if (name == srcAttr && !fastHasAttribute(srcdocAttr))
        setLocation(stripLeadingAndTrailingHTMLSpaces(value));
    else if (isIdAttributeName(name)) {
        // Important to call through to base for the id attribute so the hasID bit gets set.
        HTMLFrameOwnerElement::parseAttribute(name, value);
        m_frameName = value;
    } else if (name == nameAttr) {
        m_frameName = value;
        // FIXME: If we are already attached, this doesn't actually change the frame's name.
        // FIXME: If we are already attached, this doesn't check for frame name
        // conflicts and generate a unique frame name.
    } else if (name == marginwidthAttr) {
        m_marginWidth = value.toInt();
        // FIXME: If we are already attached, this has no effect.
    } else if (name == marginheightAttr) {
        m_marginHeight = value.toInt();
        // FIXME: If we are already attached, this has no effect.
    } else if (name == scrollingAttr) {
        // Auto and yes both simply mean "allow scrolling." No means "don't allow scrolling."
        if (equalIgnoringCase(value, "auto") || equalIgnoringCase(value, "yes"))
            m_scrolling = document()->frameElementsShouldIgnoreScrolling() ? ScrollbarAlwaysOff : ScrollbarAuto;
        else if (equalIgnoringCase(value, "no"))
            m_scrolling = ScrollbarAlwaysOff;
        // FIXME: If we are already attached, this has no effect.
#if ENABLE(VIEWSOURCE_ATTRIBUTE)
    } else if (name == viewsourceAttr) {
        m_viewSource = !value.isNull();
        if (contentFrame())
            contentFrame()->setInViewSourceMode(viewSourceMode());
#endif
    } else if (name == onbeforeloadAttr)
        setAttributeEventListener(eventNames().beforeloadEvent, createAttributeEventListener(this, name, value));
    else if (name == onbeforeunloadAttr) {
        // FIXME: should <frame> elements have beforeunload handlers?
        setAttributeEventListener(eventNames().beforeunloadEvent, createAttributeEventListener(this, name, value));
    } else
        HTMLFrameOwnerElement::parseAttribute(name, value);
}

void HTMLFrameElementBase::setNameAndOpenURL()
{
    m_frameName = getNameAttribute();
    if (m_frameName.isNull())
        m_frameName = getIdAttribute();
    openURL();
}

Node::InsertionNotificationRequest HTMLFrameElementBase::insertedInto(ContainerNode* insertionPoint)
{
    HTMLFrameOwnerElement::insertedInto(insertionPoint);
    if (insertionPoint->inDocument())
        return InsertionShouldCallDidNotifySubtreeInsertions;
    return InsertionDone;
}

void HTMLFrameElementBase::didNotifySubtreeInsertions(ContainerNode*)
{
    if (!inDocument())
        return;

    // DocumentFragments don't kick of any loads.
    if (!document()->frame())
        return;

    if (!SubframeLoadingDisabler::canLoadFrame(this))
        return;

    // JavaScript in src=javascript: and beforeonload can access the renderer
    // during attribute parsing *before* the normal parser machinery would
    // attach the element. To support this, we lazyAttach here, but only
    // if we don't already have a renderer (if we're inserted
    // as part of a DocumentFragment, insertedInto from an earlier element
    // could have forced a style resolve and already attached us).
    if (!renderer())
        lazyAttach(DoNotSetAttached);
    setNameAndOpenURL();
}

void HTMLFrameElementBase::attach(const AttachContext& context)
{
    HTMLFrameOwnerElement::attach(context);

    if (RenderPart* part = renderPart()) {
        if (Frame* frame = contentFrame())
            part->setWidget(frame->view());
    }
}

KURL HTMLFrameElementBase::location() const
{
    if (fastHasAttribute(srcdocAttr))
        return KURL(ParsedURLString, "about:srcdoc");
    return document()->completeURL(getAttribute(srcAttr));
}

void HTMLFrameElementBase::setLocation(const String& str)
{
    Settings* settings = document()->settings();
    if (settings && settings->needsAcrobatFrameReloadingQuirk() && m_URL == str)
        return;

    m_URL = AtomicString(str);

    if (inDocument())
        openURL(false, false);
}

bool HTMLFrameElementBase::supportsFocus() const
{
    return true;
}

void HTMLFrameElementBase::setFocus(bool received)
{
    HTMLFrameOwnerElement::setFocus(received);
    if (Page* page = document()->page()) {
        if (received)
            page->focusController()->setFocusedFrame(contentFrame());
        else if (page->focusController()->focusedFrame() == contentFrame()) // Focus may have already been given to another frame, don't take it away.
            page->focusController()->setFocusedFrame(0);
    }
}

bool HTMLFrameElementBase::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name() == srcAttr || HTMLFrameOwnerElement::isURLAttribute(attribute);
}

bool HTMLFrameElementBase::isHTMLContentAttribute(const Attribute& attribute) const
{
    return attribute.name() == srcdocAttr || HTMLFrameOwnerElement::isHTMLContentAttribute(attribute);
}

int HTMLFrameElementBase::width()
{
    document()->updateLayoutIgnorePendingStylesheets();
    if (!renderBox())
        return 0;
    return renderBox()->width();
}

int HTMLFrameElementBase::height()
{
    document()->updateLayoutIgnorePendingStylesheets();
    if (!renderBox())
        return 0;
    return renderBox()->height();
}

} // namespace WebCore
