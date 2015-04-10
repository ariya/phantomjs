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

#include "config.h"
#include "AudioSession.h"

#if USE(AUDIO_SESSION)

#include "AudioSessionListener.h"
#include "NotImplemented.h"

namespace WebCore {

AudioSession& AudioSession::sharedSession()
{
    DEFINE_STATIC_LOCAL(AudioSession, session, ());
    return session;
}

void AudioSession::addListener(AudioSessionListener* listener)
{
    m_listeners.add(listener);
}

void AudioSession::removeListener(AudioSessionListener* listener)
{
    m_listeners.remove(listener);
}

void AudioSession::beganAudioInterruption()
{
    for (HashSet<AudioSessionListener*>::iterator i = m_listeners.begin(); i != m_listeners.end(); ++i)
        (*i)->beganAudioInterruption();
}

void AudioSession::endedAudioInterruption()
{
    for (HashSet<AudioSessionListener*>::iterator i = m_listeners.begin(); i != m_listeners.end(); ++i)
        (*i)->endedAudioInterruption();
}

#if !PLATFORM(IOS) && !PLATFORM(MAC)
class AudioSessionPrivate {
};

AudioSession::AudioSession()
    : m_private(nullptr)
{
    notImplemented();
}

AudioSession::~AudioSession()
{
}

void AudioSession::setCategory(CategoryType)
{
    notImplemented();
}

AudioSession::CategoryType AudioSession::categoryOverride() const
{
    notImplemented();
    return None;
}

void AudioSession::setCategoryOverride(CategoryType)
{
    notImplemented();
}

AudioSession::CategoryType AudioSession::category() const
{
    notImplemented();
    return None;
}

float AudioSession::sampleRate() const
{
    notImplemented();
    return 0;
}

size_t AudioSession::numberOfOutputChannels() const
{
    notImplemented();
    return 0;
}

void AudioSession::setActive(bool)
{
    notImplemented();
}

size_t AudioSession::preferredBufferSize() const
{
    notImplemented();
    return 0;
}

void AudioSession::setPreferredBufferSize(size_t)
{
    notImplemented();
}
#endif // !PLATFORM(IOS)

}

#endif // USE(AUDIO_SESSION)
