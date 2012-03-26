/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef CollectionCache_h
#define CollectionCache_h

#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/Vector.h>

namespace WebCore {

class Element;

struct CollectionCache {
    WTF_MAKE_FAST_ALLOCATED;
public:
    CollectionCache();
    CollectionCache(const CollectionCache&);
    CollectionCache& operator=(const CollectionCache& other)
    {
        CollectionCache tmp(other);    
        swap(tmp);
        return *this;
    }
    ~CollectionCache();

    void reset();
    void swap(CollectionCache&);

    void checkConsistency();

    typedef HashMap<AtomicStringImpl*, Vector<Element*>*> NodeCacheMap;

    uint64_t version;
    Element* current;
    unsigned position;
    unsigned length;
    int elementsArrayPosition;
    NodeCacheMap idCache;
    NodeCacheMap nameCache;
    bool hasLength;
    bool hasNameCache;

private:
    static void copyCacheMap(NodeCacheMap&, const NodeCacheMap&);
};

#if ASSERT_DISABLED
    inline void CollectionCache::checkConsistency() { }
#endif

} // namespace

#endif
