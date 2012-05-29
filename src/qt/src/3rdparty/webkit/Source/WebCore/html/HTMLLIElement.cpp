/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2006, 2007, 2010 Apple Inc. All rights reserved.
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
#include "HTMLLIElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "HTMLNames.h"
#include "RenderListItem.h"

namespace WebCore {

using namespace HTMLNames;

HTMLLIElement::HTMLLIElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
    , m_requestedValue(0)
{
    ASSERT(hasTagName(liTag));
}

PassRefPtr<HTMLLIElement> HTMLLIElement::create(Document* document)
{
    return adoptRef(new HTMLLIElement(liTag, document));
}

PassRefPtr<HTMLLIElement> HTMLLIElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLLIElement(tagName, document));
}

bool HTMLLIElement::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == typeAttr) {
        result = eListItem; // Share with <ol> since all the values are the same
        return false;
    }
    
    return HTMLElement::mapToEntry(attrName, result);
}

void HTMLLIElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == valueAttr) {
        m_requestedValue = attr->value().toInt();
        if (renderer() && renderer()->isListItem()) {
            if (m_requestedValue > 0)
                toRenderListItem(renderer())->setExplicitValue(m_requestedValue);
            else
                toRenderListItem(renderer())->clearExplicitValue();
        }
    } else if (attr->name() == typeAttr) {
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
        else
            addCSSProperty(attr, CSSPropertyListStyleType, attr->value());
    } else
        HTMLElement::parseMappedAttribute(attr);
}

void HTMLLIElement::attach()
{
    ASSERT(!attached());

    HTMLElement::attach();

    if (renderer() && renderer()->isListItem()) {
        RenderListItem* render = toRenderListItem(renderer());

        // Find the enclosing list node.
        Node* listNode = 0;
        Node* n = this;
        while (!listNode && (n = n->parentNode())) {
            if (n->hasTagName(ulTag) || n->hasTagName(olTag))
                listNode = n;
        }

        // If we are not in a list, tell the renderer so it can position us inside.
        // We don't want to change our style to say "inside" since that would affect nested nodes.
        if (!listNode)
            render->setNotInList(true);

        if (m_requestedValue > 0)
            render->setExplicitValue(m_requestedValue);
        else
            render->clearExplicitValue();
    }
}

}
