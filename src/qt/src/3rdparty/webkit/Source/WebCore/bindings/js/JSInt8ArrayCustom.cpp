/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
#include "JSInt8Array.h"

#include "Int8Array.h"
#include "JSArrayBufferViewHelper.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

void JSInt8Array::indexSetter(JSC::ExecState* exec, unsigned index, JSC::JSValue value)
{
    impl()->set(index, static_cast<signed char>(value.toInt32(exec)));
}

JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, Int8Array* object)
{
    return toJSArrayBufferView<JSInt8Array>(exec, globalObject, object);
}

JSC::JSValue JSInt8Array::set(JSC::ExecState* exec)
{
    return setWebGLArrayHelper(exec, impl(), toInt8Array);
}

EncodedJSValue JSC_HOST_CALL JSInt8ArrayConstructor::constructJSInt8Array(ExecState* exec)
{
    JSInt8ArrayConstructor* jsConstructor = static_cast<JSInt8ArrayConstructor*>(exec->callee());
    RefPtr<Int8Array> array = constructArrayBufferView<Int8Array, signed char>(exec);
    if (!array.get())
        // Exception has already been thrown.
        return JSValue::encode(JSValue());
    return JSValue::encode(asObject(toJS(exec, jsConstructor->globalObject(), array.get())));
}

} // namespace WebCore
