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

#include "AudioBuffer.h"
#include "JSArrayBuffer.h"
#include "JSAudioBuffer.h"
#include "JSAudioContext.h"
#include "JSOfflineAudioContext.h"
#include "OfflineAudioContext.h"
#include <runtime/Error.h>
#include <wtf/ArrayBuffer.h>

using namespace JSC;

namespace WebCore {

EncodedJSValue JSC_HOST_CALL JSAudioContextConstructor::constructJSAudioContext(ExecState* exec)
{
    JSAudioContextConstructor* jsConstructor = jsCast<JSAudioContextConstructor*>(exec->callee());
    if (!jsConstructor)
        return throwVMError(exec, createReferenceError(exec, "AudioContext constructor callee is unavailable"));

    ScriptExecutionContext* scriptExecutionContext = jsConstructor->scriptExecutionContext();
    if (!scriptExecutionContext)
        return throwVMError(exec, createReferenceError(exec, "AudioContext constructor script execution context is unavailable"));
        
    if (!scriptExecutionContext->isDocument())
        return throwVMError(exec, createReferenceError(exec, "AudioContext constructor called in a script execution context which is not a document"));

    Document* document = toDocument(scriptExecutionContext);

    RefPtr<AudioContext> audioContext;
    
    if (!exec->argumentCount()) {
        // Constructor for default AudioContext which talks to audio hardware.
        ExceptionCode ec = 0;
        audioContext = AudioContext::create(document, ec);
        if (ec) {
            setDOMException(exec, ec);
            return JSValue::encode(JSValue());
        }
        if (!audioContext.get())
            return throwVMError(exec, createSyntaxError(exec, "audio resources unavailable for AudioContext construction"));
    } else {
#if ENABLE(LEGACY_WEB_AUDIO)
        // Constructor for offline (render-target) AudioContext which renders into an AudioBuffer.
        // new AudioContext(in unsigned long numberOfChannels, in unsigned long numberOfFrames, in float sampleRate);
        document->addConsoleMessage(JSMessageSource, WarningMessageLevel,
            "Deprecated AudioContext constructor: use OfflineAudioContext instead");

        if (exec->argumentCount() < 3)
            return throwVMError(exec, createNotEnoughArgumentsError(exec));

        int32_t numberOfChannels = exec->argument(0).toInt32(exec);
        int32_t numberOfFrames = exec->argument(1).toInt32(exec);
        float sampleRate = exec->argument(2).toFloat(exec);
        
        if (numberOfChannels <= 0 || numberOfChannels > 10)
            return throwVMError(exec, createSyntaxError(exec, "Invalid number of channels"));

        if (numberOfFrames <= 0)
            return throwVMError(exec, createSyntaxError(exec, "Invalid number of frames"));

        if (sampleRate <= 0)
            return throwVMError(exec, createSyntaxError(exec, "Invalid sample rate"));


        ExceptionCode ec = 0;
        audioContext = OfflineAudioContext::create(document, numberOfChannels, numberOfFrames, sampleRate, ec);
        if (ec) {
            setDOMException(exec, ec);
            return throwVMError(exec, createSyntaxError(exec, "Error creating OfflineAudioContext"));
        }
#else
        return throwVMError(exec, createSyntaxError(exec, "Illegal AudioContext constructor"));
#endif
    }

    if (!audioContext.get())
        return throwVMError(exec, createReferenceError(exec, "Error creating AudioContext"));

    return JSValue::encode(CREATE_DOM_WRAPPER(exec, jsConstructor->globalObject(), AudioContext, audioContext.get()));
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
