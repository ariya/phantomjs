/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "WebAlternativeTextClient.h"

#include "WebViewInternal.h"

using namespace WebCore;

WebAlternativeTextClient::WebAlternativeTextClient(WebView* webView)
: m_webView(webView)
{
}

WebAlternativeTextClient::~WebAlternativeTextClient()
{
#if USE(AUTOCORRECTION_PANEL)
    dismissAlternative(ReasonForDismissingAlternativeTextIgnored);
#endif
}

void WebAlternativeTextClient::pageDestroyed()
{
    delete this;
}

#if USE(AUTOCORRECTION_PANEL)
void WebAlternativeTextClient::showCorrectionAlternative(AlternativeTextType type, const FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings)
{
    m_correctionPanel.show(m_webView, type, boundingBoxOfReplacedString, replacedString, replacementString, alternativeReplacementStrings);
}

void WebAlternativeTextClient::dismissAlternative(ReasonForDismissingAlternativeText reason)
{
    m_correctionPanel.dismiss(reason);
}

String WebAlternativeTextClient::dismissAlternativeSoon(ReasonForDismissingAlternativeText reason)
{
    return m_correctionPanel.dismiss(reason);
}

void WebAlternativeTextClient::recordAutocorrectionResponse(AutocorrectionResponseType responseType, const String& replacedString, const String& replacementString)
{
    NSCorrectionResponse response = responseType == AutocorrectionReverted ? NSCorrectionResponseReverted : NSCorrectionResponseEdited;
    CorrectionPanel::recordAutocorrectionResponse(m_webView, response, replacedString, replacementString);
}
#endif

#if USE(DICTATION_ALTERNATIVES)
void WebAlternativeTextClient::removeDictationAlternatives(uint64_t dictationContext)
{
    [m_webView _removeDictationAlternatives:dictationContext];
}

void WebAlternativeTextClient::showDictationAlternativeUI(const WebCore::FloatRect& boundingBoxOfDictatedText, uint64_t dictationContext)
{
    [m_webView _showDictationAlternativeUI:boundingBoxOfDictatedText forDictationContext:dictationContext];
}

Vector<String> WebAlternativeTextClient::dictationAlternatives(uint64_t dictationContext)
{
    return [m_webView _dictationAlternatives:dictationContext];
}
#endif
