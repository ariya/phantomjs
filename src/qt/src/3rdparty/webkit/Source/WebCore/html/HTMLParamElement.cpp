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

bool HTMLParamElement::isURLParameter(const String& name)
{
    return equalIgnoringCase(name, "data") || equalIgnoringCase(name, "movie") || equalIgnoringCase(name, "src");
}

void HTMLParamElement::parseMappedAttribute(Attribute* attr)
{
    if (isIdAttributeName(attr->name())) {
        // Must call base class so that hasID bit gets set.
        HTMLElement::parseMappedAttribute(attr);
        if (document()->isHTMLDocument())
            return;
        m_name = attr->value();
    } else if (attr->name() == nameAttr) {
        m_name = attr->value();
    } else if (attr->name() == valueAttr) {
        m_value = attr->value();
    } else
        HTMLElement::parseMappedAttribute(attr);
}

bool HTMLParamElement::isURLAttribute(Attribute* attr) const
{
    if (attr->name() == valueAttr) {
        Attribute* attr = attributes()->getAttributeItem(nameAttr);
        if (attr) {
            const AtomicString& value = attr->value();
            if (isURLParameter(value))
                return true;
        }
    }
    return false;
}

void HTMLParamElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    HTMLElement::addSubresourceAttributeURLs(urls);

    if (!isURLParameter(name()))
        return;

    addSubresourceURL(urls, document()->completeURL(value()));
}

}
