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
#include "HTMLTableSectionElement.h"

#include "ExceptionCode.h"
#include "HTMLCollection.h"
#include "HTMLNames.h"
#include "HTMLTableRowElement.h"
#include "HTMLTableElement.h"
#include "NodeList.h"
#include "Text.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLTableSectionElement::HTMLTableSectionElement(const QualifiedName& tagName, Document* document)
    : HTMLTablePartElement(tagName, document)
{
}

PassRefPtr<HTMLTableSectionElement> HTMLTableSectionElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLTableSectionElement(tagName, document));
}

const StylePropertySet* HTMLTableSectionElement::additionalPresentationAttributeStyle()
{
    if (HTMLTableElement* table = findParentTable())
        return table->additionalGroupStyle(true);
    return 0;
}

// these functions are rather slow, since we need to get the row at
// the index... but they aren't used during usual HTML parsing anyway
PassRefPtr<HTMLElement> HTMLTableSectionElement::insertRow(int index, ExceptionCode& ec)
{
    RefPtr<HTMLTableRowElement> row;
    RefPtr<HTMLCollection> children = rows();
    int numRows = children ? (int)children->length() : 0;
    if (index < -1 || index > numRows)
        ec = INDEX_SIZE_ERR; // per the DOM
    else {
        row = HTMLTableRowElement::create(trTag, document());
        if (numRows == index || index == -1)
            appendChild(row, ec);
        else {
            Node* n;
            if (index < 1)
                n = firstChild();
            else
                n = children->item(index);
            insertBefore(row, n, ec);
        }
    }
    return row.release();
}

void HTMLTableSectionElement::deleteRow(int index, ExceptionCode& ec)
{
    RefPtr<HTMLCollection> children = rows();
    int numRows = children ? (int)children->length() : 0;
    if (index == -1)
        index = numRows - 1;
    if (index >= 0 && index < numRows) {
        RefPtr<Node> row = children->item(index);
        HTMLElement::removeChild(row.get(), ec);
    } else
        ec = INDEX_SIZE_ERR;
}

int HTMLTableSectionElement::numRows() const
{
    int rows = 0;
    const Node *n = firstChild();
    while (n) {
        if (n->hasTagName(trTag))
            rows++;
        n = n->nextSibling();
    }

    return rows;
}

String HTMLTableSectionElement::align() const
{
    return getAttribute(alignAttr);
}

void HTMLTableSectionElement::setAlign(const String &value)
{
    setAttribute(alignAttr, value);
}

String HTMLTableSectionElement::ch() const
{
    return getAttribute(charAttr);
}

void HTMLTableSectionElement::setCh(const String &value)
{
    setAttribute(charAttr, value);
}

String HTMLTableSectionElement::chOff() const
{
    return getAttribute(charoffAttr);
}

void HTMLTableSectionElement::setChOff(const String &value)
{
    setAttribute(charoffAttr, value);
}

String HTMLTableSectionElement::vAlign() const
{
    return getAttribute(valignAttr);
}

void HTMLTableSectionElement::setVAlign(const String &value)
{
    setAttribute(valignAttr, value);
}

PassRefPtr<HTMLCollection> HTMLTableSectionElement::rows()
{
    return ensureCachedHTMLCollection(TSectionRows);
}

}
