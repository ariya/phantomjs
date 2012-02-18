/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
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
#include "DynamicNodeList.h"

#include "Document.h"
#include "Element.h"

namespace WebCore {

DynamicNodeList::DynamicNodeList(PassRefPtr<Node> rootNode)
    : m_rootNode(rootNode)
    , m_caches(Caches::create())
    , m_ownsCaches(true)
{
    m_rootNode->registerDynamicNodeList(this);
}    

DynamicNodeList::DynamicNodeList(PassRefPtr<Node> rootNode, DynamicNodeList::Caches* caches)
    : m_rootNode(rootNode)
    , m_caches(caches)
    , m_ownsCaches(false)
{
    m_rootNode->registerDynamicNodeList(this);
}    

DynamicNodeList::~DynamicNodeList()
{
    m_rootNode->unregisterDynamicNodeList(this);
}

unsigned DynamicNodeList::length() const
{
    if (m_caches->isLengthCacheValid)
        return m_caches->cachedLength;

    unsigned length = 0;

    for (Node* n = m_rootNode->firstChild(); n; n = n->traverseNextNode(m_rootNode.get()))
        length += n->isElementNode() && nodeMatches(static_cast<Element*>(n));

    m_caches->cachedLength = length;
    m_caches->isLengthCacheValid = true;

    return length;
}

Node* DynamicNodeList::itemForwardsFromCurrent(Node* start, unsigned offset, int remainingOffset) const
{
    ASSERT(remainingOffset >= 0);
    for (Node* n = start; n; n = n->traverseNextNode(m_rootNode.get())) {
        if (n->isElementNode() && nodeMatches(static_cast<Element*>(n))) {
            if (!remainingOffset) {
                m_caches->lastItem = n;
                m_caches->lastItemOffset = offset;
                m_caches->isItemCacheValid = true;
                return n;
            }
            --remainingOffset;
        }
    }

    return 0; // no matching node in this subtree
}

Node* DynamicNodeList::itemBackwardsFromCurrent(Node* start, unsigned offset, int remainingOffset) const
{
    ASSERT(remainingOffset < 0);
    for (Node* n = start; n; n = n->traversePreviousNode(m_rootNode.get())) {
        if (n->isElementNode() && nodeMatches(static_cast<Element*>(n))) {
            if (!remainingOffset) {
                m_caches->lastItem = n;
                m_caches->lastItemOffset = offset;
                m_caches->isItemCacheValid = true;
                return n;
            }
            ++remainingOffset;
        }
    }

    return 0; // no matching node in this subtree
}

Node* DynamicNodeList::item(unsigned offset) const
{
    int remainingOffset = offset;
    Node* start = m_rootNode->firstChild();
    if (m_caches->isItemCacheValid) {
        if (offset == m_caches->lastItemOffset)
            return m_caches->lastItem;
        else if (offset > m_caches->lastItemOffset || m_caches->lastItemOffset - offset < offset) {
            start = m_caches->lastItem;
            remainingOffset -= m_caches->lastItemOffset;
        }
    }

    if (remainingOffset < 0)
        return itemBackwardsFromCurrent(start, offset, remainingOffset);
    return itemForwardsFromCurrent(start, offset, remainingOffset);
}

Node* DynamicNodeList::itemWithName(const AtomicString& elementId) const
{
    if (m_rootNode->isDocumentNode() || m_rootNode->inDocument()) {
        Element* node = m_rootNode->treeScope()->getElementById(elementId);
        if (node && nodeMatches(node)) {
            for (ContainerNode* p = node->parentNode(); p; p = p->parentNode()) {
                if (p == m_rootNode)
                    return node;
            }
        }
        if (!node)
            return 0;
        // In the case of multiple nodes with the same name, just fall through.
    }

    unsigned length = this->length();
    for (unsigned i = 0; i < length; i++) {
        Node* node = item(i);
        // FIXME: This should probably be using getIdAttribute instead of idForStyleResolution.
        if (node->hasID() && static_cast<Element*>(node)->idForStyleResolution() == elementId)
            return node;
    }

    return 0;
}

bool DynamicNodeList::isDynamicNodeList() const
{
    return true;
}

void DynamicNodeList::invalidateCache()
{
    // This should only be called for node lists that own their own caches.
    ASSERT(m_ownsCaches);
    m_caches->reset();
}

DynamicNodeList::Caches::Caches()
    : lastItem(0)
    , isLengthCacheValid(false)
    , isItemCacheValid(false)
{
}

PassRefPtr<DynamicNodeList::Caches> DynamicNodeList::Caches::create()
{
    return adoptRef(new Caches());
}

void DynamicNodeList::Caches::reset()
{
    lastItem = 0;
    isLengthCacheValid = false;
    isItemCacheValid = false;     
}

} // namespace WebCore
