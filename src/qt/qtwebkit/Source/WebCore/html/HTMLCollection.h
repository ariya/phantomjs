/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef HTMLCollection_h
#define HTMLCollection_h

#include "CollectionType.h"
#include "LiveNodeList.h"
#include "ScriptWrappable.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/Vector.h>

namespace WebCore {

class HTMLCollection : public LiveNodeListBase {
public:
    static PassRefPtr<HTMLCollection> create(Node* base, CollectionType);
    virtual ~HTMLCollection();

    // DOM API
    virtual Node* namedItem(const AtomicString& name) const;
    PassRefPtr<NodeList> tags(const String&);

    // Non-DOM API
    virtual bool hasNamedItem(const AtomicString& name) const;
    void namedItems(const AtomicString& name, Vector<RefPtr<Node> >&) const;
    bool isEmpty() const
    {
        if (isLengthCacheValid())
            return !cachedLength();
        if (isItemCacheValid())
            return !cachedItem();
        return !item(0);
    }
    bool hasExactlyOneItem() const
    {
        if (isLengthCacheValid())
            return cachedLength() == 1;
        if (isItemCacheValid())
            return cachedItem() && !cachedItemOffset() && !item(1);
        return item(0) && !item(1);
    }

    virtual Element* virtualItemAfter(unsigned& offsetInArray, Element*) const;

    Element* traverseFirstElement(unsigned& offsetInArray, ContainerNode* root) const;
    Element* traverseForwardToOffset(unsigned offset, Element* currentElement, unsigned& currentOffset, unsigned& offsetInArray, ContainerNode* root) const;

protected:
    HTMLCollection(Node* base, CollectionType, ItemAfterOverrideType);

    virtual void updateNameCache() const;

    typedef HashMap<AtomicStringImpl*, OwnPtr<Vector<Element*> > > NodeCacheMap;
    Vector<Element*>* idCache(const AtomicString& name) const { return m_idCache.get(name.impl()); }
    Vector<Element*>* nameCache(const AtomicString& name) const { return m_nameCache.get(name.impl()); }
    void appendIdCache(const AtomicString& name, Element* element) const { append(m_idCache, name, element); }
    void appendNameCache(const AtomicString& name, Element* element) const { append(m_nameCache, name, element); }

private:
    Element* traverseNextElement(unsigned& offsetInArray, Element* previous, ContainerNode* root) const;

    virtual bool isLiveNodeList() const OVERRIDE { ASSERT_NOT_REACHED(); return true; }

    static void append(NodeCacheMap&, const AtomicString&, Element*);

    mutable NodeCacheMap m_idCache;
    mutable NodeCacheMap m_nameCache;
    mutable unsigned m_cachedElementsArrayOffset;

    friend class LiveNodeListBase;
};

} // namespace

#endif
