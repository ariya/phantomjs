/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "JSCallbackConstructor.h"

#include "APIShims.h"
#include "APICast.h"
#include <runtime/Error.h>
#include <runtime/JSGlobalObject.h>
#include <runtime/JSLock.h>
#include <runtime/ObjectPrototype.h>
#include <wtf/Vector.h>

namespace JSC {

const ClassInfo JSCallbackConstructor::s_info = { "CallbackConstructor", &JSObjectWithGlobalObject::s_info, 0, 0 };

JSCallbackConstructor::JSCallbackConstructor(JSGlobalObject* globalObject, Structure* structure, JSClassRef jsClass, JSObjectCallAsConstructorCallback callback)
    : JSObjectWithGlobalObject(globalObject, structure)
    , m_class(jsClass)
    , m_callback(callback)
{
    ASSERT(inherits(&s_info));
    if (m_class)
        JSClassRetain(jsClass);
}

JSCallbackConstructor::~JSCallbackConstructor()
{
    if (m_class)
        JSClassRelease(m_class);
}

static EncodedJSValue JSC_HOST_CALL constructJSCallback(ExecState* exec)
{
    JSObject* constructor = exec->callee();
    JSContextRef ctx = toRef(exec);
    JSObjectRef constructorRef = toRef(constructor);

    JSObjectCallAsConstructorCallback callback = static_cast<JSCallbackConstructor*>(constructor)->callback();
    if (callback) {
        int argumentCount = static_cast<int>(exec->argumentCount());
        Vector<JSValueRef, 16> arguments(argumentCount);
        for (int i = 0; i < argumentCount; i++)
            arguments[i] = toRef(exec, exec->argument(i));

        JSValueRef exception = 0;
        JSObjectRef result;
        {
            APICallbackShim callbackShim(exec);
            result = callback(ctx, constructorRef, argumentCount, arguments.data(), &exception);
        }
        if (exception)
            throwError(exec, toJS(exec, exception));
        return JSValue::encode(toJS(result));
    }
    
    return JSValue::encode(toJS(JSObjectMake(ctx, static_cast<JSCallbackConstructor*>(constructor)->classRef(), 0)));
}

ConstructType JSCallbackConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructJSCallback;
    return ConstructTypeHost;
}

} // namespace JSC
