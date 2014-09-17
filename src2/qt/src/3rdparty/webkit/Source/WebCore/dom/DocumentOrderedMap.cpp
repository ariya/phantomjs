/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DocumentOrderedMap.h"

#include "Element.h"
#include "HTMLMapElement.h"
#include "HTMLNames.h"
#include "TreeScope.h"

namespace WebCore {

using namespace HTMLNames;

inline bool keyMatchesId(AtomicStringImpl* key, Element* element)
{
    return element->hasID() && element->getIdAttribute().impl() == key;
}

inline bool keyMatchesMapName(AtomicStringImpl* key, Element* element)
{
    return element->hasTagName(mapTag) && static_cast<HTMLMapElement*>(element)->getName().impl() == key;
}

inline bool keyMatchesLowercasedMapName(AtomicStringImpl* key, Element* element)
{
    return element->hasTagName(mapTag) && static_cast<HTMLMapElement*>(element)->getName().lower().impl() == key;
}

void DocumentOrderedMap::clear()
{
    m_map.clear();
    m_duplicateCounts.clear();
}

void DocumentOrderedMap::add(AtomicStringImpl* key, Element* element)
{
    ASSERT(key);
    ASSERT(element);

    if (!m_duplicateCounts.contains(key)) {
        // Fast path. The key is not already in m_duplicateCounts, so we assume that it's
        // also not already in m_map and try to add it. If that add succeeds, we're done.
        pair<Map::iterator, bool> addResult = m_map.add(key, element);
        if (addResult.second)
            return;

        // The add failed, so this key was already cached in m_map.
        // There are multiple elements with this key. Remove the m_map
        // cache for this key so get searches for it next time it is called.
        m_map.remove(addResult.first);
        m_duplicateCounts.add(key);
    } else {
        // There are multiple elements with this key. Remove the m_map
        // cache for this key so get will search for it next time it is called.
        Map::iterator cachedItem = m_map.find(key);
        if (cachedItem != m_map.end()) {
            m_map.remove(cachedItem);
            m_duplicateCounts.add(key);
        }
    }

    m_duplicateCounts.add(key);
}

void DocumentOrderedMap::remove(AtomicStringImpl* key, Element* element)
{
    ASSERT(key);
    ASSERT(element);

    m_map.checkConsistency();
    Map::iterator cachedItem = m_map.find(key);
    if (cachedItem != m_map.end() && cachedItem->second == element)
        m_map.remove(cachedItem);
    else
        m_duplicateCounts.remove(key);
}

template<bool keyMatches(AtomicStringImpl*, Element*)>
inline Element* DocumentOrderedMap::get(AtomicStringImpl* key, const TreeScope* scope) const
{
    ASSERT(key);

    m_map.checkConsistency();

    Element* element = m_map.get(key);
    if (element)
        return element;

    if (m_duplicateCounts.contains(key)) {
        // We know there's at least one node that matches; iterate to find the first one.
        for (Node* node = scope->firstChild(); node; node = node->traverseNextNode()) {
            if (!node->isElementNode())
                continue;
            element = static_cast<Element*>(node);
            if (!keyMatches(key, element))
                continue;
            m_duplicateCounts.remove(key);
            m_map.set(key, element);
            return element;
        }
        ASSERT_NOT_REACHED();
    }

    return 0;
}

Element* DocumentOrderedMap::getElementById(AtomicStringImpl* key, const TreeScope* scope) const
{
    return get<keyMatchesId>(key, scope);
}

Element* DocumentOrderedMap::getElementByMapName(AtomicStringImpl* key, const TreeScope* scope) const
{
    return get<keyMatchesMapName>(key, scope);
}

Element* DocumentOrderedMap::getElementByLowercasedMapName(AtomicStringImpl* key, const TreeScope* scope) const
{
    return get<keyMatchesLowercasedMapName>(key, scope);
}

} // namespace WebCore

