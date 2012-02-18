/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
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
#include "HTMLTableRowElement.h"

#include "ExceptionCode.h"
#include "HTMLCollection.h"
#include "HTMLNames.h"
#include "HTMLTableCellElement.h"
#include "HTMLTableElement.h"
#include "HTMLTableSectionElement.h"
#include "NodeList.h"
#include "Text.h"

namespace WebCore {

using namespace HTMLNames;

HTMLTableRowElement::HTMLTableRowElement(const QualifiedName& tagName, Document* document)
    : HTMLTablePartElement(tagName, document)
{
    ASSERT(hasTagName(trTag));
}

PassRefPtr<HTMLTableRowElement> HTMLTableRowElement::create(Document* document)
{
    return adoptRef(new HTMLTableRowElement(trTag, document));
}

PassRefPtr<HTMLTableRowElement> HTMLTableRowElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLTableRowElement(tagName, document));
}

int HTMLTableRowElement::rowIndex() const
{
    ContainerNode* table = parentNode();
    if (!table)
        return -1;
    table = table->parentNode();
    if (!table || !table->hasTagName(tableTag))
        return -1;

    // To match Firefox, the row indices work like this:
    //   Rows from the first <thead> are numbered before all <tbody> rows.
    //   Rows from the first <tfoot> are numbered after all <tbody> rows.
    //   Rows from other <thead> and <tfoot> elements don't get row indices at all.

    int rIndex = 0;

    if (HTMLTableSectionElement* head = static_cast<HTMLTableElement*>(table)->tHead()) {
        for (Node *row = head->firstChild(); row; row = row->nextSibling()) {
            if (row == this)
                return rIndex;
            if (row->hasTagName(trTag))
                ++rIndex;
        }
    }
    
    for (Node *node = table->firstChild(); node; node = node->nextSibling()) {
        if (node->hasTagName(tbodyTag)) {
            HTMLTableSectionElement* section = static_cast<HTMLTableSectionElement*>(node);
            for (Node* row = section->firstChild(); row; row = row->nextSibling()) {
                if (row == this)
                    return rIndex;
                if (row->hasTagName(trTag))
                    ++rIndex;
            }
        }
    }

    if (HTMLTableSectionElement* foot = static_cast<HTMLTableElement*>(table)->tFoot()) {
        for (Node *row = foot->firstChild(); row; row = row->nextSibling()) {
            if (row == this)
                return rIndex;
            if (row->hasTagName(trTag))
                ++rIndex;
        }
    }

    // We get here for rows that are in <thead> or <tfoot> sections other than the main header and footer.
    return -1;
}

int HTMLTableRowElement::sectionRowIndex() const
{
    int rIndex = 0;
    const Node *n = this;
    do {
        n = n->previousSibling();
        if (n && n->hasTagName(trTag))
            rIndex++;
    }
    while (n);

    return rIndex;
}

PassRefPtr<HTMLElement> HTMLTableRowElement::insertCell(int index, ExceptionCode& ec)
{
    RefPtr<HTMLCollection> children = cells();
    int numCells = children ? children->length() : 0;
    if (index < -1 || index > numCells) {
        ec = INDEX_SIZE_ERR;
        return 0;
    }

    RefPtr<HTMLTableCellElement> cell = HTMLTableCellElement::create(tdTag, document());
    if (index < 0 || index >= numCells)
        appendChild(cell, ec);
    else {
        Node* n;
        if (index < 1)
            n = firstChild();
        else
            n = children->item(index);
        insertBefore(cell, n, ec);
    }
    return cell.release();
}

void HTMLTableRowElement::deleteCell(int index, ExceptionCode& ec)
{
    RefPtr<HTMLCollection> children = cells();
    int numCells = children ? children->length() : 0;
    if (index == -1)
        index = numCells-1;
    if (index >= 0 && index < numCells) {
        RefPtr<Node> cell = children->item(index);
        HTMLElement::removeChild(cell.get(), ec);
    } else
        ec = INDEX_SIZE_ERR;
}

PassRefPtr<HTMLCollection> HTMLTableRowElement::cells()
{
    return HTMLCollection::create(this, TRCells);
}

void HTMLTableRowElement::setCells(HTMLCollection*, ExceptionCode& ec)
{
    ec = NO_MODIFICATION_ALLOWED_ERR;
}

}
