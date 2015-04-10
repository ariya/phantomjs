/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef WebTextCheckerClient_h
#define WebTextCheckerClient_h

#include "APIClient.h"
#include "WKTextChecker.h"
#include <WebCore/TextCheckerClient.h>
#include <wtf/Forward.h>
#include <wtf/Vector.h>

namespace WebKit {

class WebPageProxy;

class WebTextCheckerClient : public APIClient<WKTextCheckerClient, kWKTextCheckerClientCurrentVersion> {
public:
    bool continuousSpellCheckingAllowed();
    bool continuousSpellCheckingEnabled();
    void setContinuousSpellCheckingEnabled(bool);
    bool grammarCheckingEnabled();
    void setGrammarCheckingEnabled(bool);
    uint64_t uniqueSpellDocumentTag(WebPageProxy*);
    void closeSpellDocumentWithTag(uint64_t);
    void checkSpellingOfString(uint64_t tag, const String& text, int32_t& misspellingLocation, int32_t& misspellingLength);
    void checkGrammarOfString(uint64_t tag, const String& text, Vector<WebCore::GrammarDetail>&, int32_t& badGrammarLocation, int32_t& badGrammarLength);
    bool spellingUIIsShowing();
    void toggleSpellingUIIsShowing();
    void updateSpellingUIWithMisspelledWord(uint64_t tag, const String& misspelledWord);
    void updateSpellingUIWithGrammarString(uint64_t tag, const String& badGrammarPhrase, const WebCore::GrammarDetail&);
    void guessesForWord(uint64_t tag, const String& word, Vector<String>& guesses);
    void learnWord(uint64_t tag, const String& word);
    void ignoreWord(uint64_t tag, const String& word);
};

} // namespace WebKit

#endif // WebTextCheckerClient_h
