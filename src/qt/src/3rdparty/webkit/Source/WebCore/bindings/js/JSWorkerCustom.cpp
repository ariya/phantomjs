/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(WORKERS)

#include "JSWorker.h"

#include "JSDOMGlobalObject.h"
#include "JSMessagePortCustom.h"
#include "Worker.h"
#include "JSDOMWindowCustom.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

JSC::JSValue JSWorker::postMessage(JSC::ExecState* exec)
{
    return handlePostMessage(exec, impl());
}

EncodedJSValue JSC_HOST_CALL JSWorkerConstructor::constructJSWorker(ExecState* exec)
{
    JSWorkerConstructor* jsConstructor = static_cast<JSWorkerConstructor*>(exec->callee());

    if (!exec->argumentCount())
        return throwVMError(exec, createSyntaxError(exec, "Not enough arguments"));

    UString scriptURL = exec->argument(0).toString(exec);
    if (exec->hadException())
        return JSValue::encode(JSValue());

    // See section 4.8.2 step 14 of WebWorkers for why this is the lexicalGlobalObject. 
    DOMWindow* window = asJSDOMWindow(exec->lexicalGlobalObject())->impl();

    ExceptionCode ec = 0;
    RefPtr<Worker> worker = Worker::create(ustringToString(scriptURL), window->document(), ec);
    if (ec) {
        setDOMException(exec, ec);
        return JSValue::encode(JSValue());
    }

    return JSValue::encode(asObject(toJS(exec, jsConstructor->globalObject(), worker.release())));
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
