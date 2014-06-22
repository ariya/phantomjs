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

#if ENABLE(VIDEO) && USE(AVFOUNDATION) && !HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT) && !PLATFORM(IOS)

#include "InbandTextTrackPrivateLegacyAVCF.h"

#include "InbandTextTrackPrivateAVF.h"
#include "Logging.h"
#include "MediaPlayerPrivateAVFoundationCF.h"
#include "SoftLinking.h"

#include <AVFoundationCF/AVFoundationCF.h>

#include <d3d9.h>

// The softlink header files must be included after the AVCF and CoreMedia header files.
#include "AVFoundationCFSoftLinking.h"

namespace WebCore {

InbandTextTrackPrivateLegacyAVCF::InbandTextTrackPrivateLegacyAVCF(MediaPlayerPrivateAVFoundationCF* player, AVCFPlayerItemTrackRef track)
    : InbandTextTrackPrivateAVF(player)
    , m_playerItemTrack(track)
{
}

void InbandTextTrackPrivateLegacyAVCF::disconnect()
{
    m_playerItemTrack = 0;
    InbandTextTrackPrivateAVF::disconnect();
}

InbandTextTrackPrivate::Kind InbandTextTrackPrivateLegacyAVCF::kind() const
{
    if (!m_playerItemTrack)
        return InbandTextTrackPrivate::None;

    return InbandTextTrackPrivate::Captions;
}

bool InbandTextTrackPrivateLegacyAVCF::isClosedCaptions() const
{
    return m_playerItemTrack;
}

bool InbandTextTrackPrivateLegacyAVCF::containsOnlyForcedSubtitles() const
{
    return false;
}

bool InbandTextTrackPrivateLegacyAVCF::isMainProgramContent() const
{
    return m_playerItemTrack;
}

bool InbandTextTrackPrivateLegacyAVCF::isEasyToRead() const
{
    return false;
}

AtomicString InbandTextTrackPrivateLegacyAVCF::label() const
{
    if (!m_playerItemTrack)
        return emptyAtom;

    RetainPtr<CFStringRef> title;

    RetainPtr<AVCFAssetTrackRef> assetTrack = adoptCF(AVCFPlayerItemTrackCopyAssetTrack(m_playerItemTrack.get()));
    RetainPtr<CFArrayRef> commonMetaData = adoptCF(AVCFAssetTrackCopyCommonMetadata(assetTrack.get()));
    RetainPtr<CFArrayRef> titles = adoptCF(AVCFMetadataItemCopyItemsWithKeyAndKeySpace(commonMetaData.get(), AVCFMetadataCommonKeyTitle, AVCFMetadataKeySpaceCommon));
    CFIndex titlesCount = CFArrayGetCount(titles.get());
    if (titlesCount) {
        RetainPtr<CFLocaleRef> currentLocale = adoptCF(CFLocaleCopyCurrent());
        RetainPtr<CFArrayRef> titlesForPreferredLanguages = adoptCF(AVCFMetadataItemCopyItemsWithLocale(titles.get(), currentLocale.get()));
        CFIndex preferredTitlesCount = CFArrayGetCount(titlesForPreferredLanguages.get());
        if (preferredTitlesCount) {
            AVCFMetadataItemRef titleMetadata = static_cast<AVCFMetadataItemRef>(CFArrayGetValueAtIndex(titlesForPreferredLanguages.get(), 0));
            title = adoptCF(AVCFMetadataItemCopyStringValue(titleMetadata));
        }

        if (!title) {
            AVCFMetadataItemRef titleMetadata = static_cast<AVCFMetadataItemRef>(CFArrayGetValueAtIndex(titles.get(), 0));
            title = adoptCF(AVCFMetadataItemCopyStringValue(titleMetadata));
        }
    }

    return title ? AtomicString(title.get()) : emptyAtom;
}

AtomicString InbandTextTrackPrivateLegacyAVCF::language() const
{
    if (!m_playerItemTrack)
        return emptyAtom;

    RetainPtr<AVCFAssetTrackRef> assetTrack = adoptCF(AVCFPlayerItemTrackCopyAssetTrack(m_playerItemTrack.get()));
    RetainPtr<CFStringRef> languageCode = adoptCF(AVCFAssetTrackCopyLanguageCode(assetTrack.get()));
    RetainPtr<CFLocaleRef> locale = adoptCF(CFLocaleCreate(kCFAllocatorDefault, languageCode.get()));
    return CFLocaleGetIdentifier(locale.get());
}

} // namespace WebCore

#endif
