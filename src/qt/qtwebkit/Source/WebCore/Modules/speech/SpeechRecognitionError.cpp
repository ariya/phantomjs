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

#include "config.h"

#if ENABLE(SCRIPTED_SPEECH)

#include "SpeechRecognitionError.h"

namespace WebCore {

static String ErrorCodeToString(SpeechRecognitionError::ErrorCode code)
{
    switch (code) {
    case SpeechRecognitionError::ErrorCodeOther:
        return ASCIILiteral("other");
    case SpeechRecognitionError::ErrorCodeNoSpeech:
        return ASCIILiteral("no-speech");
    case SpeechRecognitionError::ErrorCodeAborted:
        return ASCIILiteral("aborted");
    case SpeechRecognitionError::ErrorCodeAudioCapture:
        return ASCIILiteral("audio-capture");
    case SpeechRecognitionError::ErrorCodeNetwork:
        return ASCIILiteral("network");
    case SpeechRecognitionError::ErrorCodeNotAllowed:
        return ASCIILiteral("not-allowed");
    case SpeechRecognitionError::ErrorCodeServiceNotAllowed:
        return ASCIILiteral("service-not-allowed");
    case SpeechRecognitionError::ErrorCodeBadGrammar:
        return ASCIILiteral("bad-grammar");
    case SpeechRecognitionError::ErrorCodeLanguageNotSupported:
        return ASCIILiteral("language-not-supported");
    }

    ASSERT_NOT_REACHED();
    return String();
}

PassRefPtr<SpeechRecognitionError> SpeechRecognitionError::create(ErrorCode code, const String& message)
{
    return adoptRef(new SpeechRecognitionError(ErrorCodeToString(code), message));
}

PassRefPtr<SpeechRecognitionError> SpeechRecognitionError::create()
{
    return adoptRef(new SpeechRecognitionError(emptyString(), emptyString()));
}

PassRefPtr<SpeechRecognitionError> SpeechRecognitionError::create(const AtomicString& eventName, const SpeechRecognitionErrorInit& initializer)
{
    return adoptRef(new SpeechRecognitionError(eventName, initializer));
}

SpeechRecognitionError::SpeechRecognitionError(const String& error, const String& message)
    : Event(eventNames().errorEvent, /*canBubble=*/false, /*cancelable=*/false)
    , m_error(error)
    , m_message(message)
{
}

SpeechRecognitionError::SpeechRecognitionError(const AtomicString& eventName, const SpeechRecognitionErrorInit& initializer)
    : Event(eventName, initializer)
    , m_error(initializer.error)
    , m_message(initializer.message)
{
}

const AtomicString& SpeechRecognitionError::interfaceName() const
{
    return eventNames().interfaceForSpeechRecognitionError;
}

SpeechRecognitionErrorInit::SpeechRecognitionErrorInit()
{
}

} // namespace WebCore

#endif // ENABLE(SCRIPTED_SPEECH)
