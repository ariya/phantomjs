/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include <WebCore/NotImplemented.h>

using namespace WebCore;
 
namespace WebKit {
  
static TextCheckerState textCheckerState;

const TextCheckerState& TextChecker::state()
{
    notImplemented();
    
    return textCheckerState;
}

bool TextChecker::isContinuousSpellCheckingAllowed()
{
    notImplemented();

    return false;
}

void TextChecker::setContinuousSpellCheckingEnabled(bool isContinuousSpellCheckingEnabled)
{
    notImplemented();
}

void TextChecker::setGrammarCheckingEnabled(bool isGrammarCheckingEnabled)
{
    notImplemented();
}

void TextChecker::continuousSpellCheckingEnabledStateChanged(bool enabled)
{
    notImplemented();
}

void TextChecker::grammarCheckingEnabledStateChanged(bool enabled)
{
    notImplemented();
}

int64_t TextChecker::uniqueSpellDocumentTag(WebPageProxy*)
{
    notImplemented();
    return 0;
}

void TextChecker::closeSpellDocumentWithTag(int64_t)
{
    notImplemented();
}

void TextChecker::checkSpellingOfString(int64_t, const UChar*, uint32_t, int32_t&, int32_t&)
{
    notImplemented();
}

void TextChecker::checkGrammarOfString(int64_t, const UChar*, uint32_t, Vector<WebCore::GrammarDetail>&, int32_t&, int32_t&)
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

void TextChecker::getGuessesForWord(int64_t spellDocumentTag, const String& word, const String& context, Vector<String>& guesses)
{
    notImplemented();
}

void TextChecker::learnWord(int64_t, const String&)
{
    notImplemented();
}

void TextChecker::ignoreWord(int64_t spellDocumentTag, const String& word)
{
    notImplemented();
}

void TextChecker::requestCheckingOfString(PassRefPtr<TextCheckerCompletion>)
{
    notImplemented();
}

} // namespace WebKit
