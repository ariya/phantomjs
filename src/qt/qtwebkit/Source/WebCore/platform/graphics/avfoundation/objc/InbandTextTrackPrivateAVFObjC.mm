/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#if ENABLE(VIDEO) && USE(AVFOUNDATION) && HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)

#import "InbandTextTrackPrivateAVFObjC.h"

#import "BlockExceptions.h"
#import "FloatConversion.h"
#import "InbandTextTrackPrivate.h"
#import "InbandTextTrackPrivateAVF.h"
#import "Logging.h"
#import "SoftLinking.h"
#import <AVFoundation/AVFoundation.h>
#import <objc/runtime.h>

SOFT_LINK_FRAMEWORK_OPTIONAL(AVFoundation)

#define AVPlayer getAVPlayerClass()
#define AVPlayerItem getAVPlayerItemClass()

SOFT_LINK_CLASS(AVFoundation, AVPlayer)
SOFT_LINK_CLASS(AVFoundation, AVPlayerItem)
SOFT_LINK_CLASS(AVFoundation, AVMetadataItem)
SOFT_LINK_CLASS(AVFoundation, AVPlayerItemLegibleOutput)
#define AVMediaCharacteristicVisual getAVMediaCharacteristicVisual()
#define AVMediaCharacteristicAudible getAVMediaCharacteristicAudible()
#define AVMediaTypeClosedCaption getAVMediaTypeClosedCaption()
#define AVMediaCharacteristicContainsOnlyForcedSubtitles getAVMediaCharacteristicContainsOnlyForcedSubtitles()
#define AVMediaCharacteristicIsMainProgramContent getAVMediaCharacteristicIsMainProgramContent()
#define AVMediaCharacteristicEasyToRead getAVMediaCharacteristicEasyToRead()

SOFT_LINK_POINTER(AVFoundation, AVMediaTypeClosedCaption, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicLegible, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMetadataCommonKeyTitle, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMetadataKeySpaceCommon, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaTypeSubtitle, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicTranscribesSpokenDialogForAccessibility, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicDescribesMusicAndSoundForAccessibility, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicContainsOnlyForcedSubtitles, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicIsMainProgramContent, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicEasyToRead, NSString *)

#define AVMetadataItem getAVMetadataItemClass()
#define AVPlayerItemLegibleOutput getAVPlayerItemLegibleOutputClass()
#define AVMediaCharacteristicLegible getAVMediaCharacteristicLegible()
#define AVMetadataCommonKeyTitle getAVMetadataCommonKeyTitle()
#define AVMetadataKeySpaceCommon getAVMetadataKeySpaceCommon()
#define AVMediaTypeSubtitle getAVMediaTypeSubtitle()
#define AVMediaCharacteristicTranscribesSpokenDialogForAccessibility getAVMediaCharacteristicTranscribesSpokenDialogForAccessibility()
#define AVMediaCharacteristicDescribesMusicAndSoundForAccessibility getAVMediaCharacteristicDescribesMusicAndSoundForAccessibility()

using namespace WebCore;
using namespace std;

