/*
 * Copyright (C) 2013 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TextCheckerClientEfl_h
#define TextCheckerClientEfl_h

#if ENABLE(SPELLCHECK)

#include "TextCheckerEnchant.h"
#include "Timer.h"
#include <WebKit2/WKTextChecker.h>
#include <WebKit2/ewk_text_checker_private.h>

namespace WebKit {

class TextCheckerClientEfl {
public:
    static TextCheckerClientEfl& instance();

    // Can be set by ewk APIs, by default they are 0.
    ClientCallbacks& clientCallbacks() { return m_clientCallbacks; }

    bool isContinuousSpellCheckingEnabled() const;

    // Languages support.
    void ensureSpellCheckingLanguage();
    Vector<String> availableSpellCheckingLanguages() const;
    Vector<String> loadedSpellCheckingLanguages() const;
    void updateSpellCheckingLanguages(const Vector<String>& defaultLanguages = Vector<String>());

private:
    TextCheckerClientEfl();

    // To set languages on timer.
    void languagesUpdateTimerFired(WebCore::Timer<TextCheckerClientEfl>*);
    WebCore::Timer<TextCheckerClientEfl> m_languagesUpdateTimer;
    Vector<String> m_spellCheckingLanguages;

    // To notify the client about the setting change on timer.
    void spellCheckingSettingChangeTimerFired(WebCore::Timer<TextCheckerClientEfl>*);
    void callContinuousSpellCheckingChangeCallbackAsync();
    WebCore::Timer<TextCheckerClientEfl> m_spellCheckingSettingChangeTimer;

    // WKTextCheckerClient callbacks.
    static bool isContinuousSpellCheckingEnabledCallback(const void*);
    static void setContinuousSpellCheckingEnabledCallback(bool, const void*);
    static uint64_t uniqueSpellDocumentTagCallback(WKPageRef, const void*);
    static void closeSpellDocumentWithTagCallback(uint64_t, const void*);
    static void checkSpellingOfStringCallback(uint64_t, WKStringRef text, int32_t* misspellingLocation, int32_t* misspellingLength, const void*);
    static WKArrayRef guessesForWordCallback(uint64_t, WKStringRef word, const void*);
    static void learnWordCallback(uint64_t, WKStringRef word, const void*);
    static void ignoreWordCallback(uint64_t, WKStringRef word, const void*);

    ClientCallbacks m_clientCallbacks;
    OwnPtr<WebCore::TextCheckerEnchant> m_textCheckerEnchant;
};

} // namespace WebKit

#endif // ENABLE(SPELLCHECK)
#endif // TextCheckerClientEfl_h
