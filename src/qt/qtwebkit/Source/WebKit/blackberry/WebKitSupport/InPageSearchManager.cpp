/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "InPageSearchManager.h"

#include "DOMSupport.h"
#include "Document.h"
#include "DocumentMarkerController.h"
#include "Editor.h"
#include "Frame.h"
#include "Node.h"
#include "Page.h"
#include "Range.h"
#include "ShadowRoot.h"
#include "TextIterator.h"
#include "Timer.h"
#include "WebPage_p.h"

static const double MaxScopingDuration = 0.1;

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

class InPageSearchManager::DeferredScopeStringMatches {
public:
    DeferredScopeStringMatches(InPageSearchManager* ipsm, Frame* scopingFrame, const String& text, bool reset, bool locateActiveMatchOnly)
    : m_searchManager(ipsm)
    , m_scopingFrame(scopingFrame)
    , m_timer(this, &DeferredScopeStringMatches::doTimeout)
    , m_searchText(text)
    , m_reset(reset)
    , m_locateActiveMatchOnly(locateActiveMatchOnly)
    {
        m_timer.startOneShot(0.0);
    }

private:
    friend class InPageSearchManager;
    void doTimeout(Timer<DeferredScopeStringMatches>*)
    {
        m_searchManager->callScopeStringMatches(this, m_scopingFrame, m_searchText, m_reset, m_locateActiveMatchOnly);
    }
    InPageSearchManager* m_searchManager;
    Frame* m_scopingFrame;
    Timer<DeferredScopeStringMatches> m_timer;
    String m_searchText;
    bool m_reset;
    bool m_locateActiveMatchOnly;
};

InPageSearchManager::InPageSearchManager(WebPagePrivate* page)
    : m_webPage(page)
    , m_activeMatch(0)
    , m_resumeScopingFromRange(0)
    , m_activeMatchCount(0)
    , m_scopingComplete(false)
    , m_scopingCaseInsensitive(false)
    , m_locatingActiveMatch(false)
    , m_highlightAllMatches(false)
    , m_activeMatchIndex(0)
{
}

InPageSearchManager::~InPageSearchManager()
{
    cancelPendingScopingEffort();
}

bool InPageSearchManager::findNextString(const String& text, FindOptions findOptions, bool wrap, bool highlightAllMatches, bool selectActiveMatchOnClear)
{
    bool highlightAllMatchesStateChanged = m_highlightAllMatches != highlightAllMatches;
    m_highlightAllMatches = highlightAllMatches;

    if (!text.length()) {
        clearTextMatches(selectActiveMatchOnClear);
        cancelPendingScopingEffort();
        m_activeSearchString = String();
        m_webPage->m_client->updateFindStringResult(m_activeMatchCount, m_activeMatchIndex);
        return false;
    }

    if (!shouldSearchForText(text)) {
        m_activeSearchString = text;
        m_webPage->m_client->updateFindStringResult(m_activeMatchCount, m_activeMatchIndex);
        return false;
    }

    // Validate the range in case any node has been removed since last search.
    if (m_activeMatch && !m_activeMatch->boundaryPointsValid())
        m_activeMatch = 0;

    ExceptionCode ec = 0;
    RefPtr<Range> searchStartingPoint = m_activeMatch ? m_activeMatch->cloneRange(ec) : 0;
    bool newSearch = highlightAllMatchesStateChanged || (m_activeSearchString != text);
    bool forward = !(findOptions & WebCore::Backwards);
    if (newSearch) { // Start a new search.
        m_activeSearchString = text;
        cancelPendingScopingEffort();
        m_scopingCaseInsensitive = findOptions & CaseInsensitive;
        m_webPage->m_page->unmarkAllTextMatches();
    } else {
        // Searching for same string should start from the end of last match.
        if (m_activeMatch) {
            if (forward)
                searchStartingPoint->setStart(searchStartingPoint->endPosition());
            else
                searchStartingPoint->setEnd(searchStartingPoint->startPosition());
        }
    }

    // If there is any active selection, new search should start from the beginning of it.
    bool startFromSelection = false;
    VisibleSelection selection = m_webPage->focusedOrMainFrame()->selection()->selection();
    if (!selection.isNone()) {
        searchStartingPoint = selection.firstRange().get();
        m_webPage->focusedOrMainFrame()->selection()->clear();
        startFromSelection = true;
    }

    Frame* currentActiveMatchFrame = selection.isNone() && m_activeMatch ? m_activeMatch->ownerDocument()->frame() : m_webPage->focusedOrMainFrame();

    if (findAndMarkText(text, searchStartingPoint.get(), currentActiveMatchFrame, findOptions, newSearch, startFromSelection))
        return true;

    Frame* startFrame = currentActiveMatchFrame;
    do {
        currentActiveMatchFrame = DOMSupport::incrementFrame(currentActiveMatchFrame, forward, wrap);

        if (!currentActiveMatchFrame) {
            // We should only ever have a null frame if we haven't found any
            // matches and we're not wrapping. We have searched every frame.
            ASSERT(!wrap);
            m_webPage->m_client->updateFindStringResult(m_activeMatchCount, m_activeMatchIndex);
            return false;
        }

        if (findAndMarkText(text, 0, currentActiveMatchFrame, findOptions, newSearch, startFromSelection))
            return true;
    } while (startFrame != currentActiveMatchFrame);

    clearTextMatches();

    m_webPage->m_client->updateFindStringResult(m_activeMatchCount, m_activeMatchIndex);
    return false;
}

