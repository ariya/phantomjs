/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004, 2005, 2006, 2008, 2010 Apple Inc. All rights reserved.
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
#include "HTMLParamElement.h"

#include "Attribute.h"
#include "Document.h"
#include "HTMLNames.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLParamElement::HTMLParamElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(paramTag));
}

PassRefPtr<HTMLParamElement> HTMLParamElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLParamElement(tagName, document));
}

String HTMLParamElement::name() const
{
    if (hasName())
        return getNameAttribute();
    return document()->isHTMLDocument() ? emptyAtom : getIdAttribute();
}

String HTMLParamElement::value() const
{
    return fastGetAttribute(valueAttr);
}

bool HTMLParamElement::isURLParameter(const String& name)
{
    return equalIgnoringCase(name, "data") || equalIgnoringCase(name, "movie") || equalIgnoringCase(name, "src");
}

bool HTMLParamElement::isURLAttribute(const Attribute& attribute) const
{
    if (attribute.name() == valueAttr && isURLParameter(name()))
        return true;
    return HTMLElement::isURLAttribute(attribute);
}

void HTMLParamElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    HTMLElement::addSubresourceAttributeURLs(urls);

    if (!isURLParameter(name()))
        return;

    addSubresourceURL(urls, document()->completeURL(value()));
}

}
