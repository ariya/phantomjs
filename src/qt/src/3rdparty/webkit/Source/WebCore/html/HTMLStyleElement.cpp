/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2010 Apple Inc. All rights reserved.
 *           (C) 2007 Rob Buis (buis@kde.org)
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
#include "HTMLStyleElement.h"

#include "Attribute.h"
#include "Document.h"
#include "HTMLNames.h"
#include "ScriptEventListener.h"
#include "ScriptableDocumentParser.h"


namespace WebCore {

using namespace HTMLNames;

inline HTMLStyleElement::HTMLStyleElement(const QualifiedName& tagName, Document* document, bool createdByParser)
    : HTMLElement(tagName, document)
    , StyleElement(document, createdByParser)
{
    ASSERT(hasTagName(styleTag));
}

HTMLStyleElement::~HTMLStyleElement()
{
    StyleElement::clearDocumentData(document(), this);
}

PassRefPtr<HTMLStyleElement> HTMLStyleElement::create(const QualifiedName& tagName, Document* document, bool createdByParser)
{
    return adoptRef(new HTMLStyleElement(tagName, document, createdByParser));
}

void HTMLStyleElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == titleAttr && m_sheet)
        m_sheet->setTitle(attr->value());
    else if (attr->name() == onbeforeprocessAttr)
        setAttributeEventListener(eventNames().beforeprocessEvent, createAttributeEventListener(this, attr));
    else
        HTMLElement::parseMappedAttribute(attr);
}

void HTMLStyleElement::finishParsingChildren()
{
    StyleElement::finishParsingChildren(this);
    HTMLElement::finishParsingChildren();
}

void HTMLStyleElement::insertedIntoDocument()
{
    HTMLElement::insertedIntoDocument();
    StyleElement::insertedIntoDocument(document(), this);
}

void HTMLStyleElement::removedFromDocument()
{
    HTMLElement::removedFromDocument();
    StyleElement::removedFromDocument(document(), this);
}

void HTMLStyleElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    StyleElement::childrenChanged(this);
    HTMLElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
}

const AtomicString& HTMLStyleElement::media() const
{
    return getAttribute(mediaAttr);
}

const AtomicString& HTMLStyleElement::type() const
{
    return getAttribute(typeAttr);
}

void HTMLStyleElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{    
    HTMLElement::addSubresourceAttributeURLs(urls);

    if (StyleSheet* styleSheet = const_cast<HTMLStyleElement*>(this)->sheet())
        styleSheet->addSubresourceStyleURLs(urls);
}

bool HTMLStyleElement::disabled() const
{
    StyleSheet* styleSheet = sheet();
    if (!styleSheet)
        return false;

    return styleSheet->disabled();
}

void HTMLStyleElement::setDisabled(bool setDisabled)
{
    if (StyleSheet* styleSheet = sheet())
        styleSheet->setDisabled(setDisabled);
}

}
