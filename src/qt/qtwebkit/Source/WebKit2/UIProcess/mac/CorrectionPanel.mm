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

#import "config.h"
#import "CorrectionPanel.h"

#if USE(AUTOCORRECTION_PANEL)

#import "WebPageProxy.h"
#import "WKView.h"
#import "WKViewInternal.h"

using namespace WebCore;

static inline NSCorrectionIndicatorType correctionIndicatorType(AlternativeTextType alternativeTextType)
{
    switch (alternativeTextType) {
    case AlternativeTextTypeCorrection:
        return NSCorrectionIndicatorTypeDefault;
    case AlternativeTextTypeReversion:
        return NSCorrectionIndicatorTypeReversion;
    case AlternativeTextTypeSpellingSuggestions:
        return NSCorrectionIndicatorTypeGuesses;
    case AlternativeTextTypeDictationAlternatives:
        ASSERT_NOT_REACHED();
        break;
    }
    ASSERT_NOT_REACHED();
    return NSCorrectionIndicatorTypeDefault;
}

namespace WebKit {

CorrectionPanel::CorrectionPanel()
    : m_wasDismissedExternally(false)
    , m_reasonForDismissing(ReasonForDismissingAlternativeTextIgnored)
{
}

CorrectionPanel::~CorrectionPanel()
{
    dismissInternal(ReasonForDismissingAlternativeTextIgnored, false);
}

void CorrectionPanel::show(WKView* view, AlternativeTextType type, const FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings)
{
    dismissInternal(ReasonForDismissingAlternativeTextIgnored, false);
    
    if (!view)
        return;

    NSString* replacedStringAsNSString = replacedString;
    NSString* replacementStringAsNSString = replacementString;
    m_view = view;
    NSCorrectionIndicatorType indicatorType = correctionIndicatorType(type);
    
    NSMutableArray* alternativeStrings = 0;
    if (!alternativeReplacementStrings.isEmpty()) {
        size_t size = alternativeReplacementStrings.size();
        alternativeStrings = [NSMutableArray arrayWithCapacity:size];
        for (size_t i = 0; i < size; ++i)
            [alternativeStrings addObject:(NSString*)alternativeReplacementStrings[i]];
    }

    NSSpellChecker* spellChecker = [NSSpellChecker sharedSpellChecker];
    [spellChecker showCorrectionIndicatorOfType:indicatorType primaryString:replacementStringAsNSString alternativeStrings:alternativeStrings forStringInRect:boundingBoxOfReplacedString view:m_view.get() completionHandler:^(NSString* acceptedString) {
        handleAcceptedReplacement(acceptedString, replacedStringAsNSString, replacementStringAsNSString, indicatorType);
    }];
}

String CorrectionPanel::dismiss(ReasonForDismissingAlternativeText reason)
{
    return dismissInternal(reason, true);
}

String CorrectionPanel::dismissInternal(ReasonForDismissingAlternativeText reason, bool dismissingExternally)
{
    if (!isShowing())
        return String();

    m_wasDismissedExternally = dismissingExternally;
    m_reasonForDismissing = reason;
    m_resultForDismissal.clear();
    [[NSSpellChecker sharedSpellChecker] dismissCorrectionIndicatorForView:m_view.get()];
    return m_resultForDismissal.get();
}

void CorrectionPanel::recordAutocorrectionResponse(WKView* view, NSCorrectionResponse response, const String& replacedString, const String& replacementString)
{
    [[NSSpellChecker sharedSpellChecker] recordResponse:response toCorrection:replacementString forWord:replacedString language:nil inSpellDocumentWithTag:[view spellCheckerDocumentTag]];
}

void CorrectionPanel::handleAcceptedReplacement(NSString* acceptedReplacement, NSString* replaced, NSString* proposedReplacement,  NSCorrectionIndicatorType correctionIndicatorType)
{
    if (!m_view)
        return;

    NSSpellChecker* spellChecker = [NSSpellChecker sharedSpellChecker];
    NSInteger documentTag = [m_view.get() spellCheckerDocumentTag];
    
    switch (correctionIndicatorType) {
    case NSCorrectionIndicatorTypeDefault:
        if (acceptedReplacement)
            [spellChecker recordResponse:NSCorrectionResponseAccepted toCorrection:acceptedReplacement forWord:replaced language:nil inSpellDocumentWithTag:documentTag];
        else {
            if (!m_wasDismissedExternally || m_reasonForDismissing == ReasonForDismissingAlternativeTextCancelled)
                [spellChecker recordResponse:NSCorrectionResponseRejected toCorrection:proposedReplacement forWord:replaced language:nil inSpellDocumentWithTag:documentTag];
            else
                [spellChecker recordResponse:NSCorrectionResponseIgnored toCorrection:proposedReplacement forWord:replaced language:nil inSpellDocumentWithTag:documentTag];
        }
        break;
    case NSCorrectionIndicatorTypeReversion:
        if (acceptedReplacement)
            [spellChecker recordResponse:NSCorrectionResponseReverted toCorrection:replaced forWord:acceptedReplacement language:nil inSpellDocumentWithTag:documentTag];
        break;
    case NSCorrectionIndicatorTypeGuesses:
        if (acceptedReplacement)
            [spellChecker recordResponse:NSCorrectionResponseAccepted toCorrection:acceptedReplacement forWord:replaced language:nil inSpellDocumentWithTag:documentTag];
        break;
    }

    [m_view.get() handleAcceptedAlternativeText:acceptedReplacement];
    m_view.clear();
    if (acceptedReplacement)
        m_resultForDismissal = adoptNS([acceptedReplacement copy]);
}

} // namespace WebKit

#endif // USE(AUTOCORRECTION_PANEL)
