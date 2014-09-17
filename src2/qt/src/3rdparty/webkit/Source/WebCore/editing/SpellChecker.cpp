/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "SpellChecker.h"

#include "Document.h"
#include "DocumentMarkerController.h"
#include "EditorClient.h"
#include "Frame.h"
#include "HTMLInputElement.h"
#include "HTMLTextAreaElement.h"
#include "Node.h"
#include "Page.h"
#include "PositionIterator.h"
#include "Range.h"
#include "RenderObject.h"
#include "Settings.h"
#include "TextCheckerClient.h"
#include "TextIterator.h"
#include "htmlediting.h"

namespace WebCore {

SpellChecker::SpellChecker(Frame* frame)
    : m_frame(frame)
    , m_requestSequence(0)
{
}

SpellChecker::~SpellChecker()
{
}

TextCheckerClient* SpellChecker::client() const
{
    Page* page = m_frame->page();
    if (!page)
        return 0;
    return page->editorClient()->textChecker();
}

bool SpellChecker::initRequest(Node* node)
{
    ASSERT(canCheckAsynchronously(node));

    String text = node->textContent();
    if (!text.length())
        return false;

    m_requestNode = node;
    m_requestText = text;
    m_requestSequence++;

    return true;
}

void SpellChecker::clearRequest()
{
    m_requestNode.clear();
    m_requestText = String();
}

bool SpellChecker::isAsynchronousEnabled() const
{
    return m_frame->settings() && m_frame->settings()->asynchronousSpellCheckingEnabled();
}

bool SpellChecker::canCheckAsynchronously(Node* node) const
{
    return client() && isCheckable(node) && isAsynchronousEnabled() && !isBusy();
}

bool SpellChecker::isBusy() const
{
    return m_requestNode.get();
}

bool SpellChecker::isValid(int sequence) const
{
    return m_requestNode.get() && m_requestText.length() && m_requestSequence == sequence;
}

bool SpellChecker::isCheckable(Node* node) const
{
    return node && node->renderer();
}

void SpellChecker::requestCheckingFor(TextCheckingTypeMask mask, Node* node)
{
    ASSERT(canCheckAsynchronously(node));

    if (!initRequest(node))
        return;
    client()->requestCheckingOfString(this, m_requestSequence, mask, m_requestText);
}

static bool forwardIterator(PositionIterator& iterator, int distance)
{
    int remaining = distance;
    while (!iterator.atEnd()) {
        if (iterator.node()->isCharacterDataNode()) {
            int length = lastOffsetForEditing(iterator.node());
            int last = length - iterator.offsetInLeafNode();
            if (remaining < last) {
                iterator.setOffsetInLeafNode(iterator.offsetInLeafNode() + remaining);
                return true;
            }

            remaining -= last;
            iterator.setOffsetInLeafNode(iterator.offsetInLeafNode() + last);
        }

        iterator.increment();
    }

    return false;    
}

static DocumentMarker::MarkerType toMarkerType(TextCheckingType type)
{
    if (type == TextCheckingTypeSpelling)
        return DocumentMarker::Spelling;
    ASSERT(type == TextCheckingTypeGrammar);
    return DocumentMarker::Grammar;
}

// Currenntly ignoring TextCheckingResult::details but should be handled. See Bug 56368.
void SpellChecker::didCheck(int sequence, const Vector<TextCheckingResult>& results)
{
    if (!isValid(sequence))
        return;

    if (!m_requestNode->renderer()) {
        clearRequest();
        return;
    }

    int startOffset = 0;
    PositionIterator start = firstPositionInOrBeforeNode(m_requestNode.get());
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].type != TextCheckingTypeSpelling && results[i].type != TextCheckingTypeGrammar)
            continue;

        // To avoid moving the position backward, we assume the given results are sorted with
        // startOffset as the ones returned by [NSSpellChecker requestCheckingOfString:].
        ASSERT(startOffset <= results[i].location);
        if (!forwardIterator(start, results[i].location - startOffset))
            break;
        PositionIterator end = start;
        if (!forwardIterator(end, results[i].length))
            break;

        // Users or JavaScript applications may change text while a spell-checker checks its
        // spellings in the background. To avoid adding markers to the words modified by users or
        // JavaScript applications, retrieve the words in the specified region and compare them with
        // the original ones.
        RefPtr<Range> range = Range::create(m_requestNode->document(), start, end);
        // FIXME: Use textContent() compatible string conversion.
        String destination = range->text();
        String source = m_requestText.substring(results[i].location, results[i].length);
        if (destination == source)
            m_requestNode->document()->markers()->addMarker(range.get(), toMarkerType(results[i].type));

        startOffset = results[i].location;
    }

    clearRequest();
}


} // namespace WebCore
