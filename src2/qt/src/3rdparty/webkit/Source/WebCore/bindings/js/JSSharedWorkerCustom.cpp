/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(SHARED_WORKERS)

#include "JSSharedWorker.h"

#include "JSDOMGlobalObject.h"
#include "JSDOMWindowCustom.h"
#include "SharedWorker.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

void JSSharedWorker::visitChildren(SlotVisitor& visitor)
{
    Base::visitChildren(visitor);

    if (MessagePort* port = impl()->port())
        visitor.addOpaqueRoot(port);
}

EncodedJSValue JSC_HOST_CALL JSSharedWorkerConstructor::constructJSSharedWorker(ExecState* exec)
{
    JSSharedWorkerConstructor* jsConstructor = static_cast<JSSharedWorkerConstructor*>(exec->callee());

    if (exec->argumentCount() < 1)
        return throwVMError(exec, createSyntaxError(exec, "Not enough arguments"));

    UString scriptURL = exec->argument(0).toString(exec);
    UString name;
    if (exec->argumentCount() > 1)
        name = exec->argument(1).toString(exec);

    if (exec->hadException())
        return JSValue::encode(JSValue());

    // FIXME: We need to use both the dynamic scope and the lexical scope (dynamic scope for resolving the worker URL)
    DOMWindow* window = asJSDOMWindow(exec->lexicalGlobalObject())->impl();
    ExceptionCode ec = 0;
    RefPtr<SharedWorker> worker = SharedWorker::create(ustringToString(scriptURL), ustringToString(name), window->document(), ec);
    if (ec) {
        setDOMException(exec, ec);
        return JSValue::encode(JSValue());
    }

    return JSValue::encode(asObject(toJS(exec, jsConstructor->globalObject(), worker.release())));
}

} // namespace WebCore

#endif // ENABLE(SHARED_WORKERS)
