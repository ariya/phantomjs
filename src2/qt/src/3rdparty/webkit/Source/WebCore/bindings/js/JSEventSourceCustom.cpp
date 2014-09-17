/*
 * Copyright (C) 2009 Ericsson AB. All rights reserved.
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Ericsson nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#if ENABLE(EVENTSOURCE)

#include "JSEventSource.h"

#include "EventSource.h"
#include "ExceptionCode.h"
#include "ScriptExecutionContext.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

EncodedJSValue JSC_HOST_CALL JSEventSourceConstructor::constructJSEventSource(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return throwVMError(exec, createSyntaxError(exec, "Not enough arguments"));

    UString url = exec->argument(0).toString(exec);
    if (exec->hadException())
        return JSValue::encode(JSValue());

    JSEventSourceConstructor* jsConstructor =  static_cast<JSEventSourceConstructor*>(exec->callee());
    ScriptExecutionContext* context = jsConstructor->scriptExecutionContext();
    if (!context)
        return throwVMError(exec, createReferenceError(exec, "EventSource constructor associated document is unavailable"));

    ExceptionCode ec = 0;
    RefPtr<EventSource> eventSource = EventSource::create(ustringToString(url), context, ec);
    if (ec) {
        setDOMException(exec, ec);
        return JSValue::encode(JSValue());
    }

    return JSValue::encode(asObject(toJS(exec, jsConstructor->globalObject(), eventSource.release())));
}

} // namespace WebCore

#endif // ENABLE(EVENTSOURCE)
