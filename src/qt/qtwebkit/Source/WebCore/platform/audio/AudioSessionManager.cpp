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
#include "AudioSessionManager.h"

#if USE(AUDIO_SESSION)

using namespace WebCore;

PassOwnPtr<AudioSessionManagerToken> AudioSessionManagerToken::create(AudioSessionManager::AudioType type)
{
    return adoptPtr(new AudioSessionManagerToken(type));
}

AudioSessionManagerToken::AudioSessionManagerToken(AudioSessionManager::AudioType type)
    : m_type(type)
{
    AudioSessionManager::sharedManager().incrementCount(type);
}

AudioSessionManagerToken::~AudioSessionManagerToken()
{
    AudioSessionManager::sharedManager().decrementCount(m_type);
}

AudioSessionManager& AudioSessionManager::sharedManager()
{
    DEFINE_STATIC_LOCAL(AudioSessionManager, manager, ());
    return manager;
}

AudioSessionManager::AudioSessionManager()
{
}

bool AudioSessionManager::has(AudioSessionManager::AudioType type)
{
    ASSERT(type >= 0);
    return m_typeCount.contains(type);
}

void AudioSessionManager::incrementCount(AudioSessionManager::AudioType type)
{
    ASSERT(type >= 0);
    m_typeCount.add(type);
    updateSessionState();
}

void AudioSessionManager::decrementCount(AudioSessionManager::AudioType type)
{
    ASSERT(type >= 0);
    m_typeCount.remove(type);
    updateSessionState();
}

#if !PLATFORM(MAC)
void AudioSessionManager::updateSessionState()
{
}
#endif

#endif // USE(AUDIO_SESSION)
