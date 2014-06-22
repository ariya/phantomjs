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
#include "SpeechSynthesis.h"

#if ENABLE(SPEECH_SYNTHESIS)

#include "PlatformSpeechSynthesisVoice.h"
#include "PlatformSpeechSynthesizer.h"
#include "SpeechSynthesisEvent.h"
#include "SpeechSynthesisUtterance.h"
#include <wtf/CurrentTime.h>

namespace WebCore {
    
PassRefPtr<SpeechSynthesis> SpeechSynthesis::create()
{
    return adoptRef(new SpeechSynthesis);
}
    
SpeechSynthesis::SpeechSynthesis()
    : m_currentSpeechUtterance(0)
    , m_isPaused(false)
{
}
    
void SpeechSynthesis::setPlatformSynthesizer(PassOwnPtr<PlatformSpeechSynthesizer> synthesizer)
{
    m_platformSpeechSynthesizer = synthesizer;
    m_voiceList.clear();
    m_currentSpeechUtterance = 0;
    m_utteranceQueue.clear();
    m_isPaused = false;
}
    
void SpeechSynthesis::voicesDidChange()
{
    m_voiceList.clear();
}
    
const Vector<RefPtr<SpeechSynthesisVoice> >& SpeechSynthesis::getVoices()
{
    if (m_voiceList.size())
        return m_voiceList;

    if (!m_platformSpeechSynthesizer)
        m_platformSpeechSynthesizer = PlatformSpeechSynthesizer::create(this);

    // If the voiceList is empty, that's the cue to get the voices from the platform again.
    const Vector<RefPtr<PlatformSpeechSynthesisVoice> >& platformVoices = m_platformSpeechSynthesizer->voiceList();
    size_t voiceCount = platformVoices.size();
    for (size_t k = 0; k < voiceCount; k++)
        m_voiceList.append(SpeechSynthesisVoice::create(platformVoices[k]));

    return m_voiceList;
}

bool SpeechSynthesis::speaking() const
{
    // If we have a current speech utterance, then that means we're assumed to be in a speaking state.
    // This state is independent of whether the utterance happens to be paused.
    return m_currentSpeechUtterance;
}

bool SpeechSynthesis::pending() const
{
    // This is true if there are any utterances that have not started.
    // That means there will be more than one in the queue.
    return m_utteranceQueue.size() > 1;
}

bool SpeechSynthesis::paused() const
{
    return m_isPaused;
}

void SpeechSynthesis::startSpeakingImmediately(SpeechSynthesisUtterance* utterance)
{
    ASSERT(!m_currentSpeechUtterance);
    utterance->setStartTime(monotonicallyIncreasingTime());
    m_currentSpeechUtterance = utterance;
    m_isPaused = false;
    if (!m_platformSpeechSynthesizer)
        m_platformSpeechSynthesizer = PlatformSpeechSynthesizer::create(this);
    m_platformSpeechSynthesizer->speak(utterance->platformUtterance());
}

void SpeechSynthesis::speak(SpeechSynthesisUtterance* utterance)
{
    if (!utterance)
        return;
    
    m_utteranceQueue.append(utterance);
    
    // If the queue was empty, speak this immediately and add it to the queue.
    if (m_utteranceQueue.size() == 1)
        startSpeakingImmediately(utterance);
}

void SpeechSynthesis::cancel()
{
    // Remove all the items from the utterance queue.
    // Hold on to the current utterance so the platform synthesizer can have a chance to clean up.
    RefPtr<SpeechSynthesisUtterance> current = m_currentSpeechUtterance;
    m_utteranceQueue.clear();
    if (m_platformSpeechSynthesizer)
        m_platformSpeechSynthesizer->cancel();
    current = 0;
    
    // The platform should have called back immediately and cleared the current utterance.
    ASSERT(!m_currentSpeechUtterance);
}

void SpeechSynthesis::pause()
{
    if (!m_isPaused && m_platformSpeechSynthesizer)
        m_platformSpeechSynthesizer->pause();
}

void SpeechSynthesis::resume()
{
    if (m_currentSpeechUtterance && m_platformSpeechSynthesizer)
        m_platformSpeechSynthesizer->resume();
}

void SpeechSynthesis::fireEvent(const AtomicString& type, SpeechSynthesisUtterance* utterance, unsigned long charIndex, const String& name)
{
    utterance->dispatchEvent(SpeechSynthesisEvent::create(type, charIndex, (monotonicallyIncreasingTime() - utterance->startTime()), name));
}
    
void SpeechSynthesis::handleSpeakingCompleted(SpeechSynthesisUtterance* utterance, bool errorOccurred)
{
    ASSERT(utterance);
    ASSERT(m_currentSpeechUtterance);
    m_currentSpeechUtterance = 0;

    fireEvent(errorOccurred ? eventNames().errorEvent : eventNames().endEvent, utterance, 0, String());

    if (m_utteranceQueue.size()) {
        RefPtr<SpeechSynthesisUtterance> firstUtterance = m_utteranceQueue.first();
        ASSERT(firstUtterance == utterance);
        if (firstUtterance == utterance)
            m_utteranceQueue.removeFirst();
        
        // Start the next job if there is one pending.
        if (!m_utteranceQueue.isEmpty())
            startSpeakingImmediately(m_utteranceQueue.first().get());
    }
}
    
void SpeechSynthesis::boundaryEventOccurred(PassRefPtr<PlatformSpeechSynthesisUtterance> utterance, SpeechBoundary boundary, unsigned charIndex)
{
    DEFINE_STATIC_LOCAL(const String, wordBoundaryString, (ASCIILiteral("word")));
    DEFINE_STATIC_LOCAL(const String, sentenceBoundaryString, (ASCIILiteral("sentence")));

    switch (boundary) {
    case SpeechWordBoundary:
        fireEvent(eventNames().boundaryEvent, static_cast<SpeechSynthesisUtterance*>(utterance->client()), charIndex, wordBoundaryString);
        break;
    case SpeechSentenceBoundary:
        fireEvent(eventNames().boundaryEvent, static_cast<SpeechSynthesisUtterance*>(utterance->client()), charIndex, sentenceBoundaryString);
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

void SpeechSynthesis::didStartSpeaking(PassRefPtr<PlatformSpeechSynthesisUtterance> utterance)
{
    if (utterance->client())
        fireEvent(eventNames().startEvent, static_cast<SpeechSynthesisUtterance*>(utterance->client()), 0, String());
}
    
void SpeechSynthesis::didPauseSpeaking(PassRefPtr<PlatformSpeechSynthesisUtterance> utterance)
{
    m_isPaused = true;
    if (utterance->client())
        fireEvent(eventNames().pauseEvent, static_cast<SpeechSynthesisUtterance*>(utterance->client()), 0, String());
}

void SpeechSynthesis::didResumeSpeaking(PassRefPtr<PlatformSpeechSynthesisUtterance> utterance)
{
    m_isPaused = false;
    if (utterance->client())
        fireEvent(eventNames().resumeEvent, static_cast<SpeechSynthesisUtterance*>(utterance->client()), 0, String());
}

void SpeechSynthesis::didFinishSpeaking(PassRefPtr<PlatformSpeechSynthesisUtterance> utterance)
{
    if (utterance->client())
        handleSpeakingCompleted(static_cast<SpeechSynthesisUtterance*>(utterance->client()), false);
}
    
void SpeechSynthesis::speakingErrorOccurred(PassRefPtr<PlatformSpeechSynthesisUtterance> utterance)
{
    if (utterance->client())
        handleSpeakingCompleted(static_cast<SpeechSynthesisUtterance*>(utterance->client()), true);
}

} // namespace WebCore

#endif // ENABLE(INPUT_SPEECH)
