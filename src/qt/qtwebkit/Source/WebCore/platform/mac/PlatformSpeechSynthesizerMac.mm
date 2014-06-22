/*
 * Copyright (C) 2013 Apple Computer, Inc.  All rights reserved.
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
#include "PlatformSpeechSynthesizer.h"

#include "PlatformSpeechSynthesisUtterance.h"
#include "PlatformSpeechSynthesisVoice.h"
#include "WebCoreSystemInterface.h"
#include <AppKit/NSSpeechSynthesizer.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RetainPtr.h>

#if ENABLE(SPEECH_SYNTHESIS)

@interface WebSpeechSynthesisWrapper : NSObject<NSSpeechSynthesizerDelegate>
{
    WebCore::PlatformSpeechSynthesizer* m_synthesizerObject;
    // Hold a Ref to the utterance so that it won't disappear until the synth is done with it.
    WebCore::PlatformSpeechSynthesisUtterance* m_utterance;
    
    RetainPtr<NSSpeechSynthesizer> m_synthesizer;
    float m_basePitch;
}

- (WebSpeechSynthesisWrapper *)initWithSpeechSynthesizer:(WebCore::PlatformSpeechSynthesizer *)synthesizer;
- (void)speakUtterance:(WebCore::PlatformSpeechSynthesisUtterance *)utterance;

@end

@implementation WebSpeechSynthesisWrapper

- (WebSpeechSynthesisWrapper *)initWithSpeechSynthesizer:(WebCore::PlatformSpeechSynthesizer *)synthesizer
{
    if (!(self = [super init]))
        return nil;
    
    m_synthesizerObject = synthesizer;
    [self updateBasePitchForSynthesizer];
    return self;
}

// NSSpeechSynthesizer expects a Words per Minute (WPM) rate. There is no preset default
// but they recommend that normal speaking is 180-220 WPM
- (float)convertRateToWPM:(float)rate
{
    // We'll say 200 WPM is the default 1x value.
    return 200.0f * rate;
}

- (float)convertPitchToNSSpeechValue:(float)pitch
{
    // This allows the base pitch to range from 0% - 200% of the normal pitch.
    return m_basePitch * pitch;
}

- (void)updateBasePitchForSynthesizer
{
    // Reset the base pitch whenever we change voices, since the base pitch is different for each voice.
    [m_synthesizer setObject:nil forProperty:NSSpeechResetProperty error:nil];
    m_basePitch = [[m_synthesizer objectForProperty:NSSpeechPitchBaseProperty error:nil] floatValue];
}

- (void)speakUtterance:(WebCore::PlatformSpeechSynthesisUtterance *)utterance
{
    // When speak is called we should not have an existing speech utterance outstanding.
    ASSERT(!m_utterance);
    ASSERT(utterance);
    
    if (!m_synthesizer) {
        m_synthesizer = [[NSSpeechSynthesizer alloc] initWithVoice:nil];
        [m_synthesizer setDelegate:self];
    }
    
    // Find if we should use a specific voice based on the voiceURI in utterance.
    // Otherwise, find the voice that matches the language. The Mac doesn't have a default voice per language, so the first
    // one will have to do.
    Vector<RefPtr<WebCore::PlatformSpeechSynthesisVoice> > voiceList = m_synthesizerObject->voiceList();
    size_t voiceListSize = voiceList.size();
    
    WebCore::PlatformSpeechSynthesisVoice* utteranceVoice = utterance->voice();
    // If no voice was specified, try to match by language.
    if (!utteranceVoice && !utterance->lang().isEmpty()) {
        for (size_t k = 0; k < voiceListSize; k++) {
            if (equalIgnoringCase(utterance->lang(), voiceList[k]->lang())) {
                utteranceVoice = voiceList[k].get();
                if (voiceList[k]->isDefault())
                    break;
            }
        }
    }
    
    NSString *voiceURI = nil;
    if (utteranceVoice)
        voiceURI = utteranceVoice->voiceURI();
    else
        voiceURI = [NSSpeechSynthesizer defaultVoice];

    // Don't set the voice unless necessary. There's a bug in NSSpeechSynthesizer such that
    // setting the voice for the first time will cause the first speechDone callback to report it was unsuccessful.
    BOOL updatePitch = NO;
    if (![[m_synthesizer voice] isEqualToString:voiceURI]) {
        [m_synthesizer setVoice:voiceURI];
        // Reset the base pitch whenever we change voices.
        updatePitch = YES;
    }
    
    if (m_basePitch == 0 || updatePitch)
        [self updateBasePitchForSynthesizer];    
    
    [m_synthesizer setObject:[NSNumber numberWithFloat:[self convertPitchToNSSpeechValue:utterance->pitch()]] forProperty:NSSpeechPitchBaseProperty error:nil];
    [m_synthesizer setRate:[self convertRateToWPM:utterance->rate()]];
    [m_synthesizer setVolume:utterance->volume()];
    
    m_utterance = utterance;
    [m_synthesizer startSpeakingString:utterance->text()];
    m_synthesizerObject->client()->didStartSpeaking(utterance);
}

- (void)pause
{
    if (!m_utterance)
        return;
    
    [m_synthesizer pauseSpeakingAtBoundary:NSSpeechImmediateBoundary];
    m_synthesizerObject->client()->didPauseSpeaking(m_utterance);
}

- (void)resume
{
    if (!m_utterance)
        return;
    
    [m_synthesizer continueSpeaking];
    m_synthesizerObject->client()->didResumeSpeaking(m_utterance);
}

- (void)cancel
{
    if (!m_utterance)
        return;
    
    [m_synthesizer stopSpeakingAtBoundary:NSSpeechImmediateBoundary];
    m_synthesizerObject->client()->speakingErrorOccurred(m_utterance);
    m_utterance = 0;
}

- (void)speechSynthesizer:(NSSpeechSynthesizer *)sender didFinishSpeaking:(BOOL)finishedSpeaking
{
    if (!m_utterance)
        return;
    
    UNUSED_PARAM(sender);
    
    // Clear the m_utterance variable in case finish speaking kicks off a new speaking job immediately.
    WebCore::PlatformSpeechSynthesisUtterance* utterance = m_utterance;
    m_utterance = 0;
    
    if (finishedSpeaking)
        m_synthesizerObject->client()->didFinishSpeaking(utterance);
    else
        m_synthesizerObject->client()->speakingErrorOccurred(utterance);
}

- (void)speechSynthesizer:(NSSpeechSynthesizer *)sender willSpeakWord:(NSRange)characterRange ofString:(NSString *)string
{
    UNUSED_PARAM(sender);
    UNUSED_PARAM(string);

    if (!m_utterance)
        return;

    // Mac platform only supports word boundaries.
    m_synthesizerObject->client()->boundaryEventOccurred(m_utterance, WebCore::SpeechWordBoundary, characterRange.location);
}

@end

namespace WebCore {

PlatformSpeechSynthesizer::PlatformSpeechSynthesizer(PlatformSpeechSynthesizerClient* client)
    : m_voiceListIsInitialized(false)
    , m_speechSynthesizerClient(client)
{
}

PlatformSpeechSynthesizer::~PlatformSpeechSynthesizer()
{
}

void PlatformSpeechSynthesizer::initializeVoiceList()
{
    NSArray *availableVoices = wkSpeechSynthesisGetVoiceIdentifiers();
    NSUInteger count = [availableVoices count];
    for (NSUInteger k = 0; k < count; k++) {
        NSString *voiceName = [availableVoices objectAtIndex:k];
        NSDictionary *attributes = [NSSpeechSynthesizer attributesForVoice:voiceName];
        
        NSString *voiceURI = [attributes objectForKey:NSVoiceIdentifier];
        NSString *name = [attributes objectForKey:NSVoiceName];
        NSString *language = [attributes objectForKey:NSVoiceLocaleIdentifier];
        NSLocale *locale = [[NSLocale alloc] initWithLocaleIdentifier:language];
        NSString *defaultVoiceURI = wkSpeechSynthesisGetDefaultVoiceIdentifierForLocale(locale);
        [locale release];
        
        // Change to BCP-47 format as defined by spec.
        language = [language stringByReplacingOccurrencesOfString:@"_" withString:@"-"];
        
        bool isDefault = [defaultVoiceURI isEqualToString:voiceURI];
        
        m_voiceList.append(PlatformSpeechSynthesisVoice::create(voiceURI, name, language, true, isDefault));
    }
}
    
void PlatformSpeechSynthesizer::pause()
{
    [m_platformSpeechWrapper.get() pause];
}

void PlatformSpeechSynthesizer::resume()
{
    [m_platformSpeechWrapper.get() resume];
}
    
void PlatformSpeechSynthesizer::speak(PassRefPtr<PlatformSpeechSynthesisUtterance> utterance)
{
    if (!m_platformSpeechWrapper)
        m_platformSpeechWrapper = adoptNS([[WebSpeechSynthesisWrapper alloc] initWithSpeechSynthesizer:this]);
    
    [m_platformSpeechWrapper.get() speakUtterance:utterance.get()];
}

void PlatformSpeechSynthesizer::cancel()
{
    [m_platformSpeechWrapper.get() cancel];
}
    
} // namespace WebCore

#endif // ENABLE(SPEECH_SYNTHESIS)
