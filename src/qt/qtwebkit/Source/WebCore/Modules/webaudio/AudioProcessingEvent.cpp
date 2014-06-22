/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioProcessingEvent.h"

#include "AudioBuffer.h"
#include "EventNames.h"

namespace WebCore {

PassRefPtr<AudioProcessingEvent> AudioProcessingEvent::create()
{
    return adoptRef(new AudioProcessingEvent);
}

PassRefPtr<AudioProcessingEvent> AudioProcessingEvent::create(PassRefPtr<AudioBuffer> inputBuffer, PassRefPtr<AudioBuffer> outputBuffer)
{
    return adoptRef(new AudioProcessingEvent(inputBuffer, outputBuffer));
}

AudioProcessingEvent::AudioProcessingEvent()
{
}

AudioProcessingEvent::AudioProcessingEvent(PassRefPtr<AudioBuffer> inputBuffer, PassRefPtr<AudioBuffer> outputBuffer)
    : Event(eventNames().audioprocessEvent, true, false)
    , m_inputBuffer(inputBuffer)
    , m_outputBuffer(outputBuffer)
{
}

AudioProcessingEvent::~AudioProcessingEvent()
{
}

const AtomicString& AudioProcessingEvent::interfaceName() const
{
    return eventNames().interfaceForAudioProcessingEvent;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
