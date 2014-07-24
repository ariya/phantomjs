/*
 * Copyright (C) 2012 Igalia S.L.
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
#include "WebKitTextChecker.h"

#if ENABLE(SPELLCHECK)

#include "WebKitPrivate.h"

using namespace WebKit;

static inline WebKitTextChecker* toTextChecker(const void* clientInfo)
{
    return static_cast<WebKitTextChecker*>(const_cast<void*>(clientInfo));
}

static bool continuousSpellCheckingEnabledCallback(const void* clientInfo)
{
    return toTextChecker(clientInfo)->isSpellCheckingEnabled();
}

static void setContinuousSpellCheckingEnabledCallback(bool enabled, const void* clientInfo)
{
    toTextChecker(clientInfo)->setSpellCheckingEnabled(enabled);
}

static void checkSpellingOfStringCallback(uint64_t tag, WKStringRef text, int32_t* misspellingLocation, int32_t* misspellingLength, const void* clientInfo)
{
    toTextChecker(clientInfo)->checkSpellingOfString(toImpl(text)->string(), *misspellingLocation, *misspellingLength);
}

static WKArrayRef guessesForWordCallback(uint64_t tag, WKStringRef word, const void* clientInfo)
{
    Vector<String> guesses = toTextChecker(clientInfo)->getGuessesForWord(toImpl(word)->string());
    if (guesses.isEmpty())
        return 0;

    WKMutableArrayRef wkSuggestions = WKMutableArrayCreate();
    for (Vector<String>::const_iterator iter = guesses.begin(); iter != guesses.end(); ++iter) {
        WKRetainPtr<WKStringRef> wkSuggestion(AdoptWK, WKStringCreateWithUTF8CString(iter->utf8().data()));
        WKArrayAppendItem(wkSuggestions, wkSuggestion.get());
    }

    return wkSuggestions;
}

static void learnWordCallback(uint64_t tag, WKStringRef word, const void* clientInfo)
{
    toTextChecker(clientInfo)->learnWord(toImpl(word)->string());
}

static void ignoreWordCallback(uint64_t tag, WKStringRef word, const void* clientInfo)
{
    toTextChecker(clientInfo)->ignoreWord(toImpl(word)->string());
}

WebKitTextChecker::~WebKitTextChecker()
{
}

WebKitTextChecker::WebKitTextChecker()
    : m_textChecker(WebCore::TextCheckerEnchant::create())
    , m_spellCheckingEnabled(false)
{
    WKTextCheckerClient wkTextCheckerClient = {
        kWKTextCheckerClientCurrentVersion,
        this, // clientInfo
        0, // continuousSpellCheckingAllowed
        continuousSpellCheckingEnabledCallback,
        setContinuousSpellCheckingEnabledCallback,
        0, // grammarCheckingEnabled
        0, // setGrammarCheckingEnabled
        0, // uniqueSpellDocumentTag
        0, // closeSpellDocumentWithTag
        checkSpellingOfStringCallback,
        0, // checkGrammarOfString
        0, // spellingUIIsShowing
        0, // toggleSpellingUIIsShowing
        0, // updateSpellingUIWithMisspelledWord
        0, // updateSpellingUIWithGrammarString
        guessesForWordCallback,
        learnWordCallback,
        ignoreWordCallback,
    };
    WKTextCheckerSetClient(&wkTextCheckerClient);
}

void WebKitTextChecker::checkSpellingOfString(const String& string, int& misspellingLocation, int& misspellingLength)
{
    m_textChecker->checkSpellingOfString(string, misspellingLocation, misspellingLength);
}

Vector<String> WebKitTextChecker::getGuessesForWord(const String& word)
{
    return m_textChecker->getGuessesForWord(word);
}

void WebKitTextChecker::learnWord(const String& word)
{
    m_textChecker->learnWord(word);
}

void WebKitTextChecker::ignoreWord(const String& word)
{
    m_textChecker->ignoreWord(word);
}

void WebKitTextChecker::setSpellCheckingEnabled(bool enabled)
{
    if (m_spellCheckingEnabled == enabled)
        return;
    m_spellCheckingEnabled = enabled;

    // We need to notify the Web process that this has changed.
    WKTextCheckerContinuousSpellCheckingEnabledStateChanged(enabled);
}

const char* const* WebKitTextChecker::getSpellCheckingLanguages()
{
    Vector<String> spellCheckingLanguages = m_textChecker->loadedSpellCheckingLanguages();
    if (spellCheckingLanguages.isEmpty())
        return 0;

    m_spellCheckingLanguages = adoptGRef(g_ptr_array_new_with_free_func(g_free));
    for (size_t i = 0; i < spellCheckingLanguages.size(); ++i)
        g_ptr_array_add(m_spellCheckingLanguages.get(), g_strdup(spellCheckingLanguages[i].utf8().data()));
    g_ptr_array_add(m_spellCheckingLanguages.get(), 0);

    return reinterpret_cast<char**>(m_spellCheckingLanguages->pdata);
}

void WebKitTextChecker::setSpellCheckingLanguages(const char* const* languages)
{
    Vector<String> spellCheckingLanguages;
    for (size_t i = 0; languages[i]; ++i)
        spellCheckingLanguages.append(String::fromUTF8(languages[i]));
    m_textChecker->updateSpellCheckingLanguages(spellCheckingLanguages);
}
#endif // ENABLE(SPELLCHECK)
