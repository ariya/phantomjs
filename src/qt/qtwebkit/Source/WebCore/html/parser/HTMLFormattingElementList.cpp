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

#ifndef NDEBUG
#include <stdio.h>
#endif

namespace WebCore {

// Biblically, Noah's Ark only had room for two of each animal, but in the
// Book of Hixie (aka http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#list-of-active-formatting-elements),
// Noah's Ark of Formatting Elements can fit three of each element.
static const size_t kNoahsArkCapacity = 3;

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
        if (entry.stackItem()->matchesHTMLTag(targetName))
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

void HTMLFormattingElementList::swapTo(Element* oldElement, PassRefPtr<HTMLStackItem> newItem, const Bookmark& bookmark)
{
    ASSERT(contains(oldElement));
    ASSERT(!contains(newItem->element()));
    if (!bookmark.hasBeenMoved()) {
        ASSERT(bookmark.mark()->element() == oldElement);
        bookmark.mark()->replaceElement(newItem);
        return;
    }
    size_t index = bookmark.mark() - first();
    ASSERT_WITH_SECURITY_IMPLICATION(index < size());
    m_entries.insert(index + 1, newItem);
    remove(oldElement);
}

void HTMLFormattingElementList::append(PassRefPtr<HTMLStackItem> item)
{
    ensureNoahsArkCondition(item.get());
    m_entries.append(item);
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

void HTMLFormattingElementList::tryToEnsureNoahsArkConditionQuickly(HTMLStackItem* newItem, Vector<HTMLStackItem*>& remainingCandidates)
{
    ASSERT(remainingCandidates.isEmpty());

    if (m_entries.size() < kNoahsArkCapacity)
        return;

    // Use a vector with inline capacity to avoid a malloc in the common case
    // of a quickly ensuring the condition.
    Vector<HTMLStackItem*, 10> candidates;

    size_t newItemAttributeCount = newItem->attributes().size();

    for (size_t i = m_entries.size(); i; ) {
        --i;
        Entry& entry = m_entries[i];
        if (entry.isMarker())
            break;

        // Quickly reject obviously non-matching candidates.
        HTMLStackItem* candidate = entry.stackItem().get();
        if (newItem->localName() != candidate->localName() || newItem->namespaceURI() != candidate->namespaceURI())
            continue;
        if (candidate->attributes().size() != newItemAttributeCount)
            continue;

        candidates.append(candidate);
    }

    if (candidates.size() < kNoahsArkCapacity)
        return; // There's room for the new element in the ark. There's no need to copy out the remainingCandidates.

    remainingCandidates.appendVector(candidates);
}

void HTMLFormattingElementList::ensureNoahsArkCondition(HTMLStackItem* newItem)
{
    Vector<HTMLStackItem*> candidates;
    tryToEnsureNoahsArkConditionQuickly(newItem, candidates);
    if (candidates.isEmpty())
        return;

    // We pre-allocate and re-use this second vector to save one malloc per
    // attribute that we verify.
    Vector<HTMLStackItem*> remainingCandidates;
    remainingCandidates.reserveInitialCapacity(candidates.size());

    const Vector<Attribute>& attributes = newItem->attributes();
    for (size_t i = 0; i < attributes.size(); ++i) {
        const Attribute& attribute = attributes[i];

        for (size_t j = 0; j < candidates.size(); ++j) {
            HTMLStackItem* candidate = candidates[j];

            // These properties should already have been checked by tryToEnsureNoahsArkConditionQuickly.
            ASSERT(newItem->attributes().size() == candidate->attributes().size());
            ASSERT(newItem->localName() == candidate->localName() && newItem->namespaceURI() == candidate->namespaceURI());

            Attribute* candidateAttribute = candidate->getAttributeItem(attribute.name());
            if (candidateAttribute && candidateAttribute->value() == attribute.value())
                remainingCandidates.append(candidate);
        }

        if (remainingCandidates.size() < kNoahsArkCapacity)
            return;

        candidates.swap(remainingCandidates);
        remainingCandidates.shrink(0);
    }

    // Inductively, we shouldn't spin this loop very many times. It's possible,
    // however, that we wil spin the loop more than once because of how the
    // formatting element list gets permuted.
    for (size_t i = kNoahsArkCapacity - 1; i < candidates.size(); ++i)
        remove(candidates[i]->element());
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
