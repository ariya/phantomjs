/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SpeechInput.h"

#if ENABLE(INPUT_SPEECH)

#include "SecurityOrigin.h"
#include "SpeechInputClient.h"
#include "SpeechInputListener.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

SpeechInput::SpeechInput(SpeechInputClient* client)
    : m_client(client)
    , m_nextListenerId(1)
{
    m_client->setListener(this);
}

SpeechInput::~SpeechInput()
{
    m_client->setListener(0);
}

PassOwnPtr<SpeechInput> SpeechInput::create(SpeechInputClient* client)
{
    return adoptPtr(new SpeechInput(client));
}

int SpeechInput::registerListener(SpeechInputListener* listener)
{
#if defined(DEBUG)
    // Check if already present.
    for (HashMap<int, SpeechInputListener*>::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
      ASSERT(it->value != listener);
#endif

    m_listeners.add(m_nextListenerId, listener);
    return m_nextListenerId++;
}

void SpeechInput::unregisterListener(int listenerId)
{
    if (m_listeners.contains(listenerId))
        m_listeners.remove(listenerId);
}

void SpeechInput::didCompleteRecording(int listenerId)
{
    // Don't assert if not present as the element might have been removed by the page while
    // this event was on the way.
    if (m_listeners.contains(listenerId))
        m_listeners.get(listenerId)->didCompleteRecording(listenerId);
}

void SpeechInput::didCompleteRecognition(int listenerId)
{
    // Don't assert if not present as the element might have been removed by the page while
    // this event was on the way.
    if (m_listeners.contains(listenerId))
        m_listeners.get(listenerId)->didCompleteRecognition(listenerId);
}

void SpeechInput::setRecognitionResult(int listenerId, const SpeechInputResultArray& result)
{
    // Don't assert if not present as the element might have been removed by the page while
    // this event was on the way.
    if (m_listeners.contains(listenerId))
        m_listeners.get(listenerId)->setRecognitionResult(listenerId, result);
}

bool SpeechInput::startRecognition(int listenerId, const IntRect& elementRect, const AtomicString& language, const String& grammar, SecurityOrigin* origin)
{
    ASSERT(m_listeners.contains(listenerId));
    return m_client->startRecognition(listenerId, elementRect, language, grammar, origin);
}

void SpeechInput::stopRecording(int listenerId)
{
    ASSERT(m_listeners.contains(listenerId));
    m_client->stopRecording(listenerId);
}

void SpeechInput::cancelRecognition(int listenerId)
{
    ASSERT(m_listeners.contains(listenerId));
    m_client->cancelRecognition(listenerId);
}

const char* SpeechInput::supplementName()
{
    return "SpeechInput";
}

void provideSpeechInputTo(Page* page, SpeechInputClient* client)
{
    SpeechInput::provideTo(page, SpeechInput::supplementName(), SpeechInput::create(client));
}

} // namespace WebCore

#endif // ENABLE(INPUT_SPEECH)
