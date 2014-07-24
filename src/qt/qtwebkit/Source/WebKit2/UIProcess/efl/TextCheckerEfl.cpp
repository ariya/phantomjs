/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 * Copyright (C) 2011-2013 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TextChecker.h"

#include "NotImplemented.h"
#include "TextCheckerState.h"

#if ENABLE(SPELLCHECK)
#include "TextBreakIterator.h"
#include "TextCheckerClientEfl.h"
#include "WebTextChecker.h"
#endif

using namespace WebCore;

namespace WebKit {

static TextCheckerState textCheckerState;

const TextCheckerState& TextChecker::state()
{
    static bool didInitializeState = false;
    if (didInitializeState)
        return textCheckerState;

    textCheckerState.isContinuousSpellCheckingEnabled = false;
    textCheckerState.isGrammarCheckingEnabled = false;

    didInitializeState = true;

    return textCheckerState;
}

bool TextChecker::isContinuousSpellCheckingAllowed()
{
    notImplemented();
    return false;
}

void TextChecker::setContinuousSpellCheckingEnabled(bool isContinuousSpellCheckingEnabled)
{
#if ENABLE(SPELLCHECK)
    if (state().isContinuousSpellCheckingEnabled == isContinuousSpellCheckingEnabled)
        return;

    textCheckerState.isContinuousSpellCheckingEnabled = isContinuousSpellCheckingEnabled;

    // Notify the client about the setting change.
    WebTextChecker::shared()->client().setContinuousSpellCheckingEnabled(isContinuousSpellCheckingEnabled);
#else
    UNUSED_PARAM(isContinuousSpellCheckingEnabled);
#endif
}

void TextChecker::setGrammarCheckingEnabled(bool)
{
    notImplemented();
}

void TextChecker::continuousSpellCheckingEnabledStateChanged(bool enabled)
{
#if ENABLE(SPELLCHECK)
    if (state().isContinuousSpellCheckingEnabled == enabled)
        return;

    textCheckerState.isContinuousSpellCheckingEnabled = enabled;
#else
    UNUSED_PARAM(enabled);
#endif
}

void TextChecker::grammarCheckingEnabledStateChanged(bool)
{
    notImplemented();
}

int64_t TextChecker::uniqueSpellDocumentTag(WebPageProxy* page)
{
#if ENABLE(SPELLCHECK)
    return WebTextChecker::shared()->client().uniqueSpellDocumentTag(page);
#else
    UNUSED_PARAM(page);
    return 0;
#endif
}

void TextChecker::closeSpellDocumentWithTag(int64_t tag)
{
#if ENABLE(SPELLCHECK)
    WebTextChecker::shared()->client().closeSpellDocumentWithTag(tag);
#else
    UNUSED_PARAM(tag);
#endif
}

#if ENABLE(SPELLCHECK)
static int nextWordOffset(const UChar* text, int length, int currentOffset)
{
    // FIXME: avoid creating textIterator object here, it could be passed as a parameter.
    //        isTextBreak() leaves the iterator pointing to the first boundary position at
    //        or after "offset" (ubrk_isBoundary side effect).
    //        For many word separators, the method doesn't properly determine the boundaries
    //        without resetting the iterator.
    TextBreakIterator* textIterator = wordBreakIterator(text, length);
    if (!textIterator)
        return currentOffset;

    int wordOffset = currentOffset;
    while (wordOffset < length && isTextBreak(textIterator, wordOffset))
        ++wordOffset;

    // Do not treat the word's boundary as a separator.
    if (!currentOffset && wordOffset == 1)
        return currentOffset;

    // Omit multiple separators.
    if ((wordOffset - currentOffset) > 1)
        --wordOffset;

    return wordOffset;
}
#endif // ENABLE(SPELLCHECK)

#if USE(UNIFIED_TEXT_CHECKING)
Vector<TextCheckingResult> TextChecker::checkTextOfParagraph(int64_t spellDocumentTag, const UChar* text, int length, uint64_t checkingTypes)
{
    Vector<TextCheckingResult> paragraphCheckingResult;
#if ENABLE(SPELLCHECK)
    if (checkingTypes & TextCheckingTypeSpelling) {
        TextBreakIterator* textIterator = wordBreakIterator(text, length);
        if (!textIterator)
            return paragraphCheckingResult;

        // Omit the word separators at the beginning/end of the text to don't unnecessarily
        // involve the client to check spelling for them.
        int offset = nextWordOffset(text, length, 0);
        int lengthStrip = length;
        while (lengthStrip > 0 && isTextBreak(textIterator, lengthStrip - 1))
            --lengthStrip;

        while (offset >= 0 && offset < lengthStrip) {
            int32_t misspellingLocation = -1;
            int32_t misspellingLength = 0;
            checkSpellingOfString(spellDocumentTag, text + offset, lengthStrip - offset, misspellingLocation, misspellingLength);
            if (!misspellingLength)
                break;

            TextCheckingResult misspellingResult;
            misspellingResult.type = TextCheckingTypeSpelling;
            misspellingResult.location = offset + misspellingLocation;
            misspellingResult.length = misspellingLength;
            paragraphCheckingResult.append(misspellingResult);
            offset += misspellingLocation + misspellingLength;
            // Generally, we end up checking at the word separator, move to the adjacent word.
            offset = nextWordOffset(text, lengthStrip, offset);
        }
    }
#else
    UNUSED_PARAM(spellDocumentTag);
    UNUSED_PARAM(text);
    UNUSED_PARAM(length);
    UNUSED_PARAM(checkingTypes);
#endif
    return paragraphCheckingResult;
}
#endif

void TextChecker::checkSpellingOfString(int64_t spellDocumentTag, const UChar* text, uint32_t length, int32_t& misspellingLocation, int32_t& misspellingLength)
{
#if ENABLE(SPELLCHECK)
    WebTextChecker::shared()->client().checkSpellingOfString(spellDocumentTag, String(text, length), misspellingLocation, misspellingLength);
#else
    UNUSED_PARAM(spellDocumentTag);
    UNUSED_PARAM(text);
    UNUSED_PARAM(length);
    UNUSED_PARAM(misspellingLocation);
    UNUSED_PARAM(misspellingLength);
#endif
}

void TextChecker::checkGrammarOfString(int64_t, const UChar*, uint32_t, Vector<GrammarDetail>&, int32_t&, int32_t&)
{
    notImplemented();
}

bool TextChecker::spellingUIIsShowing()
{
    notImplemented();
    return false;
}

void TextChecker::toggleSpellingUIIsShowing()
{
    notImplemented();
}

void TextChecker::updateSpellingUIWithMisspelledWord(int64_t, const String&)
{
    notImplemented();
}

void TextChecker::updateSpellingUIWithGrammarString(int64_t, const String&, const GrammarDetail&)
{
    notImplemented();
}

void TextChecker::getGuessesForWord(int64_t spellDocumentTag, const String& word, const String& , Vector<String>& guesses)
{
#if ENABLE(SPELLCHECK)
    WebTextChecker::shared()->client().guessesForWord(spellDocumentTag, word, guesses);
#else
    UNUSED_PARAM(spellDocumentTag);
    UNUSED_PARAM(word);
    UNUSED_PARAM(guesses);
#endif
}

void TextChecker::learnWord(int64_t spellDocumentTag, const String& word)
{
#if ENABLE(SPELLCHECK)
    WebTextChecker::shared()->client().learnWord(spellDocumentTag, word);
#else
    UNUSED_PARAM(spellDocumentTag);
    UNUSED_PARAM(word);
#endif
}

void TextChecker::ignoreWord(int64_t spellDocumentTag, const String& word)
{
#if ENABLE(SPELLCHECK)
    WebTextChecker::shared()->client().ignoreWord(spellDocumentTag, word);
#else
    UNUSED_PARAM(spellDocumentTag);
    UNUSED_PARAM(word);
#endif
}

void TextChecker::requestCheckingOfString(PassRefPtr<TextCheckerCompletion> completion)
{
#if ENABLE(SPELLCHECK)
    if (!completion)
        return;

    TextCheckingRequestData request = completion->textCheckingRequestData();
    ASSERT(request.sequence() != unrequestedTextCheckingSequence);
    ASSERT(request.mask() != TextCheckingTypeNone);

    String text = request.text();
    Vector<TextCheckingResult> result = checkTextOfParagraph(completion->spellDocumentTag(), text.characters(), text.length(), request.mask());

    completion->didFinishCheckingText(result);
#else
    UNUSED_PARAM(completion);
#endif
}

} // namespace WebKit
