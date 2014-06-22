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
#include "HTMLLabelElement.h"
#include "HTMLMapElement.h"
#include "HTMLNameCollection.h"
#include "HTMLNames.h"
#include "NodeTraversal.h"
#include "TreeScope.h"

namespace WebCore {

using namespace HTMLNames;

inline bool keyMatchesId(AtomicStringImpl* key, Element* element)
{
    return element->getIdAttribute().impl() == key;
}

inline bool keyMatchesName(AtomicStringImpl* key, Element* element)
{
    return element->getNameAttribute().impl() == key;
}

inline bool keyMatchesMapName(AtomicStringImpl* key, Element* element)
{
    return isHTMLMapElement(element) && toHTMLMapElement(element)->getName().impl() == key;
}

inline bool keyMatchesLowercasedMapName(AtomicStringImpl* key, Element* element)
{
    return isHTMLMapElement(element) && toHTMLMapElement(element)->getName().lower().impl() == key;
}

inline bool keyMatchesLabelForAttribute(AtomicStringImpl* key, Element* element)
{
    return isHTMLLabelElement(element) && element->getAttribute(forAttr).impl() == key;
}

inline bool keyMatchesWindowNamedItem(AtomicStringImpl* key, Element* element)
{
    return WindowNameCollection::nodeMatches(element, key);
}

inline bool keyMatchesDocumentNamedItem(AtomicStringImpl* key, Element* element)
{
    return DocumentNameCollection::nodeMatches(element, key);
}

void DocumentOrderedMap::clear()
{
    m_map.clear();
}

void DocumentOrderedMap::add(AtomicStringImpl* key, Element* element)
{
    ASSERT(key);
    ASSERT(element);

    Map::AddResult addResult = m_map.add(key, MapEntry(element));
    if (addResult.isNewEntry)
        return;

    MapEntry& entry = addResult.iterator->value;
    ASSERT(entry.count);
    entry.element = 0;
    entry.count++;
    entry.orderedList.clear();
}

void DocumentOrderedMap::remove(AtomicStringImpl* key, Element* element)
{
    ASSERT(key);
    ASSERT(element);

    m_map.checkConsistency();
    Map::iterator it = m_map.find(key);
    ASSERT(it != m_map.end());
    MapEntry& entry = it->value;

    ASSERT(entry.count);
    if (entry.count == 1) {
        ASSERT(!entry.element || entry.element == element);
        m_map.remove(it);
    } else {
        if (entry.element == element)
            entry.element = 0;
        entry.count--;
        entry.orderedList.clear(); // FIXME: Remove the element instead if there are only few items left.
    }
}

template<bool keyMatches(AtomicStringImpl*, Element*)>
inline Element* DocumentOrderedMap::get(AtomicStringImpl* key, const TreeScope* scope) const
{
    ASSERT(key);
    ASSERT(scope);

    m_map.checkConsistency();

    Map::iterator it = m_map.find(key);
    if (it == m_map.end())
        return 0;

    MapEntry& entry = it->value;
    ASSERT(entry.count);
    if (entry.element)
        return entry.element;

    // We know there's at least one node that matches; iterate to find the first one.
    for (Element* element = ElementTraversal::firstWithin(scope->rootNode()); element; element = ElementTraversal::next(element)) {
        if (!keyMatches(key, element))
            continue;
        entry.element = element;
        return element;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

Element* DocumentOrderedMap::getElementById(AtomicStringImpl* key, const TreeScope* scope) const
{
    return get<keyMatchesId>(key, scope);
}

Element* DocumentOrderedMap::getElementByName(AtomicStringImpl* key, const TreeScope* scope) const
{
    return get<keyMatchesName>(key, scope);
}

Element* DocumentOrderedMap::getElementByMapName(AtomicStringImpl* key, const TreeScope* scope) const
{
    return get<keyMatchesMapName>(key, scope);
}

Element* DocumentOrderedMap::getElementByLowercasedMapName(AtomicStringImpl* key, const TreeScope* scope) const
{
    return get<keyMatchesLowercasedMapName>(key, scope);
}

Element* DocumentOrderedMap::getElementByLabelForAttribute(AtomicStringImpl* key, const TreeScope* scope) const
{
    return get<keyMatchesLabelForAttribute>(key, scope);
}

Element* DocumentOrderedMap::getElementByWindowNamedItem(AtomicStringImpl* key, const TreeScope* scope) const
{
    return get<keyMatchesWindowNamedItem>(key, scope);
}

Element* DocumentOrderedMap::getElementByDocumentNamedItem(AtomicStringImpl* key, const TreeScope* scope) const
{
    return get<keyMatchesDocumentNamedItem>(key, scope);
}

const Vector<Element*>* DocumentOrderedMap::getAllElementsById(AtomicStringImpl* key, const TreeScope* scope) const
{
    ASSERT(key);
    ASSERT(scope);

    m_map.checkConsistency();

    Map::iterator it = m_map.find(key);
    if (it == m_map.end())
        return 0;

    MapEntry& entry = it->value;
    ASSERT(entry.count);
    if (!entry.count)
        return 0;

    if (entry.orderedList.isEmpty()) {
        entry.orderedList.reserveCapacity(entry.count);
        for (Element* element = entry.element ? entry.element : ElementTraversal::firstWithin(scope->rootNode()); element; element = ElementTraversal::next(element)) {
            if (!keyMatchesId(key, element))
                continue;
            entry.orderedList.append(element);
        }
        ASSERT(entry.orderedList.size() == entry.count);
    }

    return &entry.orderedList;
}

} // namespace WebCore
