/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "JSMutationObserver.h"

#include "ExceptionCode.h"
#include "JSMutationCallback.h"
#include "JSNodeCustom.h"
#include "MutationObserver.h"
#include <runtime/Error.h>
#include <runtime/PrivateName.h>

using namespace JSC;

namespace WebCore {

EncodedJSValue JSC_HOST_CALL JSMutationObserverConstructor::constructJSMutationObserver(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return throwVMError(exec, createNotEnoughArgumentsError(exec));

    JSObject* object = exec->argument(0).getObject();
    CallData callData;
    if (!object || object->methodTable()->getCallData(object, callData) == CallTypeNone)
        return throwVMError(exec, createTypeError(exec, "Callback argument must be a function"));

    JSMutationObserverConstructor* jsConstructor = jsCast<JSMutationObserverConstructor*>(exec->callee());
    RefPtr<JSMutationCallback> callback = JSMutationCallback::create(object, jsConstructor->globalObject());
    JSObject* jsObserver = asObject(toJS(exec, jsConstructor->globalObject(), MutationObserver::create(callback.release())));
    PrivateName propertyName;
    jsObserver->putDirect(jsConstructor->globalObject()->vm(), propertyName, object);
    return JSValue::encode(jsObserver);
}

bool JSMutationObserverOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor)
{
    MutationObserver* observer = jsCast<JSMutationObserver*>(handle.get().asCell())->impl();
    HashSet<Node*> observedNodes = observer->getObservedNodes();
    for (HashSet<Node*>::iterator it = observedNodes.begin(); it != observedNodes.end(); ++it) {
        if (visitor.containsOpaqueRoot(root(*it)))
            return true;
    }
    return false;
}

} // namespace WebCore
