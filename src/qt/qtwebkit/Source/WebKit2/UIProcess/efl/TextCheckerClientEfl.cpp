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

#include "config.h"
#include "TextCheckerClientEfl.h"

#if ENABLE(SPELLCHECK)

#include "EwkView.h"
#include "TextChecker.h"
#include "TextCheckerState.h"
#include "WKAPICast.h"
#include "WKEinaSharedString.h"
#include "WKMutableArray.h"
#include "WKRetainPtr.h"
#include "WKString.h"
#include "WebPageProxy.h"
#include "ewk_text_checker_private.h"
#include <Eina.h>

using namespace WebCore;
using namespace WebKit;

static inline TextCheckerClientEfl* toTextCheckerClientEfl(const void* clientInfo)
{
    return static_cast<TextCheckerClientEfl*>(const_cast<void*>(clientInfo));
}

TextCheckerClientEfl::TextCheckerClientEfl()
    : m_languagesUpdateTimer(this, &TextCheckerClientEfl::languagesUpdateTimerFired)
    , m_spellCheckingSettingChangeTimer(this, &TextCheckerClientEfl::spellCheckingSettingChangeTimerFired)
    , m_textCheckerEnchant(TextCheckerEnchant::create())
{
    memset(&m_clientCallbacks, 0, sizeof(ClientCallbacks));

    WKTextCheckerClient wkTextCheckerClient = {
        kWKTextCheckerClientCurrentVersion,
        this,
        0, // continuousSpellCheckingAllowed
        isContinuousSpellCheckingEnabledCallback,
        setContinuousSpellCheckingEnabledCallback,
        0, // grammarCheckingEnabled
        0, // setGrammarCheckingEnabled
        uniqueSpellDocumentTagCallback,
        closeSpellDocumentWithTagCallback,
        checkSpellingOfStringCallback,
        0, // checkGrammarOfString,
        0, // spellingUIIsShowing
        0, // toggleSpellingUIIsShowing
        0, // updateSpellingUIWithMisspelledWord
        0, // updateSpellingUIWithGrammarString
        guessesForWordCallback,
        learnWordCallback,
        ignoreWordCallback
    };
    WKTextCheckerSetClient(&wkTextCheckerClient);
}

TextCheckerClientEfl& TextCheckerClientEfl::instance()
{
    DEFINE_STATIC_LOCAL(TextCheckerClientEfl, textCheckerClient, ());
    return textCheckerClient;
}

bool TextCheckerClientEfl::isContinuousSpellCheckingEnabled() const
{
    return isContinuousSpellCheckingEnabledCallback(0 /* clientInfo */);
}

void TextCheckerClientEfl::ensureSpellCheckingLanguage()
{
    if (!m_textCheckerEnchant->hasDictionary())
        updateSpellCheckingLanguages();
}

void TextCheckerClientEfl::updateSpellCheckingLanguages(const Vector<String>& defaultLanguages)
{
    m_spellCheckingLanguages = defaultLanguages;
    m_languagesUpdateTimer.startOneShot(0);
}

void TextCheckerClientEfl::languagesUpdateTimerFired(Timer<TextCheckerClientEfl>*)
{
    m_textCheckerEnchant->updateSpellCheckingLanguages(m_spellCheckingLanguages);
}

void TextCheckerClientEfl::spellCheckingSettingChangeTimerFired(Timer<TextCheckerClientEfl>*)
{
    m_clientCallbacks.continuous_spell_checking_change(
        isContinuousSpellCheckingEnabledCallback(0 /* clientInfo */)
    );
}

Vector<String> TextCheckerClientEfl::availableSpellCheckingLanguages() const
{
    return m_textCheckerEnchant->availableSpellCheckingLanguages();
}

Vector<String> TextCheckerClientEfl::loadedSpellCheckingLanguages() const
{
    return m_textCheckerEnchant->loadedSpellCheckingLanguages();
}

void TextCheckerClientEfl::callContinuousSpellCheckingChangeCallbackAsync()
{
    m_spellCheckingSettingChangeTimer.startOneShot(0);
}

bool TextCheckerClientEfl::isContinuousSpellCheckingEnabledCallback(const void*)
{
    return TextChecker::state().isContinuousSpellCheckingEnabled;
}

