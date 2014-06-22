/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#include "config.h"

#if PLATFORM(WIN) && ENABLE(VIDEO) && HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)

#include "InbandTextTrackPrivateAVCF.h"

#include "FloatConversion.h"
#include "InbandTextTrackPrivate.h"
#include "InbandTextTrackPrivateAVF.h"
#include "Logging.h"
#include "SoftLinking.h"

#include <AVFoundationCF/AVCFPlayerItemLegibleOutput.h>
#include <AVFoundationCF/AVFoundationCF.h>

#include <d3d9.h>

// The softlink header files must be included after the AVCF and CoreMedia header files.
#include "AVFoundationCFSoftLinking.h"

namespace WebCore {

InbandTextTrackPrivateAVCF::InbandTextTrackPrivateAVCF(AVFInbandTrackParent* player, AVCFMediaSelectionOptionRef selection)
    : InbandTextTrackPrivateAVF(player)
    , m_mediaSelectionOption(selection)
{
}

void InbandTextTrackPrivateAVCF::disconnect()
{
    m_mediaSelectionOption = 0;
    InbandTextTrackPrivateAVF::disconnect();
}

InbandTextTrackPrivate::Kind InbandTextTrackPrivateAVCF::kind() const
{
    if (!m_mediaSelectionOption)
        return InbandTextTrackPrivate::None;

    CFStringRef mediaType = AVCFMediaSelectionOptionGetMediaType(mediaSelectionOption());
    
    if (CFStringCompare(mediaType, AVCFMediaTypeClosedCaption, kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        return InbandTextTrackPrivate::Captions;
    if (CFStringCompare(mediaType, AVCFMediaTypeSubtitle, kCFCompareCaseInsensitive) == kCFCompareEqualTo) {
        if (AVCFMediaSelectionOptionHasMediaCharacteristic(mediaSelectionOption(), AVCFMediaCharacteristicContainsOnlyForcedSubtitles))
            return InbandTextTrackPrivate::Forced;

        // An "SDH" track is a subtitle track created for the deaf or hard-of-hearing. "captions" in WebVTT are
        // "labeled as appropriate for the hard-of-hearing", so tag SDH sutitles as "captions".
        if (AVCFMediaSelectionOptionHasMediaCharacteristic(mediaSelectionOption(), AVCFMediaCharacteristicTranscribesSpokenDialogForAccessibility))
            return InbandTextTrackPrivate::Captions;
        if (AVCFMediaSelectionOptionHasMediaCharacteristic(mediaSelectionOption(), AVCFMediaCharacteristicDescribesMusicAndSoundForAccessibility))
            return InbandTextTrackPrivate::Captions;
        
        return InbandTextTrackPrivate::Subtitles;
    }

    return InbandTextTrackPrivate::Captions;
}

bool InbandTextTrackPrivateAVCF::isClosedCaptions() const
{
    if (!m_mediaSelectionOption)
        return false;

    CFStringRef mediaType = AVCFMediaSelectionOptionGetMediaType(mediaSelectionOption());

    return (CFStringCompare(mediaType, AVCFMediaTypeClosedCaption, kCFCompareCaseInsensitive) == kCFCompareEqualTo);
}

bool InbandTextTrackPrivateAVCF::isSDH() const
{
    if (!m_mediaSelectionOption)
        return false;
    
    CFStringRef mediaType = AVCFMediaSelectionOptionGetMediaType(mediaSelectionOption());

    if (CFStringCompare(mediaType, AVCFMediaTypeSubtitle, kCFCompareCaseInsensitive) != kCFCompareEqualTo)
        return false;

    if (AVCFMediaSelectionOptionHasMediaCharacteristic(mediaSelectionOption(), AVCFMediaCharacteristicTranscribesSpokenDialogForAccessibility)
        && AVCFMediaSelectionOptionHasMediaCharacteristic(mediaSelectionOption(), AVCFMediaCharacteristicDescribesMusicAndSoundForAccessibility))
        return true;

    return false;
}
    
bool InbandTextTrackPrivateAVCF::containsOnlyForcedSubtitles() const
{
    if (!m_mediaSelectionOption)
        return false;
    
    return AVCFMediaSelectionOptionHasMediaCharacteristic(mediaSelectionOption(), AVCFMediaCharacteristicContainsOnlyForcedSubtitles);
}

bool InbandTextTrackPrivateAVCF::isMainProgramContent() const
{
    if (!m_mediaSelectionOption)
        return false;
    
    return AVCFMediaSelectionOptionHasMediaCharacteristic(mediaSelectionOption(), AVCFMediaCharacteristicIsMainProgramContent);
}

bool InbandTextTrackPrivateAVCF::isEasyToRead() const
{
    if (!m_mediaSelectionOption)
        return false;

    return AVCFMediaSelectionOptionHasMediaCharacteristic(mediaSelectionOption(), AVCFMediaCharacteristicEasyToRead);
}

AtomicString InbandTextTrackPrivateAVCF::label() const
{
    if (!m_mediaSelectionOption)
        return emptyAtom;

    RetainPtr<CFStringRef> title;

    RetainPtr<CFArrayRef> commonMetaData = adoptCF(AVCFMediaSelectionOptionCopyCommonMetadata(mediaSelectionOption()));
    RetainPtr<CFArrayRef> titles = adoptCF(AVCFMetadataItemCopyItemsWithKeyAndKeySpace(commonMetaData.get(), AVCFMetadataCommonKeyTitle, AVCFMetadataKeySpaceCommon));
    CFIndex titlesCount = CFArrayGetCount(titles.get());
    if (!titlesCount)
        return emptyAtom;

    // If possible, return a title in one of the user's preferred languages.
    RetainPtr<CFArrayRef> preferredLanguages = CFLocaleCopyPreferredLanguages();
    RetainPtr<CFArrayRef> titlesForPreferredLanguages = AVCFMediaSelectionCopyOptionsFromArrayFilteredAndSortedAccordingToPreferredLanguages(titles.get(), preferredLanguages.get());
    CFIndex preferredTitlesCount = CFArrayGetCount(titlesForPreferredLanguages.get());
    if (preferredTitlesCount) {
        AVCFMetadataItemRef titleMetadata = static_cast<AVCFMetadataItemRef>(CFArrayGetValueAtIndex(titlesForPreferredLanguages.get(), 0));
        title = adoptCF(AVCFMetadataItemCopyStringValue(titleMetadata));
    }

    if (!title) {
        AVCFMetadataItemRef titleMetadata = static_cast<AVCFMetadataItemRef>(CFArrayGetValueAtIndex(titles.get(), 0));
        title = adoptCF(AVCFMetadataItemCopyStringValue(titleMetadata));
    }

    return title ? AtomicString(title.get()) : emptyAtom;
}

AtomicString InbandTextTrackPrivateAVCF::language() const
{
    if (!m_mediaSelectionOption)
        return emptyAtom;

    RetainPtr<CFLocaleRef> locale = adoptCF(AVCFMediaSelectionOptionCopyLocale(mediaSelectionOption()));
    return CFLocaleGetIdentifier(locale.get());
}

bool InbandTextTrackPrivateAVCF::isDefault() const
{
    return false;
}

} // namespace WebCore

#endif
