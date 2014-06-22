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


#ifndef WebAlternativeTextClient_h
#define WebAlternativeTextClient_h

#include <WebCore/AlternativeTextClient.h>

namespace WebKit {

class WebPage;

class WebAlternativeTextClient : public WebCore::AlternativeTextClient {
public:
    WebAlternativeTextClient(WebPage *);
    virtual ~WebAlternativeTextClient();
    virtual void pageDestroyed() OVERRIDE;
#if USE(AUTOCORRECTION_PANEL)
    virtual void showCorrectionAlternative(WebCore::AlternativeTextType, const WebCore::FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings) OVERRIDE;
    virtual void dismissAlternative(WebCore::ReasonForDismissingAlternativeText) OVERRIDE;
    virtual String dismissAlternativeSoon(WebCore::ReasonForDismissingAlternativeText) OVERRIDE;
    virtual void recordAutocorrectionResponse(WebCore::AutocorrectionResponseType, const String& replacedString, const String& replacementString) OVERRIDE;
#endif
#if USE(DICTATION_ALTERNATIVES)
    virtual void showDictationAlternativeUI(const WebCore::FloatRect& boundingBoxOfDictatedText, uint64_t dictationContext) OVERRIDE;
    virtual void removeDictationAlternatives(uint64_t dictationContext) OVERRIDE;
    virtual Vector<String> dictationAlternatives(uint64_t dictationContext) OVERRIDE;
#endif
private:
    WebPage *m_page;
};

}

#endif // WebAlternativeTextClient_h
