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

#ifndef WKTextChecker_h
#define WKTextChecker_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

// TextChecker Client
typedef bool (*WKTextCheckerContinousSpellCheckingAllowed)(const void *clientInfo);
typedef bool (*WKTextCheckerContinousSpellCheckingEnabled)(const void *clientInfo);
typedef void (*WKTextCheckerSetContinousSpellCheckingEnabled)(bool enabled, const void *clientInfo);
typedef bool (*WKTextCheckerGrammarCheckingEnabled)(const void *clientInfo);
typedef void (*WKTextCheckerSetGrammarCheckingEnabled)(bool enabled, const void *clientInfo);
typedef uint64_t (*WKTextCheckerUniqueSpellDocumentTag)(WKPageRef page, const void *clientInfo);
typedef void (*WKTextCheckerCloseSpellDocumentWithTag)(uint64_t tag, const void *clientInfo);
typedef void (*WKTextCheckerCheckSpellingOfString)(uint64_t tag, WKStringRef text, int32_t* misspellingLocation, int32_t* misspellingLength, const void *clientInfo);
typedef void (*WKTextCheckerCheckGrammarOfString)(uint64_t tag, WKStringRef text, WKArrayRef* grammarDetails, int32_t* badGrammarLocation, int32_t* badGrammarLength, const void *clientInfo);
typedef bool (*WKTextCheckerSpellingUIIsShowing)(const void *clientInfo);
typedef void (*WKTextCheckerToggleSpellingUIIsShowing)(const void *clientInfo);
typedef void (*WKTextCheckerUpdateSpellingUIWithMisspelledWord)(uint64_t tag, WKStringRef misspelledWord, const void *clientInfo);
typedef void (*WKTextCheckerUpdateSpellingUIWithGrammarString)(uint64_t tag, WKStringRef badGrammarPhrase, WKGrammarDetailRef grammarDetail, const void *clientInfo);
typedef WKArrayRef (*WKTextCheckerGuessesForWord)(uint64_t tag, WKStringRef word, const void *clientInfo);
typedef void (*WKTextCheckerLearnWord)(uint64_t tag, WKStringRef word, const void *clientInfo);
typedef void (*WKTextCheckerIgnoreWord)(uint64_t tag, WKStringRef word, const void *clientInfo);

struct WKTextCheckerClient {
    int                                                                     version;
    const void *                                                            clientInfo;
    WKTextCheckerContinousSpellCheckingAllowed                              continuousSpellCheckingAllowed;
    WKTextCheckerContinousSpellCheckingEnabled                              continuousSpellCheckingEnabled;
    WKTextCheckerSetContinousSpellCheckingEnabled                           setContinuousSpellCheckingEnabled;
    WKTextCheckerGrammarCheckingEnabled                                     grammarCheckingEnabled;
    WKTextCheckerSetGrammarCheckingEnabled                                  setGrammarCheckingEnabled;
    WKTextCheckerUniqueSpellDocumentTag                                     uniqueSpellDocumentTag;
    WKTextCheckerCloseSpellDocumentWithTag                                  closeSpellDocumentWithTag;
    WKTextCheckerCheckSpellingOfString                                      checkSpellingOfString;
    WKTextCheckerCheckGrammarOfString                                       checkGrammarOfString;
    WKTextCheckerSpellingUIIsShowing                                        spellingUIIsShowing;
    WKTextCheckerToggleSpellingUIIsShowing                                  toggleSpellingUIIsShowing;
    WKTextCheckerUpdateSpellingUIWithMisspelledWord                         updateSpellingUIWithMisspelledWord;
    WKTextCheckerUpdateSpellingUIWithGrammarString                          updateSpellingUIWithGrammarString;
    WKTextCheckerGuessesForWord                                             guessesForWord;
    WKTextCheckerLearnWord                                                  learnWord;
    WKTextCheckerIgnoreWord                                                 ignoreWord;
};
typedef struct WKTextCheckerClient WKTextCheckerClient;

enum { kWKTextCheckerClientCurrentVersion = 0 };

WK_EXPORT void WKTextCheckerSetClient(const WKTextCheckerClient* client);

WK_EXPORT void WKTextCheckerContinuousSpellCheckingEnabledStateChanged(bool);
WK_EXPORT void WKTextCheckerGrammarCheckingEnabledStateChanged(bool);

WK_EXPORT void WKTextCheckerCheckSpelling(WKPageRef page, bool startBeforeSelection);
WK_EXPORT void WKTextCheckerChangeSpellingToWord(WKPageRef page, WKStringRef word);

#ifdef __cplusplus
}
#endif

#endif /* WKTextChecker_h */
