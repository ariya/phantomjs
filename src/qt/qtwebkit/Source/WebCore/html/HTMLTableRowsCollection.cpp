/*
 * Copyright (C) 2008, 2011, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "HTMLTableRowsCollection.h"

#include "HTMLNames.h"
#include "HTMLTableElement.h"
#include "HTMLTableRowElement.h"

namespace WebCore {

using namespace HTMLNames;

static bool isInHead(Element* row)
{
    return row->parentNode() && toElement(row->parentNode())->hasLocalName(theadTag);
}

static bool isInBody(Element* row)
{
    return row->parentNode() && toElement(row->parentNode())->hasLocalName(tbodyTag);
}

static bool isInFoot(Element* row)
{
    return row->parentNode() && toElement(row->parentNode())->hasLocalName(tfootTag);
}

HTMLTableRowElement* HTMLTableRowsCollection::rowAfter(HTMLTableElement* table, HTMLTableRowElement* previous)
{
    Node* child = 0;

    // Start by looking for the next row in this section.
    // Continue only if there is none.
    if (previous && previous->parentNode() != table) {
        for (child = previous->nextSibling(); child; child = child->nextSibling()) {
            if (child->hasTagName(trTag))
                return static_cast<HTMLTableRowElement*>(child);
        }
    }

    // If still looking at head sections, find the first row in the next head section.
    if (!previous)
        child = table->firstChild();
    else if (isInHead(previous))
        child = previous->parentNode()->nextSibling();
    for (; child; child = child->nextSibling()) {
        if (child->hasTagName(theadTag)) {
            for (Node* grandchild = child->firstChild(); grandchild; grandchild = grandchild->nextSibling()) {
                if (grandchild->hasTagName(trTag))
                    return static_cast<HTMLTableRowElement*>(grandchild);
            }
        }
    }

    // If still looking at top level and bodies, find the next row in top level or the first in the next body section.
    if (!previous || isInHead(previous))
        child = table->firstChild();
    else if (previous->parentNode() == table)
        child = previous->nextSibling();
    else if (isInBody(previous))
        child = previous->parentNode()->nextSibling();
    for (; child; child = child->nextSibling()) {
        if (child->hasTagName(trTag))
            return static_cast<HTMLTableRowElement*>(child);
        if (child->hasTagName(tbodyTag)) {
            for (Node* grandchild = child->firstChild(); grandchild; grandchild = grandchild->nextSibling()) {
                if (grandchild->hasTagName(trTag))
                    return static_cast<HTMLTableRowElement*>(grandchild);
            }
        }
    }

    // Find the first row in the next foot section.
    if (!previous || !isInFoot(previous))
        child = table->firstChild();
    else
        child = previous->parentNode()->nextSibling();
    for (; child; child = child->nextSibling()) {
        if (child->hasTagName(tfootTag)) {
            for (Node* grandchild = child->firstChild(); grandchild; grandchild = grandchild->nextSibling()) {
                if (grandchild->hasTagName(trTag))
                    return static_cast<HTMLTableRowElement*>(grandchild);
            }
        }
    }

    return 0;
}

HTMLTableRowElement* HTMLTableRowsCollection::lastRow(HTMLTableElement* table)
{
    for (Node* child = table->lastChild(); child; child = child->previousSibling()) {
        if (child->hasTagName(tfootTag)) {
            for (Node* grandchild = child->lastChild(); grandchild; grandchild = grandchild->previousSibling()) {
                if (grandchild->hasTagName(trTag))
                    return static_cast<HTMLTableRowElement*>(grandchild);
            }
        }
    }

    for (Node* child = table->lastChild(); child; child = child->previousSibling()) {
        if (child->hasTagName(trTag))
            return static_cast<HTMLTableRowElement*>(child);
        if (child->hasTagName(tbodyTag)) {
            for (Node* grandchild = child->lastChild(); grandchild; grandchild = grandchild->previousSibling()) {
                if (grandchild->hasTagName(trTag))
                    return static_cast<HTMLTableRowElement*>(grandchild);
            }
        }
    }

    for (Node* child = table->lastChild(); child; child = child->previousSibling()) {
        if (child->hasTagName(theadTag)) {
            for (Node* grandchild = child->lastChild(); grandchild; grandchild = grandchild->previousSibling()) {
                if (grandchild->hasTagName(trTag))
                    return static_cast<HTMLTableRowElement*>(grandchild);
            }
        }
    }

    return 0;
}

// Must call get() on the table in case that argument is compiled before dereferencing the
// table to get at the collection cache. Order of argument evaluation is undefined and can
// differ between compilers.
HTMLTableRowsCollection::HTMLTableRowsCollection(Node* table)
    : HTMLCollection(table, TableRows, OverridesItemAfter)
{
    ASSERT(isHTMLTableElement(table));
}

PassRefPtr<HTMLTableRowsCollection> HTMLTableRowsCollection::create(Node* table, CollectionType)
{
    return adoptRef(new HTMLTableRowsCollection(table));
}

Element* HTMLTableRowsCollection::virtualItemAfter(unsigned& offsetInArray, Element* previous) const
{
    ASSERT_UNUSED(offsetInArray, !offsetInArray);
    ASSERT(!previous || (previous->isHTMLElement() && toHTMLElement(previous)->hasLocalName(trTag)));
    return rowAfter(toHTMLTableElement(ownerNode()), static_cast<HTMLTableRowElement*>(previous));
}

}
