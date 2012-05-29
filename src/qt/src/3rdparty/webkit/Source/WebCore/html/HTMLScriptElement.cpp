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

PassRefPtr<HTMLScriptElement> HTMLScriptElement::create(const QualifiedName& tagName, Document* document, bool wasInsertedByParser)
{
    return adoptRef(new HTMLScriptElement(tagName, document, wasInsertedByParser, false));
}

bool HTMLScriptElement::isURLAttribute(Attribute* attr) const
{
    return attr->name() == srcAttr;
}

void HTMLScriptElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    ScriptElement::childrenChanged();
    HTMLElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
}

void HTMLScriptElement::attributeChanged(Attribute* attr, bool preserveDecls)
{
    if (attr->name() == asyncAttr)
        handleAsyncAttribute();
    HTMLElement::attributeChanged(attr, preserveDecls);
}

void HTMLScriptElement::parseMappedAttribute(Attribute* attr)
{
    const QualifiedName& attrName = attr->name();

    if (attrName == srcAttr)
        handleSourceAttribute(attr->value());
    else if (attrName == onloadAttr)
        setAttributeEventListener(eventNames().loadEvent, createAttributeEventListener(this, attr));
    else if (attrName == onbeforeloadAttr)
        setAttributeEventListener(eventNames().beforeloadEvent, createAttributeEventListener(this, attr));
    else if (attrName == onbeforeprocessAttr)
        setAttributeEventListener(eventNames().beforeprocessEvent, createAttributeEventListener(this, attr));
    else
        HTMLElement::parseMappedAttribute(attr);
}

void HTMLScriptElement::insertedIntoDocument()
{
    HTMLElement::insertedIntoDocument();
    ScriptElement::insertedIntoDocument();
}

void HTMLScriptElement::removedFromDocument()
{
    HTMLElement::removedFromDocument();
    ScriptElement::removedFromDocument();
}

void HTMLScriptElement::setText(const String &value)
{
    ExceptionCode ec = 0;
    int numChildren = childNodeCount();

    if (numChildren == 1 && firstChild()->isTextNode()) {
        static_cast<Text*>(firstChild())->setData(value, ec);
        return;
    }

    if (numChildren > 0)
        removeChildren();

    appendChild(document()->createTextNode(value.impl()), ec);
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

PassRefPtr<Element> HTMLScriptElement::cloneElementWithoutAttributesAndChildren() const
{
    return adoptRef(new HTMLScriptElement(tagQName(), document(), false, alreadyStarted()));
}

}
