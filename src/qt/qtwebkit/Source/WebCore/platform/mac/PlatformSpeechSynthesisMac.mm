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
#include "PlatformSpeechSynthesis.h"

#include "SpeechSynthesisVoice.h"
#include "SpeechSynthesisUtterance.h"
#include <AppKit/NSSpeechSynthesizer.h>

#if ENABLE(SPEECH_SYNTHESIS)

namespace WebCore {

PassRefPtr<PlatformSpeechSynthesis> PlatformSpeechSynthesis::create(SpeechSynthesis* synthesis)
{
    return adoptRef(new PlatformSpeechSynthesis(synthesis));
}

PlatformSpeechSynthesis::PlatformSpeechSynthesis(SpeechSynthesis* synthesis)
    : m_speechSynthesis(synthesis)
{
}
    
void PlatformSpeechSynthesis::platformSpeak(SpeechSynthesisUtterance*)
{
}
    
void PlatformSpeechSynthesis::platformInitializeVoiceList(Vector<RefPtr<SpeechSynthesisVoice> >& voices);
{
    NSString *defaultVoiceURI = [NSSpeechSynthesizer defaultVoice];
    NSArray *availableVoices = [NSSpeechSynthesizer availableVoices];
    NSUInteger count = [availableVoices count];
    for (NSUInteger k = 0; k < count; k++) {
        NSString *voiceName = [availableVoices objectAtIndex:k];
        NSDictionary *attributes = [NSSpeechSynthesizer attributesForVoice:voiceName];
        
        NSString *voiceURI = [attributes objectForKey:NSVoiceIdentifier];
        NSString *name = [attributes objectForKey:NSVoiceName];
        NSString *language = [attributes objectForKey:NSVoiceLocaleIdentifier];
        
        // Change to BCP-47 format as defined by spec.
        language = [language stringByReplacingOccurrencesOfString:@"_" withString:@"-"];
        
        bool isDefault = [defaultVoiceURI isEqualToString:voiceURI];
        
        voices.append(SpeechSynthesisVoice::create(voiceURI, name, language, true, isDefault));
    }
}

} // namespace WebCore

#endif // ENABLE(SPEECH_SYNTHESIS)
