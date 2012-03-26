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

#include "config.h"
#include "CollectionCache.h"

namespace WebCore {

CollectionCache::CollectionCache()
    : version(0)
{
    reset();
}

inline void CollectionCache::copyCacheMap(NodeCacheMap& dest, const NodeCacheMap& src)
{
    ASSERT(dest.isEmpty());
    NodeCacheMap::const_iterator end = src.end();
    for (NodeCacheMap::const_iterator it = src.begin(); it != end; ++it)
        dest.add(it->first, new Vector<Element*>(*it->second));
}

CollectionCache::CollectionCache(const CollectionCache& other)
    : version(other.version)
    , current(other.current)
    , position(other.position)
    , length(other.length)
    , elementsArrayPosition(other.elementsArrayPosition)
    , hasLength(other.hasLength)
    , hasNameCache(other.hasNameCache)
{
    copyCacheMap(idCache, other.idCache);
    copyCacheMap(nameCache, other.nameCache);
}

void CollectionCache::swap(CollectionCache& other)
{
    std::swap(version, other.version);
    std::swap(current, other.current);
    std::swap(position, other.position);
    std::swap(length, other.length);
    std::swap(elementsArrayPosition, other.elementsArrayPosition);

    idCache.swap(other.idCache);
    nameCache.swap(other.nameCache);
    
    std::swap(hasLength, other.hasLength);
    std::swap(hasNameCache, other.hasNameCache);
}

CollectionCache::~CollectionCache()
{
    deleteAllValues(idCache);
    deleteAllValues(nameCache);
}

void CollectionCache::reset()
{
    current = 0;
    position = 0;
    length = 0;
    hasLength = false;
    elementsArrayPosition = 0;
    deleteAllValues(idCache);
    idCache.clear();
    deleteAllValues(nameCache);
    nameCache.clear();
    hasNameCache = false;
}

#if !ASSERT_DISABLED
void CollectionCache::checkConsistency()
{
    idCache.checkConsistency();
    nameCache.checkConsistency();
}
#endif

} // namespace WebCore
