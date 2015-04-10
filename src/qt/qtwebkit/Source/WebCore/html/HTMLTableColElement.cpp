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
#include "HTMLTableColElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "HTMLNames.h"
#include "HTMLTableElement.h"
#include "RenderTableCol.h"
#include "Text.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLTableColElement::HTMLTableColElement(const QualifiedName& tagName, Document* document)
    : HTMLTablePartElement(tagName, document)
    , m_span(1)
{
}

PassRefPtr<HTMLTableColElement> HTMLTableColElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLTableColElement(tagName, document));
}

bool HTMLTableColElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == widthAttr)
        return true;
    return HTMLTablePartElement::isPresentationAttribute(name);
}

void HTMLTableColElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStylePropertySet* style)
{
    if (name == widthAttr)
        addHTMLLengthToStyle(style, CSSPropertyWidth, value);
    else
        HTMLTablePartElement::collectStyleForPresentationAttribute(name, value, style);
}

void HTMLTableColElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == spanAttr) {
        m_span = !value.isNull() ? value.toInt() : 1;
        if (renderer() && renderer()->isRenderTableCol())
            renderer()->updateFromElement();
    } else if (name == widthAttr) {
        if (!value.isEmpty()) {
            if (renderer() && renderer()->isRenderTableCol()) {
                RenderTableCol* col = toRenderTableCol(renderer());
                int newWidth = width().toInt();
                if (newWidth != col->width())
                    col->setNeedsLayoutAndPrefWidthsRecalc();
            }
        }
    } else
        HTMLTablePartElement::parseAttribute(name, value);
}

const StylePropertySet* HTMLTableColElement::additionalPresentationAttributeStyle()
{
    if (!hasLocalName(colgroupTag))
        return 0;
    if (HTMLTableElement* table = findParentTable())
        return table->additionalGroupStyle(false);
    return 0;
}

void HTMLTableColElement::setSpan(int n)
{
    setAttribute(spanAttr, String::number(n));
}

String HTMLTableColElement::width() const
{
    return getAttribute(widthAttr);
}

}
