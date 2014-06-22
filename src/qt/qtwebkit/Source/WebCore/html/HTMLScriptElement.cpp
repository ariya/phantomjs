/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "HTMLScriptElement.h"

#include "Attribute.h"
#include "Document.h"
#include "Event.h"
#include "EventNames.h"
#include "HTMLNames.h"
#include "ScriptEventListener.h"
#include "Text.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLScriptElement::HTMLScriptElement(const QualifiedName& tagName, Document* document, bool wasInsertedByParser, bool alreadyStarted)
    : HTMLElement(tagName, document)
    , ScriptElement(this, wasInsertedByParser, alreadyStarted)
{
    ASSERT(hasTagName(scriptTag));
}

PassRefPtr<HTMLScriptElement> HTMLScriptElement::create(const QualifiedName& tagName, Document* document, bool wasInsertedByParser, bool alreadyStarted)
{
    return adoptRef(new HTMLScriptElement(tagName, document, wasInsertedByParser, alreadyStarted));
}

bool HTMLScriptElement::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name() == srcAttr || HTMLElement::isURLAttribute(attribute);
}

void HTMLScriptElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    HTMLElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
    ScriptElement::childrenChanged();
}

void HTMLScriptElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == srcAttr)
        handleSourceAttribute(value);
    else if (name == asyncAttr)
        handleAsyncAttribute();
    else if (name == onbeforeloadAttr)
        setAttributeEventListener(eventNames().beforeloadEvent, createAttributeEventListener(this, name, value));
    else
        HTMLElement::parseAttribute(name, value);
}

Node::InsertionNotificationRequest HTMLScriptElement::insertedInto(ContainerNode* insertionPoint)
{
    HTMLElement::insertedInto(insertionPoint);
    ScriptElement::insertedInto(insertionPoint);
    return InsertionDone;
}

void HTMLScriptElement::setText(const String &value)
{
    RefPtr<Node> protectFromMutationEvents(this);

    int numChildren = childNodeCount();

    if (numChildren == 1 && firstChild()->isTextNode()) {
        toText(firstChild())->setData(value, IGNORE_EXCEPTION);
        return;
    }

    if (numChildren > 0)
        removeChildren();

    appendChild(document()->createTextNode(value.impl()), IGNORE_EXCEPTION);
}

void HTMLScriptElement::setAsync(bool async)
{
    setBooleanAttribute(asyncAttr, async);
    handleAsyncAttribute();
}

bool HTMLScriptElement::async() const
{
    return fastHasAttribute(asyncAttr) || forceAsync();
}

KURL HTMLScriptElement::src() const
{
    return document()->completeURL(sourceAttributeValue());
}

void HTMLScriptElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    HTMLElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, src());
}

String HTMLScriptElement::sourceAttributeValue() const
{
    return getAttribute(srcAttr).string();
}

String HTMLScriptElement::charsetAttributeValue() const
{
    return getAttribute(charsetAttr).string();
}

String HTMLScriptElement::typeAttributeValue() const
{
    return getAttribute(typeAttr).string();
}

String HTMLScriptElement::languageAttributeValue() const
{
    return getAttribute(languageAttr).string();
}

String HTMLScriptElement::forAttributeValue() const
{
    return getAttribute(forAttr).string();
}

String HTMLScriptElement::eventAttributeValue() const
{
    return getAttribute(eventAttr).string();
}

bool HTMLScriptElement::asyncAttributeValue() const
{
    return fastHasAttribute(asyncAttr);
}

bool HTMLScriptElement::deferAttributeValue() const
{
    return fastHasAttribute(deferAttr);
}

bool HTMLScriptElement::hasSourceAttribute() const
{
    return fastHasAttribute(srcAttr);
}

void HTMLScriptElement::dispatchLoadEvent()
{
    ASSERT(!haveFiredLoadEvent());
    setHaveFiredLoadEvent(true);

    dispatchEvent(Event::create(eventNames().loadEvent, false, false));
}

PassRefPtr<Element> HTMLScriptElement::cloneElementWithoutAttributesAndChildren()
{
    return adoptRef(new HTMLScriptElement(tagQName(), document(), false, alreadyStarted()));
}

}
