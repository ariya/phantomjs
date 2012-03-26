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

#include "AudioContext.h"

#include "ArrayBuffer.h"
#include "AudioBuffer.h"
#include "JSArrayBuffer.h"
#include "JSAudioBuffer.h"
#include "JSAudioContext.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

void JSAudioContext::visitChildren(SlotVisitor& visitor)
{
    Base::visitChildren(visitor);
    m_impl->visitJSEventListeners(visitor);
}

EncodedJSValue JSC_HOST_CALL JSAudioContextConstructor::constructJSAudioContext(ExecState* exec)
{
    JSAudioContextConstructor* jsConstructor = static_cast<JSAudioContextConstructor*>(exec->callee());
    if (!jsConstructor)
      return throwError(exec, createReferenceError(exec, "AudioContext constructor callee is unavailable"));

    ScriptExecutionContext* scriptExecutionContext = jsConstructor->scriptExecutionContext();
    if (!scriptExecutionContext)
      return throwError(exec, createReferenceError(exec, "AudioContext constructor script execution context is unavailable"));
        
    if (!scriptExecutionContext->isDocument())
      return throwError(exec, createReferenceError(exec, "AudioContext constructor called in a script execution context which is not a document"));

    Document* document = static_cast<Document*>(scriptExecutionContext);

    RefPtr<AudioContext> audioContext;
    
    if (!exec->argumentCount()) {
        // Constructor for default AudioContext which talks to audio hardware.
        audioContext = AudioContext::create(document);
    } else {
        // Constructor for offline (render-target) AudioContext which renders into an AudioBuffer.
        // new AudioContext(in unsigned long numberOfChannels, in unsigned long numberOfFrames, in float sampleRate);
        if (exec->argumentCount() < 3)
            return throwError(exec, createSyntaxError(exec, "Not enough arguments"));

        unsigned numberOfChannels = exec->argument(0).toInt32(exec);
        unsigned numberOfFrames = exec->argument(1).toInt32(exec);
        float sampleRate = exec->argument(2).toFloat(exec);

        audioContext = AudioContext::createOfflineContext(document, numberOfChannels, numberOfFrames, sampleRate);
    }

    if (!audioContext.get())
        return throwError(exec, createReferenceError(exec, "Error creating AudioContext"));

    return JSValue::encode(asObject(toJS(exec, jsConstructor->globalObject(), audioContext.get())));
}

JSValue JSAudioContext::createBuffer(ExecState* exec)
{
    if (exec->argumentCount() < 2)
        return throwError(exec, createSyntaxError(exec, "Not enough arguments"));

    AudioContext* audioContext = static_cast<AudioContext*>(impl());
    ASSERT(audioContext);

    // AudioBuffer createBuffer(in ArrayBuffer buffer, in boolean mixToMono);
    JSValue val = exec->argument(0);
    if (val.inherits(&JSArrayBuffer::s_info)) {
        ArrayBuffer* arrayBuffer = toArrayBuffer(val);
        ASSERT(arrayBuffer);
        if (arrayBuffer) {
            bool mixToMono = exec->argument(1).toBoolean(exec);

            RefPtr<AudioBuffer> audioBuffer = audioContext->createBuffer(arrayBuffer, mixToMono);
            if (!audioBuffer.get())
                return throwError(exec, createSyntaxError(exec, "Error decoding audio file data"));

            return toJS(exec, globalObject(), audioBuffer.get());
        }

        return jsUndefined();
    }
    
    // AudioBuffer createBuffer(in unsigned long numberOfChannels, in unsigned long numberOfFrames, in float sampleRate);
    if (exec->argumentCount() < 3)
        return throwError(exec, createSyntaxError(exec, "Not enough arguments"));
    
    unsigned numberOfChannels = exec->argument(0).toInt32(exec);
    unsigned numberOfFrames = exec->argument(1).toInt32(exec);
    float sampleRate = exec->argument(2).toFloat(exec);

    RefPtr<AudioBuffer> audioBuffer = audioContext->createBuffer(numberOfChannels, numberOfFrames, sampleRate);
    if (!audioBuffer.get())
        return throwError(exec, createSyntaxError(exec, "Error creating AudioBuffer"));

    return toJS(exec, globalObject(), audioBuffer.get());
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
