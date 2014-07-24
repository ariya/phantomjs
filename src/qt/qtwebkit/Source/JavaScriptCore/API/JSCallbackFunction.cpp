/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSCallbackFunction.h"

#include "APIShims.h"
#include "APICast.h"
#include "CodeBlock.h"
#include "Error.h"
#include "ExceptionHelpers.h"
#include "FunctionPrototype.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "JSLock.h"
#include "Operations.h"
#include <wtf/Vector.h>

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(JSCallbackFunction);

const ClassInfo JSCallbackFunction::s_info = { "CallbackFunction", &InternalFunction::s_info, 0, 0, CREATE_METHOD_TABLE(JSCallbackFunction) };

JSCallbackFunction::JSCallbackFunction(JSGlobalObject* globalObject, Structure* structure, JSObjectCallAsFunctionCallback callback)
    : InternalFunction(globalObject, structure)
    , m_callback(callback)
{
}

void JSCallbackFunction::finishCreation(VM& vm, const String& name)
{
    Base::finishCreation(vm, name);
    ASSERT(inherits(&s_info));
}

JSCallbackFunction* JSCallbackFunction::create(ExecState* exec, JSGlobalObject* globalObject, JSObjectCallAsFunctionCallback callback, const String& name)
{
    JSCallbackFunction* function = new (NotNull, allocateCell<JSCallbackFunction>(*exec->heap())) JSCallbackFunction(globalObject, globalObject->callbackFunctionStructure(), callback);
    function->finishCreation(exec->vm(), name);
    return function;
}

EncodedJSValue JSCallbackFunction::call(ExecState* exec)
{
    JSContextRef execRef = toRef(exec);
    JSObjectRef functionRef = toRef(exec->callee());
    JSObjectRef thisObjRef = toRef(exec->hostThisValue().toThisObject(exec));

    size_t argumentCount = exec->argumentCount();
    Vector<JSValueRef, 16> arguments;
    arguments.reserveInitialCapacity(argumentCount);
    for (size_t i = 0; i < argumentCount; ++i)
        arguments.uncheckedAppend(toRef(exec, exec->argument(i)));

    JSValueRef exception = 0;
    JSValueRef result;
    {
        APICallbackShim callbackShim(exec);
        result = jsCast<JSCallbackFunction*>(toJS(functionRef))->m_callback(execRef, functionRef, thisObjRef, argumentCount, arguments.data(), &exception);
    }
    if (exception)
        throwError(exec, toJS(exec, exception));

    // result must be a valid JSValue.
    if (!result)
        return JSValue::encode(jsUndefined());

    return JSValue::encode(toJS(exec, result));
}

CallType JSCallbackFunction::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = call;
    return CallTypeHost;
}

} // namespace JSC
