/*
 * Copyright (C) Research In Motion Limited 2013. All rights reserved.
 */

#include "config.h"
#include "SpellingHandler.h"

#include "DOMSupport.h"
#include "Frame.h"
#include "InputHandler.h"
#include "Range.h"
#include "SpellChecker.h"
#include "VisibleUnits.h"

#include <BlackBerryPlatformIMF.h>
#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformStopWatch.h>

#define ENABLE_SPELLING_LOG 0

using namespace BlackBerry::Platform;
using namespace WebCore;

#if ENABLE_SPELLING_LOG
#define SpellingLog(severity, format, ...) Platform::logAlways(severity, format, ## __VA_ARGS__)
#else
#define SpellingLog(severity, format, ...)
#endif // ENABLE_SPELLING_LOG

static const double s_timeout = 0.05;

namespace BlackBerry {
namespace WebKit {

SpellingHandler::SpellingHandler(InputHandler* inputHandler)
    : m_inputHandler(inputHandler)
    , m_iterationDelayTimer(this, &SpellingHandler::parseBlockForSpellChecking)
    , m_isSpellCheckActive(false)
{
}

SpellingHandler::~SpellingHandler()
{
}

void SpellingHandler::spellCheckTextBlock(const WebCore::Element* element, WebCore::TextCheckingProcessType textCheckingProcessType)
{
    SpellingLog(Platform::LogLevelInfo, "SpellingHandler::spellCheckTextBlock received request of type %s",
        textCheckingProcessType == TextCheckingProcessBatch ? "Batch" : "Incremental");

    if (!(element->document() && element->document()->frame() && element->document()->frame()->selection()))
        return;

    VisiblePosition caretPosition = element->document()->frame()->selection()->start();
    // Expand the range to include the previous line. This should handle cases when the user hits enter to finish composing a word and create a new line.
    // Account for word wrapping by jumping to the start of the previous line, then moving to the start of any word which might be there.
    VisibleSelection visibleSelection = VisibleSelection(
        startOfWord(startOfLine(previousLinePosition(caretPosition, caretPosition.lineDirectionPointForBlockDirectionNavigation()))),
        endOfWord(endOfLine(caretPosition)));

    // Check if this request can be sent off in one message, or if it needs to be broken down.
    RefPtr<Range> rangeForSpellChecking = visibleSelection.toNormalizedRange();
    if (!rangeForSpellChecking || !rangeForSpellChecking->text() || !rangeForSpellChecking->text().length())
        return;

    m_textCheckingProcessType = textCheckingProcessType;

    // Spellcheck Batch requests are used when focusing an element. During this time, we might have a lingering request
    // from a previously focused element.
    if (m_textCheckingProcessType == TextCheckingProcessBatch) {
        // If a previous request is being processed, stop it before continueing.
        if (m_iterationDelayTimer.isActive())
            m_iterationDelayTimer.stop();
    }

    m_isSpellCheckActive = true;

    // If we have a batch request, try to send off the entire block.
    if (m_textCheckingProcessType == TextCheckingProcessBatch) {
        // If total block text is under the limited amount, send the entire chunk.
        if (rangeForSpellChecking->text().length() < MaxSpellCheckingStringLength) {
            SpellingLog(Platform::LogLevelInfo, "SpellingHandler::spellCheckTextBlock creating single batch request");
            createSpellCheckRequest(rangeForSpellChecking);
            return;
        }
    }

    // Since we couldn't check the entire block at once, set up starting and ending markers to fire incrementally.
    // Find the start and end of the region we're intending on checking
    m_startPosition = visibleSelection.visibleStart();
    m_endPosition = endOfWord(m_startPosition);
    m_endOfRange = visibleSelection.visibleEnd();
    m_cachedEndPosition = m_endOfRange;

    SpellingLog(Platform::LogLevelInfo, "SpellingHandler::spellCheckTextBlock starting first iteration");
    m_iterationDelayTimer.startOneShot(0);
}

void SpellingHandler::createSpellCheckRequest(const PassRefPtr<WebCore::Range> rangeForSpellCheckingPtr)
{
    RefPtr<WebCore::Range> rangeForSpellChecking = rangeForSpellCheckingPtr;
    rangeForSpellChecking = DOMSupport::trimWhitespaceFromRange(rangeForSpellChecking);
    if (!rangeForSpellChecking)
        return;

    if (rangeForSpellChecking->text().length() >= MinSpellCheckingStringLength) {
        SpellingLog(Platform::LogLevelInfo, "SpellingHandler::createSpellCheckRequest Substring text is '%s', of size %d"
            , rangeForSpellChecking->text().latin1().data()
            , rangeForSpellChecking->text().length());
        m_inputHandler->callRequestCheckingFor(SpellCheckRequest::create(TextCheckingTypeSpelling, m_textCheckingProcessType, rangeForSpellChecking, rangeForSpellChecking));
    }
}

void SpellingHandler::parseBlockForSpellChecking(WebCore::Timer<SpellingHandler>*)
{
#if ENABLE_SPELLING_LOG
    BlackBerry::Platform::StopWatch timer;
    timer.start();
#endif
    SpellingLog(Platform::LogLevelInfo, "SpellingHandler::parseBlockForSpellChecking m_startPosition = %d, m_endPosition = %d, m_cachedEndPosition = %d, m_endOfRange = %d"
        , DOMSupport::offsetFromStartOfBlock(m_startPosition)
        , DOMSupport::offsetFromStartOfBlock(m_endPosition)
        , DOMSupport::offsetFromStartOfBlock(m_cachedEndPosition)
        , DOMSupport::offsetFromStartOfBlock(m_endOfRange));

    if (m_startPosition == m_endOfRange)
        return;

    RefPtr<Range> rangeForSpellChecking = makeRange(m_startPosition, m_endPosition);
    if (!rangeForSpellChecking) {
        SpellingLog(Platform::LogLevelInfo, "SpellingHandler::parseBlockForSpellChecking Failed to set text range for spellchecking.");
        return;
    }

    if (rangeForSpellChecking->text().length() < MaxSpellCheckingStringLength) {
        if (m_endPosition == m_endOfRange || m_cachedEndPosition == m_endPosition) {
            createSpellCheckRequest(rangeForSpellChecking);
            m_isSpellCheckActive = false;
            return;
        }

        incrementSentinels(false /* shouldIncrementStartPosition */);
#if ENABLE_SPELLING_LOG
        SpellingLog(Platform::LogLevelInfo, "SpellingHandler::parseBlockForSpellChecking spellcheck iteration took %lf seconds", timer.elapsed());
#endif
        m_iterationDelayTimer.startOneShot(s_timeout);
        return;
    }

    // Create a spellcheck request with the substring if we have a range that is of size less than MaxSpellCheckingStringLength
    if (rangeForSpellChecking = handleOversizedRange())
        createSpellCheckRequest(rangeForSpellChecking);

    if (isSpellCheckActive()) {
#if ENABLE_SPELLING_LOG
        SpellingLog(Platform::LogLevelInfo, "SpellingHandler::parseBlockForSpellChecking spellcheck iteration took %lf seconds", timer.elapsed());
#endif
        m_iterationDelayTimer.startOneShot(s_timeout);
    }
}

PassRefPtr<Range> SpellingHandler::handleOversizedRange()
{
    SpellingLog(Platform::LogLevelInfo, "SpellingHandler::handleOversizedRange");

    if (m_startPosition == m_cachedEndPosition || m_startPosition == startOfWord(m_endPosition, LeftWordIfOnBoundary)) {
        // Our first word has gone over the character limit. Increment the starting position past an uncheckable word.
        incrementSentinels(true /* shouldIncrementStartPosition */);
        return 0;
    }

    // If this is not the first word, return a Range with end boundary set to the previous word.
    RefPtr<Range> rangeToStartOfOversizedWord = makeRange(m_startPosition, m_cachedEndPosition);
    // We've created the range using the cached end position. Now increment the sentinals forward.
    // FIXME Incrementing the start/end positions outside of incrementSentinels
    m_startPosition = m_cachedEndPosition;
    m_endPosition = endOfWord(m_startPosition);
    return rangeToStartOfOversizedWord;
}

void SpellingHandler::incrementSentinels(bool shouldIncrementStartPosition)
{
    SpellingLog(Platform::LogLevelInfo, "SpellingHandler::incrementSentinels shouldIncrementStartPosition %s", shouldIncrementStartPosition ? "true" : "false");

    if (shouldIncrementStartPosition)
        m_startPosition = m_endPosition;

    VisiblePosition nextWord = nextWordPosition(m_endPosition);
    VisiblePosition startOfNextWord = startOfWord(nextWord, LeftWordIfOnBoundary);
    if (DOMSupport::isRangeTextAllWhitespace(m_endPosition, startOfNextWord)) {
        m_cachedEndPosition = startOfNextWord;
        m_endPosition = endOfWord(startOfNextWord);
        return;
    }

    m_cachedEndPosition = m_endPosition;
    m_endPosition = endOfWord(nextWord);
}

} // WebKit
} // BlackBerry
