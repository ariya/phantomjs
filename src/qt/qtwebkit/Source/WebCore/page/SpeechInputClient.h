/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright
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

#ifndef SpeechInputClient_h
#define SpeechInputClient_h

#include <wtf/Forward.h>

#if ENABLE(INPUT_SPEECH)

namespace WebCore {

class IntRect;
class SecurityOrigin;
class SpeechInputListener;
class Page;

// Provides an interface for SpeechInput to call into the embedder.
class SpeechInputClient {
public:
    // This is the first call made by a listener, registering itself for future callbacks.
    // When the listener no longer needs speech input (for e.g. when it gets destroyed),
    // it should set a null listener to stop receiving callbacks.
    // The client does not take ownership of the pointer.
    virtual void setListener(SpeechInputListener*) = 0;

    // Starts speech recognition and audio recording. elementRect is the position
    // of the element where the user clicked in the RootView coordinate system.
    virtual bool startRecognition(int requestId, const IntRect& elementRect, const AtomicString& language, const String& grammar, SecurityOrigin*) = 0;

    // Stops audio recording and performs recognition with the audio recorded until now
    // (does not discard audio).
    virtual void stopRecording(int requestId) = 0;

    // Cancels an ongoing recognition and discards any audio recorded so far. No partial
    // recognition results are returned to the listener.
    virtual void cancelRecognition(int requestId) = 0;

protected:
    virtual ~SpeechInputClient() { }
};

void provideSpeechInputTo(Page*, SpeechInputClient*);

} // namespace WebCore

#endif // ENABLE(INPUT_SPEECH)

#endif // SpeechInputClient_h
