/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann (hausmann@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2006, 2009, 2010 Apple Inc. All rights reserved.
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
#include "HTMLFrameSetElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "Document.h"
#include "Event.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameLoaderClient.h"
#include "HTMLNames.h"
#include "Length.h"
#include "MouseEvent.h"
#include "RenderFrameSet.h"
#include "ScriptEventListener.h"
#include "Text.h"

namespace WebCore {

using namespace HTMLNames;

HTMLFrameSetElement::HTMLFrameSetElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
    , m_totalRows(1)
    , m_totalCols(1)
    , m_border(6)
    , m_borderSet(false)
    , m_borderColorSet(false)
    , m_frameborder(true)
    , m_frameborderSet(false)
    , m_noresize(false)
{
    ASSERT(hasTagName(framesetTag));
}

PassRefPtr<HTMLFrameSetElement> HTMLFrameSetElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLFrameSetElement(tagName, document));
}

bool HTMLFrameSetElement::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == bordercolorAttr) {
        result = eUniversal;
        return true;
    }

    return HTMLElement::mapToEntry(attrName, result);
}

void HTMLFrameSetElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == rowsAttr) {
        if (!attr->isNull()) {
            m_rowLengths = newLengthArray(attr->value().string(), m_totalRows);
            setNeedsStyleRecalc();
        }
    } else if (attr->name() == colsAttr) {
        if (!attr->isNull()) {
            m_colLengths = newLengthArray(attr->value().string(), m_totalCols);
            setNeedsStyleRecalc();
        }
    } else if (attr->name() == frameborderAttr) {
        if (!attr->isNull()) {
            // false or "no" or "0"..
            if (attr->value().toInt() == 0) {
                m_frameborder = false;
                m_border = 0;
            }
            m_frameborderSet = true;
        } else {
            m_frameborder = false;
            m_frameborderSet = false;
        }
    } else if (attr->name() == noresizeAttr) {
        m_noresize = true;
    } else if (attr->name() == borderAttr) {
        if (!attr->isNull()) {
            m_border = attr->value().toInt();
            if (!m_border)
                m_frameborder = false;
            m_borderSet = true;
        } else
            m_borderSet = false;
    } else if (attr->name() == bordercolorAttr) {
        m_borderColorSet = attr->decl();
        if (!attr->decl() && !attr->isEmpty()) {
            addCSSColor(attr, CSSPropertyBorderColor, attr->value());
            m_borderColorSet = true;
        }
    } else if (attr->name() == onloadAttr)
        document()->setWindowAttributeEventListener(eventNames().loadEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == onbeforeunloadAttr)
        document()->setWindowAttributeEventListener(eventNames().beforeunloadEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == onunloadAttr)
        document()->setWindowAttributeEventListener(eventNames().unloadEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == onblurAttr)
        document()->setWindowAttributeEventListener(eventNames().blurEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == onfocusAttr)
        document()->setWindowAttributeEventListener(eventNames().focusEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == onfocusinAttr)
        document()->setWindowAttributeEventListener(eventNames().focusinEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == onfocusoutAttr)
        document()->setWindowAttributeEventListener(eventNames().focusoutEvent, createAttributeEventListener(document()->frame(), attr));
#if ENABLE(ORIENTATION_EVENTS)
    else if (attr->name() == onorientationchangeAttr)
        document()->setWindowAttributeEventListener(eventNames().orientationchangeEvent, createAttributeEventListener(document()->frame(), attr));
#endif
    else if (attr->name() == onhashchangeAttr)
        document()->setWindowAttributeEventListener(eventNames().hashchangeEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == onresizeAttr)
        document()->setWindowAttributeEventListener(eventNames().resizeEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == onscrollAttr)
        document()->setWindowAttributeEventListener(eventNames().scrollEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == onstorageAttr)
        document()->setWindowAttributeEventListener(eventNames().storageEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == ononlineAttr)
        document()->setWindowAttributeEventListener(eventNames().onlineEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == onofflineAttr)
        document()->setWindowAttributeEventListener(eventNames().offlineEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == onpopstateAttr)
        document()->setWindowAttributeEventListener(eventNames().popstateEvent, createAttributeEventListener(document()->frame(), attr));
    else
        HTMLElement::parseMappedAttribute(attr);
}

bool HTMLFrameSetElement::rendererIsNeeded(RenderStyle *style)
{
    // For compatibility, frames render even when display: none is set.
    // However, we delay creating a renderer until stylesheets have loaded. 
    return style->isStyleAvailable();
}

RenderObject *HTMLFrameSetElement::createRenderer(RenderArena *arena, RenderStyle *style)
{
    if (style->contentData())
        return RenderObject::createObject(this, style);
    
    return new (arena) RenderFrameSet(this);
}

void HTMLFrameSetElement::attach()
{
    // Inherit default settings from parent frameset
    // FIXME: This is not dynamic.
    for (ContainerNode* node = parentNode(); node; node = node->parentNode()) {
        if (node->hasTagName(framesetTag)) {
            HTMLFrameSetElement* frameset = static_cast<HTMLFrameSetElement*>(node);
            if (!m_frameborderSet)
                m_frameborder = frameset->hasFrameBorder();
            if (m_frameborder) {
                if (!m_borderSet)
                    m_border = frameset->border();
                if (!m_borderColorSet)
                    m_borderColorSet = frameset->hasBorderColor();
            }
            if (!m_noresize)
                m_noresize = frameset->noResize();
            break;
        }
    }

    HTMLElement::attach();
}

void HTMLFrameSetElement::defaultEventHandler(Event* evt)
{
    if (evt->isMouseEvent() && !m_noresize && renderer() && renderer()->isFrameSet()) {
        if (toRenderFrameSet(renderer())->userResize(static_cast<MouseEvent*>(evt))) {
            evt->setDefaultHandled();
            return;
        }
    }
    HTMLElement::defaultEventHandler(evt);
}

void HTMLFrameSetElement::recalcStyle(StyleChange ch)
{
    if (needsStyleRecalc() && renderer()) {
        renderer()->setNeedsLayout(true);
        clearNeedsStyleRecalc();
    }
    HTMLElement::recalcStyle(ch);
}

void HTMLFrameSetElement::insertedIntoDocument()
{
    HTMLElement::insertedIntoDocument();
    if (Frame* frame = document()->frame())
        frame->loader()->client()->dispatchDidBecomeFrameset(document()->isFrameSet());
}

void HTMLFrameSetElement::removedFromDocument()
{
    HTMLElement::removedFromDocument();
    if (Frame* frame = document()->frame())
        frame->loader()->client()->dispatchDidBecomeFrameset(document()->isFrameSet());
}

} // namespace WebCore
