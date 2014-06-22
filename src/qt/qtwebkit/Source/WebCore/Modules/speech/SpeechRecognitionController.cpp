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
#include "SpeechRecognitionController.h"

#if ENABLE(SCRIPTED_SPEECH)

namespace WebCore {

const char* SpeechRecognitionController::supplementName()
{
    return "SpeechRecognitionController";
}

SpeechRecognitionController::SpeechRecognitionController(SpeechRecognitionClient* client)
    : m_client(client)
{
}

SpeechRecognitionController::~SpeechRecognitionController()
{
    // FIXME: Call m_client->pageDestroyed(); once we have implemented a client.
}

PassOwnPtr<SpeechRecognitionController> SpeechRecognitionController::create(SpeechRecognitionClient* client)
{
    return adoptPtr(new SpeechRecognitionController(client));
}

void provideSpeechRecognitionTo(Page* page, SpeechRecognitionClient* client)
{
    SpeechRecognitionController::provideTo(page, SpeechRecognitionController::supplementName(), SpeechRecognitionController::create(client));
}

} // namespace WebCore

#endif // ENABLE(SCRIPTED_SPEECH)
