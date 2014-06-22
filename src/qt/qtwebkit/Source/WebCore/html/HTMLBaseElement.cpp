/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
#include "HTMLBaseElement.h"

#include "Attribute.h"
#include "Document.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "TextResourceDecoder.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLBaseElement::HTMLBaseElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(baseTag));
}

PassRefPtr<HTMLBaseElement> HTMLBaseElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLBaseElement(tagName, document));
}

void HTMLBaseElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == hrefAttr || name == targetAttr)
        document()->processBaseElement();
    else
        HTMLElement::parseAttribute(name, value);
}

Node::InsertionNotificationRequest HTMLBaseElement::insertedInto(ContainerNode* insertionPoint)
{
    HTMLElement::insertedInto(insertionPoint);
    if (insertionPoint->inDocument())
        document()->processBaseElement();
    return InsertionDone;
}

void HTMLBaseElement::removedFrom(ContainerNode* insertionPoint)
{
    HTMLElement::removedFrom(insertionPoint);
    if (insertionPoint->inDocument())
        document()->processBaseElement();
}

bool HTMLBaseElement::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name().localName() == hrefAttr || HTMLElement::isURLAttribute(attribute);
}

String HTMLBaseElement::target() const
{
    return fastGetAttribute(targetAttr);
}

KURL HTMLBaseElement::href() const
{
    // This does not use the getURLAttribute function because that will resolve relative to the document's base URL;
    // base elements like this one can be used to set that base URL. Thus we need to resolve relative to the document's
    // URL and ignore the base URL.

    const AtomicString& attributeValue = fastGetAttribute(hrefAttr);
    if (attributeValue.isNull())
        return document()->url();

    KURL url = !document()->decoder() ?
        KURL(document()->url(), stripLeadingAndTrailingHTMLSpaces(attributeValue)) :
        KURL(document()->url(), stripLeadingAndTrailingHTMLSpaces(attributeValue), document()->decoder()->encoding());

    if (!url.isValid())
        return KURL();

    return url;
}

void HTMLBaseElement::setHref(const AtomicString& value)
{
    setAttribute(hrefAttr, value);
}

}
