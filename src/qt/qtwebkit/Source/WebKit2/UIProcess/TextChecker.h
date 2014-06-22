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

#ifndef TextChecker_h
#define TextChecker_h

#include "TextCheckerCompletion.h"
#include <WebCore/EditorClient.h>
#include <WebCore/TextCheckerClient.h>

namespace WebKit {

class WebPageProxy;
struct TextCheckerState;

class TextChecker {
public:
    static const TextCheckerState& state();
    static bool isContinuousSpellCheckingAllowed();

    static void setContinuousSpellCheckingEnabled(bool);
    static void setGrammarCheckingEnabled(bool);

#if PLATFORM(MAC)
    static void setAutomaticSpellingCorrectionEnabled(bool);
    static void setAutomaticQuoteSubstitutionEnabled(bool);
    static void setAutomaticDashSubstitutionEnabled(bool);
    static void setAutomaticLinkDetectionEnabled(bool);
    static void setAutomaticTextReplacementEnabled(bool);

    static void didChangeAutomaticTextReplacementEnabled();
    static void didChangeAutomaticSpellingCorrectionEnabled();
    static void didChangeAutomaticQuoteSubstitutionEnabled();
    static void didChangeAutomaticDashSubstitutionEnabled();

    static bool isSmartInsertDeleteEnabled();
    static void setSmartInsertDeleteEnabled(bool);

    static bool substitutionsPanelIsShowing();
    static void toggleSubstitutionsPanelIsShowing();
#endif

    static void continuousSpellCheckingEnabledStateChanged(bool);
    static void grammarCheckingEnabledStateChanged(bool);
    static int64_t uniqueSpellDocumentTag(WebPageProxy*);
    static void closeSpellDocumentWithTag(int64_t);
#if USE(UNIFIED_TEXT_CHECKING)
    static Vector<WebCore::TextCheckingResult> checkTextOfParagraph(int64_t spellDocumentTag, const UChar* text, int length, uint64_t checkingTypes);
#endif
    static void checkSpellingOfString(int64_t spellDocumentTag, const UChar* text, uint32_t length, int32_t& misspellingLocation, int32_t& misspellingLength);
    static void checkGrammarOfString(int64_t spellDocumentTag, const UChar* text, uint32_t length, Vector<WebCore::GrammarDetail>&, int32_t& badGrammarLocation, int32_t& badGrammarLength);
    static bool spellingUIIsShowing();
    static void toggleSpellingUIIsShowing();
    static void updateSpellingUIWithMisspelledWord(int64_t spellDocumentTag, const String& misspelledWord);
    static void updateSpellingUIWithGrammarString(int64_t spellDocumentTag, const String& badGrammarPhrase, const WebCore::GrammarDetail&);
    static void getGuessesForWord(int64_t spellDocumentTag, const String& word, const String& context, Vector<String>& guesses);
    static void learnWord(int64_t spellDocumentTag, const String& word);
    static void ignoreWord(int64_t spellDocumentTag, const String& word);
    static void requestCheckingOfString(PassRefPtr<TextCheckerCompletion>);
};

} // namespace WebKit

#endif // TextChecker_h
