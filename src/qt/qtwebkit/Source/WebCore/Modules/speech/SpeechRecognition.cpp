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

#include "SpeechRecognition.h"

#include "Document.h"
#include "ExceptionCode.h"
#include "Page.h"
#include "SpeechRecognitionController.h"
#include "SpeechRecognitionError.h"
#include "SpeechRecognitionEvent.h"

namespace WebCore {

PassRefPtr<SpeechRecognition> SpeechRecognition::create(ScriptExecutionContext* context)
{
    RefPtr<SpeechRecognition> speechRecognition(adoptRef(new SpeechRecognition(context)));
    speechRecognition->suspendIfNeeded();
    return speechRecognition.release();
}

void SpeechRecognition::start(ExceptionCode& ec)
{
    ASSERT(m_controller);
    if (m_started) {
        ec = INVALID_STATE_ERR;
        return;
    }

    setPendingActivity(this);
    m_finalResults.clear();
    m_controller->start(this, m_grammars.get(), m_lang, m_continuous, m_interimResults, m_maxAlternatives);
    m_started = true;
}

void SpeechRecognition::stopFunction()
{
    ASSERT(m_controller);
    if (m_started && !m_stopping) {
        m_stopping = true;
        m_controller->stop(this);
    }
}

void SpeechRecognition::abort()
{
    ASSERT(m_controller);
    if (m_started && !m_stopping) {
        m_stopping = true;
        m_controller->abort(this);
    }
}

void SpeechRecognition::didStartAudio()
{
    dispatchEvent(Event::create(eventNames().audiostartEvent, /*canBubble=*/false, /*cancelable=*/false));
}

void SpeechRecognition::didStartSound()
{
    dispatchEvent(Event::create(eventNames().soundstartEvent, /*canBubble=*/false, /*cancelable=*/false));
}

void SpeechRecognition::didStartSpeech()
{
    dispatchEvent(Event::create(eventNames().speechstartEvent, /*canBubble=*/false, /*cancelable=*/false));
}

void SpeechRecognition::didEndSpeech()
{
    dispatchEvent(Event::create(eventNames().speechendEvent, /*canBubble=*/false, /*cancelable=*/false));
}

void SpeechRecognition::didEndSound()
{
    dispatchEvent(Event::create(eventNames().soundendEvent, /*canBubble=*/false, /*cancelable=*/false));
}

void SpeechRecognition::didEndAudio()
{
    dispatchEvent(Event::create(eventNames().audioendEvent, /*canBubble=*/false, /*cancelable=*/false));
}

void SpeechRecognition::didReceiveResults(const Vector<RefPtr<SpeechRecognitionResult> >& newFinalResults, const Vector<RefPtr<SpeechRecognitionResult> >& currentInterimResults)
{
    unsigned long resultIndex = m_finalResults.size();

    for (size_t i = 0; i < newFinalResults.size(); ++i)
        m_finalResults.append(newFinalResults[i]);

    Vector<RefPtr<SpeechRecognitionResult> > results = m_finalResults;
    for (size_t i = 0; i < currentInterimResults.size(); ++i)
        results.append(currentInterimResults[i]);

    dispatchEvent(SpeechRecognitionEvent::createResult(resultIndex, results));
}

void SpeechRecognition::didReceiveNoMatch(PassRefPtr<SpeechRecognitionResult> result)
{
    dispatchEvent(SpeechRecognitionEvent::createNoMatch(result));
}

void SpeechRecognition::didReceiveError(PassRefPtr<SpeechRecognitionError> error)
{
    dispatchEvent(error);
    m_started = false;
}

void SpeechRecognition::didStart()
{
    dispatchEvent(Event::create(eventNames().startEvent, /*canBubble=*/false, /*cancelable=*/false));
}

void SpeechRecognition::didEnd()
{
    m_started = false;
    m_stopping = false;
    if (!m_stoppedByActiveDOMObject)
        dispatchEvent(Event::create(eventNames().endEvent, /*canBubble=*/false, /*cancelable=*/false));
    unsetPendingActivity(this);
}

const AtomicString& SpeechRecognition::interfaceName() const
{
    return eventNames().interfaceForSpeechRecognition;
}

ScriptExecutionContext* SpeechRecognition::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

void SpeechRecognition::stop()
{
    m_stoppedByActiveDOMObject = true;
    if (hasPendingActivity())
        abort();
}

SpeechRecognition::SpeechRecognition(ScriptExecutionContext* context)
    : ActiveDOMObject(context)
    , m_grammars(SpeechGrammarList::create()) // FIXME: The spec is not clear on the default value for the grammars attribute.
    , m_continuous(false)
    , m_interimResults(false)
    , m_maxAlternatives(1)
    , m_controller(0)
    , m_stoppedByActiveDOMObject(false)
    , m_started(false)
    , m_stopping(false)
{
    Document* document = toDocument(scriptExecutionContext());

    Page* page = document->page();
    ASSERT(page);

    m_controller = SpeechRecognitionController::from(page);
    ASSERT(m_controller);

    // FIXME: Need to hook up with Page to get notified when the visibility changes.
}

SpeechRecognition::~SpeechRecognition()
{
}

} // namespace WebCore

#endif // ENABLE(SCRIPTED_SPEECH)
