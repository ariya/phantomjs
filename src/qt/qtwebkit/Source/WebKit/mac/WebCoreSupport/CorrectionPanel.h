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

#ifndef CorrectionPanel_h
#define CorrectionPanel_h

#import <AppKit/NSSpellChecker.h>
#import <WebCore/AlternativeTextClient.h>
#import <wtf/RetainPtr.h>

#if USE(AUTOCORRECTION_PANEL)

@class WebView;

class CorrectionPanel {
    WTF_MAKE_NONCOPYABLE(CorrectionPanel);
public:
    CorrectionPanel();
    ~CorrectionPanel();
    void show(WebView*, WebCore::AlternativeTextType, const WebCore::FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings);
    String dismiss(WebCore::ReasonForDismissingAlternativeText);
    static void recordAutocorrectionResponse(WebView*, NSCorrectionResponse, const String& replacedString, const String& replacementString);

private:
    bool isShowing() const { return m_view; }
    String dismissInternal(WebCore::ReasonForDismissingAlternativeText, bool dismissingExternally);
    void handleAcceptedReplacement(NSString* acceptedReplacement, NSString* replaced, NSString* proposedReplacement, NSCorrectionIndicatorType);

    bool m_wasDismissedExternally;
    WebCore::ReasonForDismissingAlternativeText m_reasonForDismissing;
    RetainPtr<WebView> m_view;
    RetainPtr<NSString> m_resultForDismissal;
};

#endif // USE(AUTOCORRECTION_PANEL)

#endif // CorrectionPanel_h