bool InPageSearchManager::shouldSearchForText(const String& text)
{
    if (text == m_activeSearchString)
        return m_activeMatchCount;

    // If the previous search string is prefix of new search string,
    // don't search if the previous one has zero result.
    if (m_scopingComplete
        && !m_activeMatchCount
        && m_activeSearchString.length()
        && text.length() > m_activeSearchString.length()
        && m_activeSearchString == text.substring(0, m_activeSearchString.length()))
        return false;
    return true;
}

bool InPageSearchManager::findAndMarkText(const String& text, Range* range, Frame* frame, const FindOptions& options, bool isNewSearch, bool startFromSelection)
{
    if (RefPtr<Range> match = frame->editor().findStringAndScrollToVisible(text, range, options)) {
        // Move the highlight to the new match.
        setActiveMatchAndMarker(match);
        if (isNewSearch) {
            scopeStringMatches(text, true /* reset */, false /* locateActiveMatchOnly */);
            if (!m_highlightAllMatches) {
                // Not highlighting all matches, we need to add the marker here,
                // because scopeStringMatches does not add any markers, it only counts the number.
                // No need to unmarkAllTextMatches, it is already done from the caller because of newSearch
                m_activeMatch->ownerDocument()->markers()->addTextMatchMarker(m_activeMatch.get(), true);
                frame->editor().setMarkedTextMatchesAreHighlighted(true /* highlight */);
            }
            return true;
        }
        if (startFromSelection || m_locatingActiveMatch) {
            // We are finding next, but
            // - starting from a new node, or
            // - last locating active match effort is not done yet
            if (!m_scopingComplete) {
                // Last scoping is not done yet, let's restart it.
                scopeStringMatches(text, true /* reset */, false /* locateActiveMatchOnly */);
            } else {
                // Last scoping is done, but we are jumping to somewhere instead of
                // searching one by one, or there is another locating active match effort,
                // let's start a scoping effort to locate active match only.
                scopeStringMatches(text, true /* reset */, true /* locateActiveMatchOnly */);
            }
        } else {
            // We are finding next one by one, let's calculate active match index
            // There is at least one match, because otherwise we won't get into this block,
            // so m_activeMatchIndex is at least one.
            ASSERT(m_activeMatchCount);
            if (!(options & WebCore::Backwards))
                m_activeMatchIndex = m_activeMatchIndex + 1 > m_activeMatchCount ? 1 : m_activeMatchIndex + 1;
            else
                m_activeMatchIndex = m_activeMatchIndex - 1 < 1 ? m_activeMatchCount : m_activeMatchIndex - 1;
            m_webPage->m_client->updateFindStringResult(m_activeMatchCount, m_activeMatchIndex);
        }
        if (!m_highlightAllMatches) {
            // When only showing single matches, the scoping effort won't highlight
            // all matches but count them.
            m_webPage->m_page->unmarkAllTextMatches();
            m_activeMatch->ownerDocument()->markers()->addTextMatchMarker(m_activeMatch.get(), true);
            frame->editor().setMarkedTextMatchesAreHighlighted(true /* highlight */);
        }

        return true;
    }
    return false;
}

