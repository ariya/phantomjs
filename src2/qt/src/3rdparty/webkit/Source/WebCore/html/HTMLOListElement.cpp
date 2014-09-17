/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "HTMLOListElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "HTMLNames.h"
#include "RenderListItem.h"

namespace WebCore {

using namespace HTMLNames;

HTMLOListElement::HTMLOListElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
    , m_start(1)
{
    ASSERT(hasTagName(olTag));
}

PassRefPtr<HTMLOListElement> HTMLOListElement::create(Document* document)
{
    return adoptRef(new HTMLOListElement(olTag, document));
}

PassRefPtr<HTMLOListElement> HTMLOListElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLOListElement(tagName, document));
}

bool HTMLOListElement::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == typeAttr) {
        result = eListItem; // Share with <li>
        return false;
    }
    
    return HTMLElement::mapToEntry(attrName, result);
}

void HTMLOListElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == typeAttr) {
        if (attr->value() == "a")
            addCSSProperty(attr, CSSPropertyListStyleType, CSSValueLowerAlpha);
        else if (attr->value() == "A")
            addCSSProperty(attr, CSSPropertyListStyleType, CSSValueUpperAlpha);
        else if (attr->value() == "i")
            addCSSProperty(attr, CSSPropertyListStyleType, CSSValueLowerRoman);
        else if (attr->value() == "I")
            addCSSProperty(attr, CSSPropertyListStyleType, CSSValueUpperRoman);
        else if (attr->value() == "1")
            addCSSProperty(attr, CSSPropertyListStyleType, CSSValueDecimal);
    } else if (attr->name() == startAttr) {
        bool canParse;
        int start = attr->value().toInt(&canParse);
        if (!canParse)
            start = 1;
        if (start == m_start)
            return;
        m_start = start;
        for (RenderObject* child = renderer(); child; child = child->nextInPreOrder(renderer())) {
            if (child->isListItem())
                toRenderListItem(child)->updateValue();
        }
    } else
        HTMLElement::parseMappedAttribute(attr);
}

void HTMLOListElement::setStart(int start)
{
    setAttribute(startAttr, String::number(start));
}

}
