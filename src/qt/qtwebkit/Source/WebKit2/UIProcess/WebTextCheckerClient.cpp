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

#include "config.h"
#include "WebTextCheckerClient.h"

#include "ImmutableArray.h"
#include "WKAPICast.h"
#include "WKSharedAPICast.h"
#include "WebGrammarDetail.h"
#include "WebPageProxy.h"
#include <wtf/text/WTFString.h>

namespace WebKit {

bool WebTextCheckerClient::continuousSpellCheckingAllowed()
{
    if (!m_client.continuousSpellCheckingAllowed)
        return false;

    return m_client.continuousSpellCheckingAllowed(m_client.clientInfo);
}

bool WebTextCheckerClient::continuousSpellCheckingEnabled()
{
    if (!m_client.continuousSpellCheckingEnabled)
        return false;

    return m_client.continuousSpellCheckingEnabled(m_client.clientInfo);
}

void WebTextCheckerClient::setContinuousSpellCheckingEnabled(bool enabled)
{
    if (!m_client.setContinuousSpellCheckingEnabled)
        return;

    m_client.setContinuousSpellCheckingEnabled(enabled, m_client.clientInfo);
}

bool WebTextCheckerClient::grammarCheckingEnabled()
{
    if (!m_client.grammarCheckingEnabled)
        return false;

    return m_client.grammarCheckingEnabled(m_client.clientInfo);
}

void WebTextCheckerClient::setGrammarCheckingEnabled(bool enabled)
{
    if (!m_client.setGrammarCheckingEnabled)
        return;

    m_client.setGrammarCheckingEnabled(enabled, m_client.clientInfo);
}

uint64_t WebTextCheckerClient::uniqueSpellDocumentTag(WebPageProxy* page)
{
    if (!m_client.uniqueSpellDocumentTag)
        return 0;

    return m_client.uniqueSpellDocumentTag(toAPI(page), m_client.clientInfo);
}

void WebTextCheckerClient::closeSpellDocumentWithTag(uint64_t tag)
{
    if (!m_client.closeSpellDocumentWithTag)
        return;

    m_client.closeSpellDocumentWithTag(tag, m_client.clientInfo);
}

void WebTextCheckerClient::checkSpellingOfString(uint64_t tag, const String& text, int32_t& misspellingLocation, int32_t& misspellingLength)
{
    misspellingLocation = -1;
    misspellingLength = 0;

    if (!m_client.checkSpellingOfString)
        return;

    m_client.checkSpellingOfString(tag, toAPI(text.impl()), &misspellingLocation, &misspellingLength, m_client.clientInfo);
}

void WebTextCheckerClient::checkGrammarOfString(uint64_t tag, const String& text, Vector<WebCore::GrammarDetail>& grammarDetails, int32_t& badGrammarLocation, int32_t& badGrammarLength)
{
    badGrammarLocation = -1;
    badGrammarLength = 0;

    if (!m_client.checkGrammarOfString)
        return;

    WKArrayRef wkGrammarDetailsRef = 0;
    m_client.checkGrammarOfString(tag, toAPI(text.impl()), &wkGrammarDetailsRef, &badGrammarLocation, &badGrammarLength, m_client.clientInfo);

    RefPtr<ImmutableArray> wkGrammarDetails = adoptRef(toImpl(wkGrammarDetailsRef));
    size_t numGrammarDetails = wkGrammarDetails->size();
    for (size_t i = 0; i < numGrammarDetails; ++i)
        grammarDetails.append(wkGrammarDetails->at<WebGrammarDetail>(i)->grammarDetail());
}

bool WebTextCheckerClient::spellingUIIsShowing()
{
    if (!m_client.spellingUIIsShowing)
        return false;

    return m_client.spellingUIIsShowing(m_client.clientInfo);
}

void WebTextCheckerClient::toggleSpellingUIIsShowing()
{
    if (!m_client.toggleSpellingUIIsShowing)
        return;

    return m_client.toggleSpellingUIIsShowing(m_client.clientInfo);
}

void WebTextCheckerClient::updateSpellingUIWithMisspelledWord(uint64_t tag, const String& misspelledWord)
{
    if (!m_client.updateSpellingUIWithMisspelledWord)
        return;

    m_client.updateSpellingUIWithMisspelledWord(tag, toAPI(misspelledWord.impl()), m_client.clientInfo);
}

void WebTextCheckerClient::updateSpellingUIWithGrammarString(uint64_t tag, const String& badGrammarPhrase, const WebCore::GrammarDetail& grammarDetail)
{
    if (!m_client.updateSpellingUIWithGrammarString)
        return;

    m_client.updateSpellingUIWithGrammarString(tag, toAPI(badGrammarPhrase.impl()), toAPI(grammarDetail), m_client.clientInfo);
}

void WebTextCheckerClient::guessesForWord(uint64_t tag, const String& word, Vector<String>& guesses)
{
    if (!m_client.guessesForWord)
        return;

    RefPtr<ImmutableArray> wkGuesses = adoptRef(toImpl(m_client.guessesForWord(tag, toAPI(word.impl()), m_client.clientInfo)));
    size_t numGuesses = wkGuesses->size();
    for (size_t i = 0; i < numGuesses; ++i)
        guesses.append(wkGuesses->at<WebString>(i)->string());
}

void WebTextCheckerClient::learnWord(uint64_t tag, const String& word)
{
    if (!m_client.learnWord)
        return;

    m_client.learnWord(tag, toAPI(word.impl()), m_client.clientInfo);
}

void WebTextCheckerClient::ignoreWord(uint64_t tag, const String& word)
{
    if (!m_client.ignoreWord)
        return;

    m_client.ignoreWord(tag, toAPI(word.impl()), m_client.clientInfo);
}

} // namespace WebKit