void TextCheckerClientEfl::setContinuousSpellCheckingEnabledCallback(bool, const void* clientInfo)
{
    TextCheckerClientEfl* textCheckerClient = toTextCheckerClientEfl(clientInfo);
    if (textCheckerClient->clientCallbacks().continuous_spell_checking_change)
        textCheckerClient->callContinuousSpellCheckingChangeCallbackAsync();
}

uint64_t TextCheckerClientEfl::uniqueSpellDocumentTagCallback(WKPageRef page, const void* clientInfo)
{
    TextCheckerClientEfl* textCheckerClient = toTextCheckerClientEfl(clientInfo);
    if (textCheckerClient->clientCallbacks().unique_spell_document_tag_get)
        return textCheckerClient->clientCallbacks().unique_spell_document_tag_get(EwkView::toEvasObject(page));

    return 0;
}

void TextCheckerClientEfl::closeSpellDocumentWithTagCallback(uint64_t tag, const void* clientInfo)
{
    TextCheckerClientEfl* textCheckerClient = toTextCheckerClientEfl(clientInfo);
    if (textCheckerClient->clientCallbacks().unique_spell_document_tag_close)
        textCheckerClient->clientCallbacks().unique_spell_document_tag_close(tag);
}

void TextCheckerClientEfl::checkSpellingOfStringCallback(uint64_t tag, WKStringRef text, int32_t* misspellingLocation, int32_t* misspellingLength, const void* clientInfo)
{
    TextCheckerClientEfl* textCheckerClient = toTextCheckerClientEfl(clientInfo);
    if (textCheckerClient->clientCallbacks().string_spelling_check)
        textCheckerClient->clientCallbacks().string_spelling_check(tag, WKEinaSharedString(text), misspellingLocation, misspellingLength);
    else
        textCheckerClient->m_textCheckerEnchant->checkSpellingOfString(toWTFString(text), *misspellingLocation, *misspellingLength);
}

WKArrayRef TextCheckerClientEfl::guessesForWordCallback(uint64_t tag, WKStringRef word, const void* clientInfo)
{
    WKMutableArrayRef suggestionsForWord = WKMutableArrayCreate();

    TextCheckerClientEfl* textCheckerClient = toTextCheckerClientEfl(clientInfo);
    if (textCheckerClient->clientCallbacks().word_guesses_get) {
        Eina_List* list = textCheckerClient->clientCallbacks().word_guesses_get(tag, WKEinaSharedString(word));
        void* item;

        EINA_LIST_FREE(list, item) {
            WKRetainPtr<WKStringRef> suggestion(AdoptWK, WKStringCreateWithUTF8CString(static_cast<const char*>(item)));
            WKArrayAppendItem(suggestionsForWord, suggestion.get());
            free(item);
        }
    } else {
        const Vector<String>& guesses = textCheckerClient->m_textCheckerEnchant->getGuessesForWord(toWTFString(word));
        size_t numberOfGuesses = guesses.size();
        for (size_t i = 0; i < numberOfGuesses; ++i) {
            WKRetainPtr<WKStringRef> suggestion(AdoptWK, WKStringCreateWithUTF8CString(guesses[i].utf8().data()));
            WKArrayAppendItem(suggestionsForWord, suggestion.get());
        }
    }

    return suggestionsForWord;
}

void TextCheckerClientEfl::learnWordCallback(uint64_t tag, WKStringRef word, const void* clientInfo)
{
    TextCheckerClientEfl* textCheckerClient = toTextCheckerClientEfl(clientInfo);
    if (textCheckerClient->clientCallbacks().word_learn)
        textCheckerClient->clientCallbacks().word_learn(tag, WKEinaSharedString(word));
    else
        textCheckerClient->m_textCheckerEnchant->learnWord(toWTFString(word));
}

void TextCheckerClientEfl::ignoreWordCallback(uint64_t tag, WKStringRef word, const void* clientInfo)
{
    TextCheckerClientEfl* textCheckerClient = toTextCheckerClientEfl(clientInfo);
    if (textCheckerClient->clientCallbacks().word_ignore)
        textCheckerClient->clientCallbacks().word_ignore(tag, WKEinaSharedString(word));
    else
        textCheckerClient->m_textCheckerEnchant->ignoreWord(toWTFString(word));
}

#endif // ENABLE(SPELLCHECK)