void InPageSearchManager::clearTextMatches(bool selectActiveMatchOnClear)
{
    if (selectActiveMatchOnClear && m_activeMatch.get()) {
        VisibleSelection selection(m_activeMatch.get());
        m_activeMatch->ownerDocument()->frame()->selection()->setSelection(selection);
    }
    m_webPage->m_page->unmarkAllTextMatches();
    m_activeMatch = 0;
    m_activeMatchCount = 0;
    m_activeMatchIndex = 0;
}

void InPageSearchManager::setActiveMatchAndMarker(PassRefPtr<Range> range)
{
    // Clear the old marker, update our range, and highlight the new range.
    if (m_activeMatch.get()) {
        if (Document* doc = m_activeMatch->ownerDocument())
            doc->markers()->setMarkersActive(m_activeMatch.get(), false);
    }

    m_activeMatch = range;
    if (m_activeMatch.get()) {
        if (Document* doc = m_activeMatch->ownerDocument())
            doc->markers()->setMarkersActive(m_activeMatch.get(), true);
    }
}

void InPageSearchManager::frameUnloaded(const Frame* frame)
{
    for (size_t i = 0; i < m_deferredScopingWork.size(); i++) {
        if (m_deferredScopingWork[i]->m_scopingFrame == frame) {
            // Clear pending scoping efforts in case of dangling pointer.
            cancelPendingScopingEffort();
            break;
        }
    }
    if (!m_activeMatch) {
        if (m_webPage->mainFrame() == frame && m_activeSearchString.length())
            m_activeSearchString = String();
        return;
    }

    Frame* currentActiveMatchFrame = m_activeMatch->ownerDocument()->frame();
    if (currentActiveMatchFrame == frame) {
        // FIXME: We need to re-scope this frame instead of cancelling all effort?
        cancelPendingScopingEffort();
        m_activeMatch = 0;
        m_activeSearchString = String();
        m_activeMatchCount = 0;
        // FIXME: We need to notify client here.
        if (frame == m_webPage->mainFrame()) // Don't need to unmark because the page will be destroyed.
            return;
        m_webPage->m_page->unmarkAllTextMatches();
    }
}

