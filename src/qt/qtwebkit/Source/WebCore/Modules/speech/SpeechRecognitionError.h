/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SpeechRecognitionError_h
#define SpeechRecognitionError_h

#if ENABLE(SCRIPTED_SPEECH)

#include "Event.h"
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

struct SpeechRecognitionErrorInit : public EventInit {
    SpeechRecognitionErrorInit();

    String error;
    String message;
};

class SpeechRecognitionError : public Event {
public:
    enum ErrorCode {
        // FIXME: This is an unspecified error and Chromium should stop using it.
        ErrorCodeOther = 0,

        ErrorCodeNoSpeech = 1,
        ErrorCodeAborted = 2,
        ErrorCodeAudioCapture = 3,
        ErrorCodeNetwork = 4,
        ErrorCodeNotAllowed = 5,
        ErrorCodeServiceNotAllowed = 6,
        ErrorCodeBadGrammar = 7,
        ErrorCodeLanguageNotSupported = 8
    };

    static PassRefPtr<SpeechRecognitionError> create(ErrorCode, const String&);
    static PassRefPtr<SpeechRecognitionError> create();
    static PassRefPtr<SpeechRecognitionError> create(const AtomicString&, const SpeechRecognitionErrorInit&);

    const String& error() { return m_error; }
    const String& message() { return m_message; }

    virtual const AtomicString& interfaceName() const OVERRIDE;

private:
    SpeechRecognitionError(const String&, const String&);
    SpeechRecognitionError(const AtomicString&, const SpeechRecognitionErrorInit&);

    String m_error;
    String m_message;
};

} // namespace WebCore

#endif // ENABLE(SCRIPTED_SPEECH)

#endif // SpeechRecognitionError_h