namespace WebCore {

InbandTextTrackPrivateAVFObjC::InbandTextTrackPrivateAVFObjC(AVFInbandTrackParent* player, AVMediaSelectionOption *selection)
    : InbandTextTrackPrivateAVF(player)
    , m_mediaSelectionOption(selection)
{
}

void InbandTextTrackPrivateAVFObjC::disconnect()
{
    m_mediaSelectionOption = 0;
    InbandTextTrackPrivateAVF::disconnect();
}

InbandTextTrackPrivate::Kind InbandTextTrackPrivateAVFObjC::kind() const
{
    if (!m_mediaSelectionOption)
        return InbandTextTrackPrivate::None;

    NSString *mediaType = [m_mediaSelectionOption mediaType];
    
    if ([mediaType isEqualToString:AVMediaTypeClosedCaption])
        return InbandTextTrackPrivate::Captions;
    if ([mediaType isEqualToString:AVMediaTypeSubtitle]) {

        if ([m_mediaSelectionOption hasMediaCharacteristic:AVMediaCharacteristicContainsOnlyForcedSubtitles])
            return InbandTextTrackPrivate::Forced;

        // An "SDH" track is a subtitle track created for the deaf or hard-of-hearing. "captions" in WebVTT are
        // "labeled as appropriate for the hard-of-hearing", so tag SDH sutitles as "captions".
        if ([m_mediaSelectionOption hasMediaCharacteristic:AVMediaCharacteristicTranscribesSpokenDialogForAccessibility])
            return InbandTextTrackPrivate::Captions;
        if ([m_mediaSelectionOption hasMediaCharacteristic:AVMediaCharacteristicDescribesMusicAndSoundForAccessibility])
            return InbandTextTrackPrivate::Captions;
        
        return InbandTextTrackPrivate::Subtitles;
    }

    return InbandTextTrackPrivate::Captions;
}

bool InbandTextTrackPrivateAVFObjC::isClosedCaptions() const
{
    if (!m_mediaSelectionOption)
        return false;
    
    return [[m_mediaSelectionOption mediaType] isEqualToString:AVMediaTypeClosedCaption];
}

bool InbandTextTrackPrivateAVFObjC::isSDH() const
{
    if (!m_mediaSelectionOption)
        return false;
    
    if (![[m_mediaSelectionOption mediaType] isEqualToString:AVMediaTypeSubtitle])
        return false;

    if ([m_mediaSelectionOption hasMediaCharacteristic:AVMediaCharacteristicTranscribesSpokenDialogForAccessibility] && [m_mediaSelectionOption hasMediaCharacteristic:AVMediaCharacteristicDescribesMusicAndSoundForAccessibility])
        return true;

    return false;
}
    
bool InbandTextTrackPrivateAVFObjC::containsOnlyForcedSubtitles() const
{
    if (!m_mediaSelectionOption)
        return false;
    
    return [m_mediaSelectionOption hasMediaCharacteristic:AVMediaCharacteristicContainsOnlyForcedSubtitles];
}

bool InbandTextTrackPrivateAVFObjC::isMainProgramContent() const
{
    if (!m_mediaSelectionOption)
        return false;
    
    return [m_mediaSelectionOption hasMediaCharacteristic:AVMediaCharacteristicIsMainProgramContent];
}

bool InbandTextTrackPrivateAVFObjC::isEasyToRead() const
{
    if (!m_mediaSelectionOption)
        return false;

    return [m_mediaSelectionOption hasMediaCharacteristic:AVMediaCharacteristicEasyToRead];
}

AtomicString InbandTextTrackPrivateAVFObjC::label() const
{
    if (!m_mediaSelectionOption)
        return emptyAtom;

    NSString *title = 0;

    NSArray *titles = [AVMetadataItem metadataItemsFromArray:[m_mediaSelectionOption.get() commonMetadata] withKey:AVMetadataCommonKeyTitle keySpace:AVMetadataKeySpaceCommon];
    if ([titles count]) {
        // If possible, return a title in one of the user's preferred languages.
        NSArray *titlesForPreferredLanguages = [AVMetadataItem metadataItemsFromArray:titles filteredAndSortedAccordingToPreferredLanguages:[NSLocale preferredLanguages]];
        if ([titlesForPreferredLanguages count])
            title = [[titlesForPreferredLanguages objectAtIndex:0] stringValue];

        if (!title)
            title = [[titles objectAtIndex:0] stringValue];
    }

    return title ? AtomicString(title) : emptyAtom;
}

AtomicString InbandTextTrackPrivateAVFObjC::language() const
{
    if (!m_mediaSelectionOption)
        return emptyAtom;

    return [[m_mediaSelectionOption.get() locale] localeIdentifier];
}

bool InbandTextTrackPrivateAVFObjC::isDefault() const
{
    return false;
}

} // namespace WebCore

#endif
