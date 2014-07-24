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
#import "AudioSession.h"

#if USE(AUDIO_SESSION) && PLATFORM(IOS)

#import "SoftLinking.h"
#import <AVFoundation/AVAudioSession.h>
#import <objc/runtime.h>
#import <wtf/RetainPtr.h>

SOFT_LINK_FRAMEWORK(AVFoundation)
SOFT_LINK_CLASS(AVFoundation, AVAudioSession)

SOFT_LINK_POINTER(AVFoundation, AVAudioSessionCategoryAmbient, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVAudioSessionCategorySoloAmbient, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVAudioSessionCategoryPlayback, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVAudioSessionCategoryRecord, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVAudioSessionCategoryPlayAndRecord, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVAudioSessionCategoryAudioProcessing, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVAudioSessionInterruptionNotification, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVAudioSessionInterruptionTypeKey, NSString *)

#define AVAudioSession getAVAudioSessionClass()
#define AVAudioSessionCategoryAmbient getAVAudioSessionCategoryAmbient()
#define AVAudioSessionCategorySoloAmbient getAVAudioSessionCategorySoloAmbient()
#define AVAudioSessionCategoryPlayback getAVAudioSessionCategoryPlayback()
#define AVAudioSessionCategoryRecord getAVAudioSessionCategoryRecord()
#define AVAudioSessionCategoryPlayAndRecord getAVAudioSessionCategoryPlayAndRecord()
#define AVAudioSessionCategoryAudioProcessing getAVAudioSessionCategoryAudioProcessing()
#define AVAudioSessionInterruptionNotification getAVAudioSessionInterruptionNotification()
#define AVAudioSessionInterruptionTypeKey getAVAudioSessionInterruptionTypeKey()

@interface WebAudioSessionHelper : NSObject {
    WebCore::AudioSession* _callback;
}
- (id)initWithCallback:(WebCore::AudioSession*)callback;
- (void)interruption:(NSNotification*)notification;
@end

@implementation WebAudioSessionHelper
- (id)initWithCallback:(WebCore::AudioSession*)callback
{
    self = [super init];
    if (!self)
        return nil;

    _callback = callback;

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(interruption:) name:AVAudioSessionInterruptionNotification object:[AVAudioSession sharedInstance]];

    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

- (void)interruption:(NSNotification *)notification
{
    NSUInteger type = [[[notification userInfo] objectForKey:AVAudioSessionInterruptionTypeKey] unsignedIntegerValue];
    if (type == AVAudioSessionInterruptionTypeBegan)
        _callback->beganAudioInterruption();
    else
        _callback->endedAudioInterruption();
}
@end

namespace WebCore {

class AudioSessionPrivate {
public:
    AudioSessionPrivate(AudioSession*);
    RetainPtr<WebAudioSessionHelper> m_helper;
    AudioSession::CategoryType m_categoryOverride;
};

AudioSessionPrivate::AudioSessionPrivate(AudioSession* session)
    : m_helper(adoptNS([[WebAudioSessionHelper alloc] initWithCallback:session]))
    , m_categoryOverride(AudioSession::None)
{
}

AudioSession::AudioSession()
    : m_private(adoptPtr(new AudioSessionPrivate(this)))
{
}

AudioSession::~AudioSession()
{
}

void AudioSession::setCategory(CategoryType newCategory)
{
    if (categoryOverride() && categoryOverride() != newCategory)
        return;

    if (!categoryOverride()) {
        // If a page plays only WebAudio we set the audio category to "ambient" so it is muted by the ringer switch.
        // However, audio from an HTML5 media element or from a plug-in should always play as "media" so don't change the
        // category once it has already been set to media.
        if (newCategory == AudioSession::AmbientSound && category() == AudioSession::MediaPlayback)
            return;
    }

    NSString* categoryString;
    switch (newCategory) {
    case AmbientSound:
        categoryString = AVAudioSessionCategoryAmbient;
    case SoloAmbientSound:
        categoryString = AVAudioSessionCategorySoloAmbient;
    case MediaPlayback:
        categoryString = AVAudioSessionCategoryPlayback;
    case RecordAudio:
        categoryString = AVAudioSessionCategoryRecord;
    case PlayAndRecord:
        categoryString = AVAudioSessionCategoryPlayAndRecord;
    case AudioProcessing:
        categoryString = AVAudioSessionCategoryAudioProcessing;
    case None:
    default:
        categoryString = nil;
    }
    NSError *error = nil;
    [[AVAudioSession sharedInstance] setCategory:categoryString error:&error];
    ASSERT(!error);
}

AudioSession::CategoryType AudioSession::category() const
{
    NSString* categoryString = [[AVAudioSession sharedInstance] category];
    if ([categoryString isEqual:AVAudioSessionCategoryAmbient])
        return AmbientSound;
    if ([categoryString isEqual:AVAudioSessionCategorySoloAmbient])
        return SoloAmbientSound;
    if ([categoryString isEqual:AVAudioSessionCategoryPlayback])
        return MediaPlayback;
    if ([categoryString isEqual:AVAudioSessionCategoryRecord])
        return RecordAudio;
    if ([categoryString isEqual:AVAudioSessionCategoryPlayAndRecord])
        return PlayAndRecord;
    if ([categoryString isEqual:AVAudioSessionCategoryAudioProcessing])
        return AudioProcessing;
    return None;
}

void AudioSession::setCategoryOverride(CategoryType category)
{
    if (m_private->m_categoryOverride == category)
        return;

    m_private->m_categoryOverride = category;
    setCategory(category);
}

AudioSession::CategoryType AudioSession::categoryOverride() const
{
    return m_private->m_categoryOverride;
}

float AudioSession::sampleRate() const
{
    return [[AVAudioSession sharedInstance] sampleRate];
}

size_t AudioSession::numberOfOutputChannels() const
{
    return [[AVAudioSession sharedInstance] outputNumberOfChannels];
}

void AudioSession::setActive(bool active)
{
    NSError *error = nil;
    [[AVAudioSession sharedInstance] setActive:active error:&error];
    ASSERT(!error);
}

size_t AudioSession::preferredBufferSize() const
{
    return [[AVAudioSession sharedInstance] preferredIOBufferDuration] * sampleRate();
}

void AudioSession::setPreferredBufferSize(size_t bufferSize)
{
    NSError *error = nil;
    float duration = bufferSize / sampleRate();
    [[AVAudioSession sharedInstance] setPreferredIOBufferDuration:duration error:&error];
    ASSERT(!error);
}

}

#endif // USE(AUDIO_SESSION) && PLATFORM(IOS)
