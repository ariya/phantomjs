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

#ifndef SpeechRecognition_h
#define SpeechRecognition_h

#if ENABLE(SCRIPTED_SPEECH)

#include "ActiveDOMObject.h"
#include "EventTarget.h"
#include "SpeechGrammarList.h"
#include <wtf/Compiler.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ScriptExecutionContext;
class SpeechRecognitionController;
class SpeechRecognitionError;
class SpeechRecognitionResult;
class SpeechRecognitionResultList;

class SpeechRecognition : public RefCounted<SpeechRecognition>, public ActiveDOMObject, public EventTarget {
public:
    static PassRefPtr<SpeechRecognition> create(ScriptExecutionContext*);
    ~SpeechRecognition();

    // Attributes.
    PassRefPtr<SpeechGrammarList> grammars() { return m_grammars; }
    void setGrammars(PassRefPtr<SpeechGrammarList> grammars) { m_grammars = grammars; }
    String lang() { return m_lang; }
    void setLang(const String& lang) { m_lang = lang; }
    bool continuous() { return m_continuous; }
    void setContinuous(bool continuous) { m_continuous = continuous; }
    bool interimResults() { return m_interimResults; }
    void setInterimResults(bool interimResults) { m_interimResults = interimResults; }
    unsigned long maxAlternatives() { return m_maxAlternatives; }
    void setMaxAlternatives(unsigned long maxAlternatives) { m_maxAlternatives = maxAlternatives; }

    // Callable by the user.
    void start(ExceptionCode&);
    void stopFunction();
    void abort();

    // Called by the SpeechRecognitionClient.
    void didStartAudio();
    void didStartSound();
    void didStartSpeech();
    void didEndSpeech();
    void didEndSound();
    void didEndAudio();
    void didReceiveResults(const Vector<RefPtr<SpeechRecognitionResult> >& newFinalResults, const Vector<RefPtr<SpeechRecognitionResult> >& currentInterimResults);
    void didReceiveNoMatch(PassRefPtr<SpeechRecognitionResult>);
    void didReceiveError(PassRefPtr<SpeechRecognitionError>);
    void didStart();
    void didEnd();

    // EventTarget.
    virtual const AtomicString& interfaceName() const OVERRIDE;
    virtual ScriptExecutionContext* scriptExecutionContext() const OVERRIDE;

    // ActiveDOMObject.
    virtual void stop() OVERRIDE;

    using RefCounted<SpeechRecognition>::ref;
    using RefCounted<SpeechRecognition>::deref;

    DEFINE_ATTRIBUTE_EVENT_LISTENER(audiostart);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(soundstart);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(speechstart);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(speechend);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(soundend);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(audioend);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(result);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(nomatch);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(start);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(end);

private:
    friend class RefCounted<SpeechRecognition>;

    explicit SpeechRecognition(ScriptExecutionContext*);


    // EventTarget
    virtual void refEventTarget() OVERRIDE { ref(); }
    virtual void derefEventTarget() OVERRIDE { deref(); }
    virtual EventTargetData* eventTargetData() OVERRIDE { return &m_eventTargetData; }
    virtual EventTargetData* ensureEventTargetData() OVERRIDE { return &m_eventTargetData; }

    RefPtr<SpeechGrammarList> m_grammars;
    String m_lang;
    bool m_continuous;
    bool m_interimResults;
    unsigned long m_maxAlternatives;

    EventTargetData m_eventTargetData;

    SpeechRecognitionController* m_controller;
    bool m_stoppedByActiveDOMObject;
    bool m_started;
    bool m_stopping;
    Vector<RefPtr<SpeechRecognitionResult> > m_finalResults;
};

} // namespace WebCore

#endif // ENABLE(SCRIPTED_SPEECH)

#endif // SpeechRecognition_h