void InPageSearchManager::scopeStringMatches(const String& text, bool reset, bool locateActiveMatchOnly, Frame* scopingFrame)
{
    if (reset) {
        if (!locateActiveMatchOnly) {
            m_activeMatchCount = 0;
            m_scopingComplete = false;
        }
        m_resumeScopingFromRange = 0;
        m_locatingActiveMatch = true;
        m_activeMatchIndex = 0;
        // New search should always start from mainFrame.
        scopeStringMatchesSoon(m_webPage->mainFrame(), text, false /* reset */, locateActiveMatchOnly);
        return;
    }

    if (m_resumeScopingFromRange && scopingFrame != m_resumeScopingFromRange->ownerDocument()->frame())
        m_resumeScopingFromRange = 0;

    RefPtr<Range> searchRange(rangeOfContents(scopingFrame->document()));
    Node* originalEndContainer = searchRange->endContainer();
    int originalEndOffset = searchRange->endOffset();
    ExceptionCode ec = 0, ec2 = 0;
    if (m_resumeScopingFromRange) {
        searchRange->setStart(m_resumeScopingFromRange->startContainer(), m_resumeScopingFromRange->startOffset(ec2) + 1, ec);
        if (ec || ec2) {
            m_scopingComplete = true; // We should stop scoping because of some stale data.
            return;
        }
    }

    int matchCount = 0;
    bool timeout = false;
    double startTime = currentTime();
    do {
        RefPtr<Range> resultRange(findPlainText(searchRange.get(), text, m_scopingCaseInsensitive ? CaseInsensitive : 0));
        if (resultRange->collapsed(ec)) {
            if (!resultRange->startContainer()->isInShadowTree())
                break;
            searchRange->setStartAfter(resultRange->startContainer()->deprecatedShadowAncestorNode(), ec);
            searchRange->setEnd(originalEndContainer, originalEndOffset, ec);
            continue;
        }

        ++matchCount;
        bool foundActiveMatch = false;
        if (m_locatingActiveMatch && areRangesEqual(resultRange.get(), m_activeMatch.get())) {
            foundActiveMatch = true;
            m_locatingActiveMatch = false;
            if (locateActiveMatchOnly) {
                m_activeMatchIndex += matchCount;
                m_webPage->m_client->updateFindStringResult(m_activeMatchCount, m_activeMatchIndex);
                return;
            }
            m_activeMatchIndex = m_activeMatchCount + matchCount;
        }
        if (!locateActiveMatchOnly && m_highlightAllMatches)
            resultRange->ownerDocument()->markers()->addTextMatchMarker(resultRange.get(), foundActiveMatch);

        searchRange->setStart(resultRange->endContainer(ec), resultRange->endOffset(ec), ec);
        ShadowRoot* shadowTreeRoot = searchRange->shadowRoot();
        if (searchRange->collapsed(ec) && shadowTreeRoot)
            searchRange->setEnd(shadowTreeRoot, shadowTreeRoot->childNodeCount(), ec);
        m_resumeScopingFromRange = resultRange;
        timeout = (currentTime() - startTime) >= MaxScopingDuration;
    } while (!timeout);

    if (matchCount > 0) {
        if (locateActiveMatchOnly) {
            // We have not found it yet.
            // m_activeMatchIndex now temporarily remember where we left over in this time slot.
            m_activeMatchIndex += matchCount;
        } else {
            if (m_highlightAllMatches)
                scopingFrame->editor().setMarkedTextMatchesAreHighlighted(true /* highlight */);
            m_activeMatchCount += matchCount;
            m_webPage->m_client->updateFindStringResult(m_activeMatchCount, m_activeMatchIndex);
        }
    }

    if (timeout)
        scopeStringMatchesSoon(scopingFrame, text, false /* reset */, locateActiveMatchOnly);
    else {
        // Scoping is done for this frame.
        Frame* nextFrame = DOMSupport::incrementFrame(scopingFrame, true /* forward */, false /* wrapFlag */);
        if (!nextFrame) {
            m_scopingComplete = true;
            return; // Scoping is done for all frames;
        }
        scopeStringMatchesSoon(nextFrame, text, false /* reset */, locateActiveMatchOnly);
    }
}

void InPageSearchManager::scopeStringMatchesSoon(Frame* scopingFrame, const String& text, bool reset, bool locateActiveMatchOnly)
{
    m_deferredScopingWork.append(new DeferredScopeStringMatches(this, scopingFrame, text, reset, locateActiveMatchOnly));
}

void InPageSearchManager::callScopeStringMatches(DeferredScopeStringMatches* caller, Frame* scopingFrame, const String& text, bool reset, bool locateActiveMatchOnly)
{
    m_deferredScopingWork.remove(m_deferredScopingWork.find(caller));
    scopeStringMatches(text, reset, locateActiveMatchOnly, scopingFrame);
    delete caller;
}

void InPageSearchManager::cancelPendingScopingEffort()
{
    deleteAllValues(m_deferredScopingWork);
    m_deferredScopingWork.clear();
}

}
}
