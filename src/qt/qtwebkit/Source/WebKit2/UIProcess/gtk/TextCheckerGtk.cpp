/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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

#include "TextCheckerState.h"
#include "WebTextChecker.h"
#include <WebCore/NotImplemented.h>

using namespace WebCore;
 
namespace WebKit {

static TextCheckerState textCheckerState;

const TextCheckerState& TextChecker::state()
{
    static bool didInitializeState = false;
    if (didInitializeState)
        return textCheckerState;

    WebTextCheckerClient& client = WebTextChecker::shared()->client();
    textCheckerState.isContinuousSpellCheckingEnabled = client.continuousSpellCheckingEnabled();
    textCheckerState.isGrammarCheckingEnabled =  client.grammarCheckingEnabled();

    didInitializeState = true;

    return textCheckerState;
}
  
bool TextChecker::isContinuousSpellCheckingAllowed()
{
    return WebTextChecker::shared()->client().continuousSpellCheckingAllowed();
}

void TextChecker::setContinuousSpellCheckingEnabled(bool isContinuousSpellCheckingEnabled)
{
    if (state().isContinuousSpellCheckingEnabled == isContinuousSpellCheckingEnabled)
        return;
    textCheckerState.isContinuousSpellCheckingEnabled = isContinuousSpellCheckingEnabled;
    WebTextChecker::shared()->client().setContinuousSpellCheckingEnabled(isContinuousSpellCheckingEnabled);
}

void TextChecker::setGrammarCheckingEnabled(bool isGrammarCheckingEnabled)
{
    if (state().isGrammarCheckingEnabled == isGrammarCheckingEnabled)
        return;
    textCheckerState.isGrammarCheckingEnabled = isGrammarCheckingEnabled;
    WebTextChecker::shared()->client().setGrammarCheckingEnabled(isGrammarCheckingEnabled);
}

void TextChecker::continuousSpellCheckingEnabledStateChanged(bool enabled)
{
    textCheckerState.isContinuousSpellCheckingEnabled = enabled;
}

void TextChecker::grammarCheckingEnabledStateChanged(bool enabled)
{
    textCheckerState.isGrammarCheckingEnabled = enabled;
}

int64_t TextChecker::uniqueSpellDocumentTag(WebPageProxy* page)
{
    return WebTextChecker::shared()->client().uniqueSpellDocumentTag(page);
}

void TextChecker::closeSpellDocumentWithTag(int64_t tag)
{
    WebTextChecker::shared()->client().closeSpellDocumentWithTag(tag);
}

void TextChecker::checkSpellingOfString(int64_t spellDocumentTag, const UChar* text, uint32_t length, int32_t& misspellingLocation, int32_t& misspellingLength)
{
    WebTextChecker::shared()->client().checkSpellingOfString(spellDocumentTag, String(text, length), misspellingLocation, misspellingLength);
}

void TextChecker::checkGrammarOfString(int64_t spellDocumentTag, const UChar* text, uint32_t length, Vector<WebCore::GrammarDetail>& grammarDetails, int32_t& badGrammarLocation, int32_t& badGrammarLength)
{
    WebTextChecker::shared()->client().checkGrammarOfString(spellDocumentTag, String(text, length), grammarDetails, badGrammarLocation, badGrammarLength);
}

bool TextChecker::spellingUIIsShowing()
{
    return WebTextChecker::shared()->client().spellingUIIsShowing();
}

void TextChecker::toggleSpellingUIIsShowing()
{
    WebTextChecker::shared()->client().toggleSpellingUIIsShowing();
}

void TextChecker::updateSpellingUIWithMisspelledWord(int64_t spellDocumentTag, const String& misspelledWord)
{
    WebTextChecker::shared()->client().updateSpellingUIWithMisspelledWord(spellDocumentTag, misspelledWord);
}

void TextChecker::updateSpellingUIWithGrammarString(int64_t spellDocumentTag, const String& badGrammarPhrase, const GrammarDetail& grammarDetail)
{
    WebTextChecker::shared()->client().updateSpellingUIWithGrammarString(spellDocumentTag, badGrammarPhrase, grammarDetail);
}

void TextChecker::getGuessesForWord(int64_t spellDocumentTag, const String& word, const String& context, Vector<String>& guesses)
{
    WebTextChecker::shared()->client().guessesForWord(spellDocumentTag, word, guesses);
}

void TextChecker::learnWord(int64_t spellDocumentTag, const String& word)
{
    WebTextChecker::shared()->client().learnWord(spellDocumentTag, word);
}

void TextChecker::ignoreWord(int64_t spellDocumentTag, const String& word)
{
    WebTextChecker::shared()->client().ignoreWord(spellDocumentTag, word);
}

void TextChecker::requestCheckingOfString(PassRefPtr<TextCheckerCompletion>)
{
    notImplemented();
}

} // namespace WebKit
