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

#import "config.h"

#if ENABLE(VIDEO) && USE(AVFOUNDATION) && !HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT) && !PLATFORM(IOS)

#import "InbandTextTrackPrivateLegacyAVFObjC.h"

#import "InbandTextTrackPrivateAVF.h"
#import "Logging.h"
#import "MediaPlayerPrivateAVFoundationObjC.h"
#import "SoftLinking.h"
#import <AVFoundation/AVFoundation.h>
#import <objc/runtime.h>

SOFT_LINK_FRAMEWORK_OPTIONAL(AVFoundation)
#define AVPlayerItem getAVPlayerItemClass()

SOFT_LINK_CLASS(AVFoundation, AVPlayerItem)
SOFT_LINK_CLASS(AVFoundation, AVMetadataItem)
#define AVMediaTypeClosedCaption getAVMediaTypeClosedCaption()

SOFT_LINK_POINTER(AVFoundation, AVMediaTypeClosedCaption, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicLegible, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMetadataCommonKeyTitle, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMetadataKeySpaceCommon, NSString *)

#define AVMetadataItem getAVMetadataItemClass()
#define AVMediaCharacteristicLegible getAVMediaCharacteristicLegible()
#define AVMetadataCommonKeyTitle getAVMetadataCommonKeyTitle()
#define AVMetadataKeySpaceCommon getAVMetadataKeySpaceCommon()

using namespace WebCore;
using namespace std;

namespace WebCore {

InbandTextTrackPrivateLegacyAVFObjC::InbandTextTrackPrivateLegacyAVFObjC(MediaPlayerPrivateAVFoundationObjC* player, AVPlayerItemTrack* track)
    : InbandTextTrackPrivateAVF(player)
    , m_playerItemTrack(track)
{
}

void InbandTextTrackPrivateLegacyAVFObjC::disconnect()
{
    m_playerItemTrack = 0;
    InbandTextTrackPrivateAVF::disconnect();
}

InbandTextTrackPrivate::Kind InbandTextTrackPrivateLegacyAVFObjC::kind() const
{
    if (!m_playerItemTrack)
        return InbandTextTrackPrivate::None;

    return InbandTextTrackPrivate::Captions;
}

bool InbandTextTrackPrivateLegacyAVFObjC::isClosedCaptions() const
{
    return m_playerItemTrack;
}

bool InbandTextTrackPrivateLegacyAVFObjC::containsOnlyForcedSubtitles() const
{
    return false;
}

bool InbandTextTrackPrivateLegacyAVFObjC::isMainProgramContent() const
{
    return m_playerItemTrack;
}

bool InbandTextTrackPrivateLegacyAVFObjC::isEasyToRead() const
{
    return false;
}

AtomicString InbandTextTrackPrivateLegacyAVFObjC::label() const
{
    if (!m_playerItemTrack)
        return emptyAtom;

    NSString *title = 0;

    NSArray *titles = [AVMetadataItem metadataItemsFromArray:[[m_playerItemTrack assetTrack] commonMetadata] withKey:AVMetadataCommonKeyTitle keySpace:AVMetadataKeySpaceCommon];
    if ([titles count]) {
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
        // If possible, return a title in one of the user's preferred languages.
        NSArray *titlesForPreferredLanguages = [AVMetadataItem metadataItemsFromArray:titles filteredAndSortedAccordingToPreferredLanguages:[NSLocale preferredLanguages]];
#else
         NSArray *titlesForPreferredLanguages = [AVMetadataItem metadataItemsFromArray:titles withLocale:[NSLocale currentLocale]];
#endif
        if ([titlesForPreferredLanguages count])
            title = [[titlesForPreferredLanguages objectAtIndex:0] stringValue];

        if (!title)
            title = [[titles objectAtIndex:0] stringValue];
    }

    return title ? AtomicString(title) : emptyAtom;
}

AtomicString InbandTextTrackPrivateLegacyAVFObjC::language() const
{
    if (!m_playerItemTrack)
        return emptyAtom;

    NSString *languageCode = [[m_playerItemTrack assetTrack] languageCode];
    RetainPtr<NSLocale> locale = adoptNS([[NSLocale alloc] initWithLocaleIdentifier:languageCode]);
    return [locale localeIdentifier];
}

} // namespace WebCore

#endif
