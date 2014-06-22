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

#ifndef AudioSession_h
#define AudioSession_h

#if USE(AUDIO_SESSION)

#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

class AudioSessionListener;
class AudioSessionPrivate;

class AudioSession {
    WTF_MAKE_NONCOPYABLE(AudioSession);
public:
    static AudioSession& sharedSession();

    enum CategoryType {
        None,
        AmbientSound,
        SoloAmbientSound,
        MediaPlayback,
        RecordAudio,
        PlayAndRecord,
        AudioProcessing,
    };
    void setCategory(CategoryType);
    CategoryType category() const;

    void setCategoryOverride(CategoryType);
    CategoryType categoryOverride() const;

    void addListener(AudioSessionListener*);
    void removeListener(AudioSessionListener*);

    float sampleRate() const;
    size_t numberOfOutputChannels() const;

    void setActive(bool);

    size_t preferredBufferSize() const;
    void setPreferredBufferSize(size_t);

    void beganAudioInterruption();
    void endedAudioInterruption();

private:
    AudioSession();
    ~AudioSession();

    OwnPtr<AudioSessionPrivate> m_private;
    HashSet<AudioSessionListener*> m_listeners;
};

}

#endif // USE(AUDIO_SESSION)

#endif // AudioSession_h
