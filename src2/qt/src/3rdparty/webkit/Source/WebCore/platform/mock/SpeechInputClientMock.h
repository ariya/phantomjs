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

#ifndef SpeechInputClientMock_h
#define SpeechInputClientMock_h

#include "PlatformString.h"
#include "SpeechInputClient.h"
#include "SpeechInputResult.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>

#if ENABLE(INPUT_SPEECH)

namespace WebCore {

class SpeechInputListener;

// Provides a mock object for the speech input embedder API called by WebCore.
class SpeechInputClientMock : public SpeechInputClient {
public:
    SpeechInputClientMock();

    void addRecognitionResult(const String& result, double confidence, const AtomicString& language);
    void clearResults();

    // SpeechInputClient methods.
    void setListener(SpeechInputListener*);
    bool startRecognition(int requestId, const IntRect& elementRect, const AtomicString& language, const String& grammar, SecurityOrigin*);
    void stopRecording(int);
    void cancelRecognition(int);

private:
    void timerFired(Timer<SpeechInputClientMock>*);

    bool m_recording;
    Timer<SpeechInputClientMock> m_timer;
    SpeechInputListener* m_listener;
    int m_requestId;

    HashMap<String, SpeechInputResultArray> m_recognitionResults;
    AtomicString m_language;
    SpeechInputResultArray m_resultsForEmptyLanguage;
};

} // namespace WebCore

#endif // ENABLE(INPUT_SPEECH)

#endif // SpeechInputClientMock_h
