/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "HTMLFormattingElementList.h"

#include "Element.h"
#include "NotImplemented.h"

namespace WebCore {

HTMLFormattingElementList::HTMLFormattingElementList()
{
}

HTMLFormattingElementList::~HTMLFormattingElementList()
{
}

Element* HTMLFormattingElementList::closestElementInScopeWithName(const AtomicString& targetName)
{
    for (unsigned i = 1; i <= m_entries.size(); ++i) {
        const Entry& entry = m_entries[m_entries.size() - i];
        if (entry.isMarker())
            return 0;
        if (entry.element()->hasLocalName(targetName))
            return entry.element();
    }
    return 0;
}

bool HTMLFormattingElementList::contains(Element* element)
{
    return !!find(element);
}

HTMLFormattingElementList::Entry* HTMLFormattingElementList::find(Element* element)
{
    size_t index = m_entries.reverseFind(element);
    if (index != notFound) {
        // This is somewhat of a hack, and is why this method can't be const.
        return &m_entries[index];
    }
    return 0;
}

HTMLFormattingElementList::Bookmark HTMLFormattingElementList::bookmarkFor(Element* element)
{
    size_t index = m_entries.reverseFind(element);
    ASSERT(index != notFound);
    return Bookmark(&at(index));
}

void HTMLFormattingElementList::swapTo(Element* oldElement, Element* newElement, const Bookmark& bookmark)
{
    ASSERT(contains(oldElement));
    ASSERT(!contains(newElement));
    if (!bookmark.hasBeenMoved()) {
        ASSERT(bookmark.mark()->element() == oldElement);
        bookmark.mark()->replaceElement(newElement);
        return;
    }
    size_t index = bookmark.mark() - first();
    ASSERT(index < size());
    m_entries.insert(index + 1, newElement);
    remove(oldElement);
}

void HTMLFormattingElementList::append(Element* element)
{
    m_entries.append(element);
}

void HTMLFormattingElementList::remove(Element* element)
{
    size_t index = m_entries.reverseFind(element);
    if (index != notFound)
        m_entries.remove(index);
}

void HTMLFormattingElementList::appendMarker()
{
    m_entries.append(Entry::MarkerEntry);
}

void HTMLFormattingElementList::clearToLastMarker()
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#clear-the-list-of-active-formatting-elements-up-to-the-last-marker
    while (m_entries.size()) {
        bool shouldStop = m_entries.last().isMarker();
        m_entries.removeLast();
        if (shouldStop)
            break;
    }
}

#ifndef NDEBUG

void HTMLFormattingElementList::show()
{
    for (unsigned i = 1; i <= m_entries.size(); ++i) {
        const Entry& entry = m_entries[m_entries.size() - i];
        if (entry.isMarker())
            fprintf(stderr, "marker\n");
        else
            entry.element()->showNode();
    }
}

#endif

}
