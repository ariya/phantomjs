/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2010 Apple Inc. All rights reserved.
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
#include "HTMLTitleElement.h"

#include "Document.h"
#include "HTMLNames.h"
#include "RenderStyle.h"
#include "Text.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLTitleElement::HTMLTitleElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(titleTag));
}

PassRefPtr<HTMLTitleElement> HTMLTitleElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLTitleElement(tagName, document));
}

void HTMLTitleElement::insertedIntoDocument()
{
    HTMLElement::insertedIntoDocument();
    document()->setTitleElement(m_title, this);
}

void HTMLTitleElement::removedFromDocument()
{
    HTMLElement::removedFromDocument();
    document()->removeTitle(this);
}

void HTMLTitleElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    m_title = textWithDirection();
    if (inDocument())
        document()->setTitleElement(m_title, this);
    HTMLElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
}

String HTMLTitleElement::text() const
{
    String val = "";
    
    for (Node *n = firstChild(); n; n = n->nextSibling()) {
        if (n->isTextNode())
            val += static_cast<Text*>(n)->data();
    }

    return val;
}

StringWithDirection HTMLTitleElement::textWithDirection()
{
    TextDirection direction = LTR;
    if (RenderStyle* style = computedStyle())
        direction = style->direction();
    else if (RefPtr<RenderStyle> style = styleForRenderer())
        direction = style->direction();
    return StringWithDirection(text(), direction);
}

void HTMLTitleElement::setText(const String &value)
{
    ExceptionCode ec = 0;
    int numChildren = childNodeCount();
    
    if (numChildren == 1 && firstChild()->isTextNode())
        static_cast<Text*>(firstChild())->setData(value, ec);
    else {  
        // We make a copy here because entity of "value" argument can be Document::m_title,
        // which goes empty during removeChildren() invocation below,
        // which causes HTMLTitleElement::childrenChanged(), which ends up Document::setTitle().
        String valueCopy(value);

        if (numChildren > 0)
            removeChildren();

        appendChild(document()->createTextNode(valueCopy.impl()), ec);
    }
}

}
