/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSDataView.h"

#include "DataView.h"
#include "ExceptionCode.h"
#include "JSArrayBufferViewHelper.h"
#include <wtf/MathExtras.h>

using namespace JSC;

namespace WebCore {

enum DataViewAccessType {
    AccessDataViewMemberAsFloat32,
    AccessDataViewMemberAsFloat64
};

JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, DataView* object)
{
    return wrap<JSDataView>(exec, globalObject, object);
}

EncodedJSValue JSC_HOST_CALL JSDataViewConstructor::constructJSDataView(ExecState* exec)
{
    if (exec->argument(0).isNull() || !exec->argument(0).isObject())
        return throwVMTypeError(exec);

    RefPtr<DataView> view = constructArrayBufferViewWithArrayBufferArgument<DataView, char>(exec);
    if (!view.get()) {
        setDOMException(exec, INDEX_SIZE_ERR);
        return JSValue::encode(jsUndefined());
    }

    JSDataViewConstructor* jsConstructor = jsCast<JSDataViewConstructor*>(exec->callee());
    return JSValue::encode(asObject(toJS(exec, jsConstructor->globalObject(), view.get())));
}

static JSValue getDataViewMember(ExecState* exec, DataView* imp, DataViewAccessType type)
{
    if (exec->argumentCount() < 1)
        return throwError(exec, createNotEnoughArgumentsError(exec));
    ExceptionCode ec = 0;
    unsigned byteOffset = exec->argument(0).toUInt32(exec);
    if (exec->hadException())
        return jsUndefined();

    bool littleEndian = false;
    if (exec->argumentCount() > 1 && (type == AccessDataViewMemberAsFloat32 || type == AccessDataViewMemberAsFloat64)) {
        littleEndian = exec->argument(1).toBoolean(exec);
        if (exec->hadException())
            return jsUndefined();
    }

    JSC::JSValue result;
    switch (type) {
    case AccessDataViewMemberAsFloat32:
    case AccessDataViewMemberAsFloat64: {
        double value = (type == AccessDataViewMemberAsFloat32) ? imp->getFloat32(byteOffset, littleEndian, ec) : imp->getFloat64(byteOffset, littleEndian, ec);
        result = std::isnan(value) ? jsNaN() : jsNumber(value);
        break;
    } default:
        ASSERT_NOT_REACHED();
        break;
    }
    setDOMException(exec, ec);
    return result;
}

JSValue JSDataView::getFloat32(ExecState* exec)
{
    return getDataViewMember(exec, static_cast<DataView*>(impl()), AccessDataViewMemberAsFloat32);
}

JSValue JSDataView::getFloat64(ExecState* exec)
{
    return getDataViewMember(exec, static_cast<DataView*>(impl()), AccessDataViewMemberAsFloat64);
}

} // namespace WebCore
