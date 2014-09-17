/**
 * Copyright (C) 2011 Nokia Inc.  All rights reserved.
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
#include "RenderQuote.h"

#include "Document.h"
#include "Element.h"
#include "HTMLElement.h"
#include "QuotesData.h"
#include "RenderStyle.h"
#include <algorithm>
#include <wtf/text/AtomicString.h>
#include <wtf/text/CString.h>

#define UNKNOWN_DEPTH -1

namespace WebCore {
static inline void adjustDepth(int &depth, QuoteType type)
{
    switch (type) {
    case OPEN_QUOTE:
    case NO_OPEN_QUOTE:
        ++depth;
        break;
    case CLOSE_QUOTE:
    case NO_CLOSE_QUOTE:
        if (depth)
            --depth;
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

RenderQuote::RenderQuote(Document* node, QuoteType quote)
    : RenderText(node, StringImpl::empty())
    , m_type(quote)
    , m_depth(UNKNOWN_DEPTH)
    , m_next(0)
    , m_previous(0)
{
}

RenderQuote::~RenderQuote()
{
}

const char* RenderQuote::renderName() const
{
    return "RenderQuote";
}

// This function places a list of quote renderers starting at "this" in the list of quote renderers already
// in the document's renderer tree.
// The assumptions are made (for performance):
// 1. The list of quotes already in the renderers tree of the document is already in a consistent state
// (All quote renderers are linked and have the correct depth set)
// 2. The quote renderers of the inserted list are in a tree of renderers of their own which has been just
// inserted in the main renderer tree with its root as child of some renderer.
// 3. The quote renderers in the inserted list have depths consistent with their position in the list relative
// to "this", thus if "this" does not need to change its depth upon insertion, the other renderers in the list don't
// need to either.
void RenderQuote::placeQuote()
{
    RenderQuote* head = this;
    ASSERT(!head->m_previous);
    RenderQuote* tail = 0;
    for (RenderObject* predecessor = head->previousInPreOrder(); predecessor; predecessor = predecessor->previousInPreOrder()) {
        if (!predecessor->isQuote())
            continue;
        head->m_previous = toRenderQuote(predecessor);
        if (head->m_previous->m_next) {
            // We need to splice the list of quotes headed by head into the document's list of quotes.
            tail = head;
            while (tail->m_next)
                 tail = tail->m_next;
            tail->m_next = head->m_previous->m_next;
            ASSERT(tail->m_next->m_previous == head->m_previous);
            tail->m_next->m_previous =  tail;
            tail = tail->m_next; // This marks the splicing point here there may be a depth discontinuity
        }
        head->m_previous->m_next = head;
        ASSERT(head->m_previous->m_depth != UNKNOWN_DEPTH);
        break;
    }
    int newDepth;
    if (!head->m_previous) {
        newDepth = 0;
        goto skipNewDepthCalc;
    }
    newDepth = head->m_previous->m_depth;
    do {
        adjustDepth(newDepth, head->m_previous->m_type);
skipNewDepthCalc:
        if (head->m_depth == newDepth) { // All remaining depth should be correct except if splicing was done.
            if (!tail) // We've done the post splicing section already or there was no splicing.
                break;
            head = tail; // Continue after the splicing point
            tail = 0; // Mark the possible splicing point discontinuity fixed.
            newDepth = head->m_previous->m_depth;
            continue;
        }
        head->m_depth = newDepth;
        // FIXME: If the width and height of the quotation characters does not change we may only need to
        // Invalidate the renderer's area not a relayout.
        head->setNeedsLayoutAndPrefWidthsRecalc();
        head = head->m_next;
        if (head == tail) // We are at the splicing point
            tail = 0; // Mark the possible depth discontinuity fixed.
    } while (head);
}

#define ARRAY_SIZE(Carray) (sizeof(Carray) / sizeof(*Carray))
#define LANGUAGE_DATA(name, languageSourceArray) { name, languageSourceArray, ARRAY_SIZE(languageSourceArray) }
#define U(x) ((const UChar*)L##x)

static const UChar* simpleQuotes[] = {U("\""), U("\""), U("'"), U("'")};

static const UChar* englishQuotes[] = {U("\x201C"), U("\x201D"), U("\x2018"), U("\x2019")};
static const UChar* norwegianQuotes[] = { U("\x00AB"), U("\x00BB"), U("\x2039"), U("\x203A") };
static const UChar* romanianQuotes[] = { U("\x201E"), U("\x201D")};
static const UChar* russianQuotes[] = { U("\x00AB"), U("\x00BB"), U("\x201E"), U("\x201C") };
#undef U

struct LanguageData {
    const char *name;
    const UChar* const* const array;
    const int arraySize;
    bool operator<(const LanguageData& compareTo) const
    {
        return strcmp(name, compareTo.name);
    }
};

// Data mast be alphabetically sorted and in all lower case for fast comparison
LanguageData languageData[] = {
    LANGUAGE_DATA("en", englishQuotes),
    LANGUAGE_DATA("no", norwegianQuotes),
    LANGUAGE_DATA("ro", romanianQuotes),
    LANGUAGE_DATA("ru", russianQuotes)
};
#undef LANGUAGE_DATA
const LanguageData* const languageDataEnd = languageData + ARRAY_SIZE(languageData);

#define defaultLanguageQuotesSource simpleQuotes
#define defaultLanguageQuotesCount ARRAY_SIZE(defaultLanguageQuotesSource)

static QuotesData* defaultLanguageQuotesValue = 0;
static const QuotesData* defaultLanguageQuotes()
{
    if (!defaultLanguageQuotesValue) {
        defaultLanguageQuotesValue = QuotesData::create(defaultLanguageQuotesCount);
        if (!defaultLanguageQuotesValue)
            return 0;
        String* data = defaultLanguageQuotesValue->data();
        for (size_t i = 0; i < defaultLanguageQuotesCount; ++i)
            data[i] = defaultLanguageQuotesSource[i];
    }
    return defaultLanguageQuotesValue;
}
#undef defaultLanguageQuotesSource
#undef defaultLanguageQuotesCount

typedef HashMap<RefPtr<AtomicStringImpl>, QuotesData* > QuotesMap;

static QuotesMap& quotesMap()
{
    DEFINE_STATIC_LOCAL(QuotesMap, staticQuotesMap, ());
    return staticQuotesMap;
}

static const QuotesData* quotesForLanguage(AtomicStringImpl* language)
{
    QuotesData* returnValue;
    AtomicString lower(language->lower());
    returnValue = quotesMap().get(lower.impl());
    if (returnValue)
        return returnValue;
    CString s(static_cast<const String&>(lower).ascii());
    LanguageData request = { s.buffer()->data(), 0, 0 };
    const LanguageData* lowerBound = std::lower_bound<const LanguageData*, const LanguageData>(languageData, languageDataEnd, request);
    if (lowerBound == languageDataEnd)
        return defaultLanguageQuotes();
    if (strncmp(lowerBound->name, request.name, strlen(lowerBound->name)))
        return defaultLanguageQuotes();
    returnValue = QuotesData::create(lowerBound->arraySize);
    if (!returnValue)
        return defaultLanguageQuotes();
    String* data = returnValue->data();
    for (int i = 0; i < lowerBound->arraySize; ++i)
        data[i] = lowerBound->array[i];
    quotesMap().set(lower.impl(), returnValue);
    return returnValue;
}
#undef ARRAY_SIZE

static const QuotesData* defaultQuotes(const RenderObject* object)
{
    DEFINE_STATIC_LOCAL(String, langString, ("lang"));
    Node* node =  object->generatingNode();
    Element* element;
    if (!node) {
        element = object->document()->body();
        if (!element)
            element = object->document()->documentElement();
    } else if (!node->isElementNode()) {
        element = node->parentElement();
        if (!element)
            return defaultLanguageQuotes();
    } else
      element = toElement(node);
    const AtomicString* language;
    while ((language = &element->getAttribute(langString)) && language->isNull()) {
        element = element->parentElement();
        if (!element)
            return defaultLanguageQuotes();
    }
    return quotesForLanguage(language->impl());
}

PassRefPtr<StringImpl> RenderQuote::originalText() const
{
    if (!parent())
        return 0;
    ASSERT(m_depth != UNKNOWN_DEPTH);
    const QuotesData* quotes = style()->quotes();
    if (!quotes)
        quotes = defaultQuotes(this);
    if (!quotes->length)
        return emptyAtom.impl();
    int index = m_depth * 2;
    switch (m_type) {
    case NO_OPEN_QUOTE:
    case NO_CLOSE_QUOTE:
        return String("").impl();
    case CLOSE_QUOTE:
        if (index)
            --index;
        else
            ++index;
        break;
    case OPEN_QUOTE:
        break;
    default:
        ASSERT_NOT_REACHED();
        return emptyAtom.impl();
    }
    if (index >= quotes->length)
        index = (quotes->length-2) | (index & 1);
    if (index < 0)
        return emptyAtom.impl();
    return quotes->data()[index].impl();
}

void RenderQuote::computePreferredLogicalWidths(float lead)
{
    setTextInternal(originalText());
    RenderText::computePreferredLogicalWidths(lead);
}

void RenderQuote::rendererSubtreeAttached(RenderObject* renderer)
{
    if (renderer->documentBeingDestroyed())
        return;
    for (RenderObject* descendant = renderer; descendant; descendant = descendant->nextInPreOrder(renderer))
        if (descendant->isQuote()) {
            toRenderQuote(descendant)->placeQuote();
            break;
        }
}

void RenderQuote::rendererRemovedFromTree(RenderObject* subtreeRoot)
{
    if (subtreeRoot->documentBeingDestroyed())
        return;
    for (RenderObject* descendant = subtreeRoot; descendant; descendant = descendant->nextInPreOrder(subtreeRoot))
        if (descendant->isQuote()) {
            RenderQuote* removedQuote = toRenderQuote(descendant);
            RenderQuote* lastQuoteBefore = removedQuote->m_previous;
            removedQuote->m_previous = 0;
            int depth = removedQuote->m_depth;
            for (descendant = descendant->nextInPreOrder(subtreeRoot); descendant; descendant = descendant->nextInPreOrder(subtreeRoot))
                if (descendant->isQuote())
                    removedQuote = toRenderQuote(descendant);
            RenderQuote* quoteAfter = removedQuote->m_next;
            removedQuote->m_next = 0;
            if (lastQuoteBefore)
                lastQuoteBefore->m_next = quoteAfter;
            if (quoteAfter) {
                quoteAfter->m_previous = lastQuoteBefore;
                do {
                    if (depth == quoteAfter->m_depth)
                        break;
                    quoteAfter->m_depth = depth;
                    quoteAfter->setNeedsLayoutAndPrefWidthsRecalc();
                    adjustDepth(depth, quoteAfter->m_type);
                    quoteAfter = quoteAfter->m_next;
                } while (quoteAfter);
            }
            break;
        }
}

void RenderQuote::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    const QuotesData* newQuotes = style()->quotes();
    const QuotesData* oldQuotes = oldStyle ? oldStyle->quotes() : 0;
    if (!((newQuotes && oldQuotes && (*newQuotes == *oldQuotes)) || (!newQuotes && !oldQuotes)))
        setNeedsLayoutAndPrefWidthsRecalc();
    RenderText::styleDidChange(diff, oldStyle);
}

} // namespace WebCore
