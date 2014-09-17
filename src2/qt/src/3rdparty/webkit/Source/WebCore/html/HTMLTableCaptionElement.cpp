/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
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
#include "HTMLTableCaptionElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "HTMLNames.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLTableCaptionElement::HTMLTableCaptionElement(const QualifiedName& tagName, Document* document)
    : HTMLTablePartElement(tagName, document)
{
    ASSERT(hasTagName(captionTag));
}

PassRefPtr<HTMLTableCaptionElement> HTMLTableCaptionElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLTableCaptionElement(tagName, document));
}

bool HTMLTableCaptionElement::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == alignAttr) {
        result = eCaption;
        return false;
    }

    return HTMLElement::mapToEntry(attrName, result);
}

void HTMLTableCaptionElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == alignAttr) {
        if (!attr->value().isEmpty())
            addCSSProperty(attr, CSSPropertyCaptionSide, attr->value());
    } else
        HTMLElement::parseMappedAttribute(attr);
}

}
