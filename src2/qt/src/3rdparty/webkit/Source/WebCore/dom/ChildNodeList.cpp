/**
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2007, 2008 Apple Inc. All rights reserved.
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
#include "ChildNodeList.h"

#include "Element.h"

namespace WebCore {

ChildNodeList::ChildNodeList(PassRefPtr<Node> rootNode, DynamicNodeList::Caches* info)
    : DynamicNodeList(rootNode, info)
{
}

unsigned ChildNodeList::length() const
{
    if (m_caches->isLengthCacheValid)
        return m_caches->cachedLength;

    unsigned len = 0;
    for (Node* n = m_rootNode->firstChild(); n; n = n->nextSibling())
        len++;

    m_caches->cachedLength = len;
    m_caches->isLengthCacheValid = true;

    return len;
}

Node* ChildNodeList::item(unsigned index) const
{
    unsigned int pos = 0;
    Node* n = m_rootNode->firstChild();

    if (m_caches->isItemCacheValid) {
        if (index == m_caches->lastItemOffset)
            return m_caches->lastItem;
        
        int diff = index - m_caches->lastItemOffset;
        unsigned dist = abs(diff);
        if (dist < index) {
            n = m_caches->lastItem;
            pos = m_caches->lastItemOffset;
        }
    }

    if (m_caches->isLengthCacheValid) {
        if (index >= m_caches->cachedLength)
            return 0;

        int diff = index - pos;
        unsigned dist = abs(diff);
        if (dist > m_caches->cachedLength - 1 - index) {
            n = m_rootNode->lastChild();
            pos = m_caches->cachedLength - 1;
        }
    }

    if (pos <= index) {
        while (n && pos < index) {
            n = n->nextSibling();
            ++pos;
        }
    } else {
        while (n && pos > index) {
            n = n->previousSibling();
            --pos;
        }
    }

    if (n) {
        m_caches->lastItem = n;
        m_caches->lastItemOffset = pos;
        m_caches->isItemCacheValid = true;
        return n;
    }

    return 0;
}

bool ChildNodeList::nodeMatches(Element* testNode) const
{
    // Note: Due to the overrides of the length and item functions above,
    // this function will be called only by DynamicNodeList::itemWithName,
    // for an element that was located with getElementById.
    return testNode->parentNode() == m_rootNode;
}

} // namespace WebCore
