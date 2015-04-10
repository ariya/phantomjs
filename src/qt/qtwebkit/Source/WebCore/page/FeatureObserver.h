/*
 * Copyright (C) 2012 Google, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
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

#ifndef FeatureObserver_h
#define FeatureObserver_h

#include <wtf/BitVector.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class DOMWindow;
class Document;

class FeatureObserver {
    WTF_MAKE_NONCOPYABLE(FeatureObserver);
public:
    FeatureObserver();
    ~FeatureObserver();

    enum Feature {
        PageDestruction,
        LegacyNotifications,
        MultipartMainResource,
        PrefixedIndexedDB,
        WorkerStart,
        SharedWorkerStart,
        LegacyWebAudio,
        WebAudioStart,
        PrefixedContentSecurityPolicy,
        UnprefixedIndexedDB,
        OpenWebDatabase,
        UnusedSlot01, // We used this slot for LegacyHTMLNotifications.
        LegacyTextNotifications,
        UnprefixedRequestAnimationFrame,
        PrefixedRequestAnimationFrame,
        ContentSecurityPolicy,
        ContentSecurityPolicyReportOnly,
        PrefixedContentSecurityPolicyReportOnly,
        PrefixedTransitionEndEvent,
        UnprefixedTransitionEndEvent,
        PrefixedAndUnprefixedTransitionEndEvent,
        AutoFocusAttribute,
        AutoSaveAttribute,
        DataListElement,
        FormAttribute,
        IncrementalAttribute,
        InputTypeColor,
        InputTypeDate,
        InputTypeDateTime,
        InputTypeDateTimeFallback,
        InputTypeDateTimeLocal,
        InputTypeEmail,
        InputTypeMonth,
        InputTypeNumber,
        InputTypeRange,
        InputTypeSearch,
        InputTypeTel,
        InputTypeTime,
        InputTypeURL,
        InputTypeWeek,
        InputTypeWeekFallback,
        ListAttribute,
        MaxAttribute,
        MinAttribute,
        PatternAttribute,
        PlaceholderAttribute,
        PrecisionAttribute,
        PrefixedDirectoryAttribute,
        PrefixedSpeechAttribute,
        RequiredAttribute,
        ResultsAttribute,
        StepAttribute,
        PageVisits,
        HTMLMarqueeElement,
        CSSOverflowMarquee,
        Reflection,
        CursorVisibility,
        StorageInfo,
        XFrameOptions,
        XFrameOptionsSameOrigin,
        XFrameOptionsSameOriginWithBadAncestorChain,
        DeprecatedFlexboxWebContent,
        DeprecatedFlexboxChrome,
        DeprecatedFlexboxChromeExtension,
        // Add new features above this line. Don't change assigned numbers of each items.
        NumberOfFeatures, // This enum value must be last.
    };

    static void observe(Document*, Feature);
    static void observe(DOMWindow*, Feature);
    void didCommitLoad();

    const BitVector* accumulatedFeatureBits() const { return m_featureBits.get(); }

private:
    void didObserve(Feature feature)
    {
        ASSERT(feature != PageDestruction); // PageDestruction is reserved as a scaling factor.
        ASSERT(feature < NumberOfFeatures);
        if (!m_featureBits) {
            m_featureBits = adoptPtr(new BitVector(NumberOfFeatures));
            m_featureBits->clearAll();
        }
        m_featureBits->quickSet(feature);
    }

    void updateMeasurements();

    OwnPtr<BitVector> m_featureBits;
};

} // namespace WebCore
    
#endif // FeatureObserver_h
