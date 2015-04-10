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
#include "JSArrayBuffer.h"

#include "ExceptionCode.h"
#include <runtime/Error.h>
#include <wtf/ArrayBuffer.h>

namespace WebCore {

using namespace JSC;

EncodedJSValue JSC_HOST_CALL JSArrayBufferConstructor::constructJSArrayBuffer(ExecState* exec)
{
    JSArrayBufferConstructor* jsConstructor = jsCast<JSArrayBufferConstructor*>(exec->callee());

    int length = 0;
    if (exec->argumentCount() > 0)
        length = exec->argument(0).toInt32(exec); // NaN/+inf/-inf returns 0, this is intended by WebIDL
    RefPtr<ArrayBuffer> buffer;
    if (length >= 0)
        buffer = ArrayBuffer::create(static_cast<unsigned>(length), 1);
    if (!buffer.get())
        return throwVMError(exec, createRangeError(exec, "ArrayBuffer size is not a small enough positive integer."));
    return JSValue::encode(asObject(toJS(exec, jsConstructor->globalObject(), buffer.get())));
}

} // namespace WebCore
