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
#include "JSUint8Array.h"

#include "JSArrayBufferViewHelper.h"
#include "Uint8Array.h"

using namespace JSC;

namespace WebCore {

void JSUint8Array::indexSetter(JSC::ExecState* exec, unsigned index, JSC::JSValue value)
{
    impl()->set(index, static_cast<unsigned char>(value.toInt32(exec)));
}

JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, Uint8Array* object)
{
    return toJSArrayBufferView<JSUint8Array>(exec, globalObject, object);
}

JSC::JSValue JSUint8Array::set(JSC::ExecState* exec)
{
    return setWebGLArrayHelper(exec, impl(), toUint8Array);
}

EncodedJSValue JSC_HOST_CALL JSUint8ArrayConstructor::constructJSUint8Array(ExecState* exec)
{
    JSUint8ArrayConstructor* jsConstructor = static_cast<JSUint8ArrayConstructor*>(exec->callee());
    RefPtr<Uint8Array> array = constructArrayBufferView<Uint8Array, unsigned char>(exec);
    if (!array.get())
        // Exception has already been thrown.
        return JSValue::encode(JSValue());
    return JSValue::encode(asObject(toJS(exec, jsConstructor->globalObject(), array.get())));
}

} // namespace WebCore
