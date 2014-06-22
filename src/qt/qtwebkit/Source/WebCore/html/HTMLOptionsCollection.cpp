/*
 * Copyright (C) 2006, 2011, 2012 Apple Computer, Inc.
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
#include "HTMLOptionsCollection.h"

#include "ExceptionCode.h"
#include "HTMLOptionElement.h"
#include "HTMLSelectElement.h"

namespace WebCore {

HTMLOptionsCollection::HTMLOptionsCollection(Node* select)
    : HTMLCollection(select, SelectOptions, DoesNotOverrideItemAfter)
{
    ASSERT(select->hasTagName(HTMLNames::selectTag));
}

PassRefPtr<HTMLOptionsCollection> HTMLOptionsCollection::create(Node* select, CollectionType)
{
    return adoptRef(new HTMLOptionsCollection(select));
}

void HTMLOptionsCollection::add(PassRefPtr<HTMLOptionElement> element, ExceptionCode& ec)
{
    add(element, length(), ec);
}

void HTMLOptionsCollection::add(PassRefPtr<HTMLOptionElement> element, int index, ExceptionCode& ec)
{
    HTMLOptionElement* newOption = element.get();

    if (!newOption) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    if (index < -1) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    ec = 0;
    HTMLSelectElement* select = toHTMLSelectElement(ownerNode());

    if (index == -1 || unsigned(index) >= length())
        select->add(newOption, 0, ec);
    else
        select->add(newOption, toHTMLOptionElement(item(index)), ec);

    ASSERT(!ec);
}

void HTMLOptionsCollection::remove(int index)
{
    toHTMLSelectElement(ownerNode())->remove(index);
}

int HTMLOptionsCollection::selectedIndex() const
{
    return toHTMLSelectElement(ownerNode())->selectedIndex();
}

void HTMLOptionsCollection::setSelectedIndex(int index)
{
    toHTMLSelectElement(ownerNode())->setSelectedIndex(index);
}

void HTMLOptionsCollection::setLength(unsigned length, ExceptionCode& ec)
{
    toHTMLSelectElement(ownerNode())->setLength(length, ec);
}

} //namespace
