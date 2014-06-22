/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef JSArrayBufferViewHelper_h
#define JSArrayBufferViewHelper_h

#include "JSArrayBuffer.h"
#include "JSArrayBufferView.h"
#include "JSDOMBinding.h"
#include <interpreter/CallFrame.h>
#include <runtime/ArgList.h>
#include <runtime/Error.h>
#include <runtime/JSCJSValue.h>
#include <runtime/JSObject.h>
#include <runtime/Operations.h>
#include <wtf/ArrayBufferView.h>
#include <wtf/TypedArrayBase.h>

namespace WebCore {

static const char* tooLargeSize = "Size is too large (or is negative).";

template<class C, typename T>
bool copyTypedArrayBuffer(C* target, ArrayBufferView* source, unsigned sourceLength, unsigned offset)
{
    ArrayBufferView::ViewType sourceType = source->getType();
    ASSERT(sourceType != ArrayBufferView::TypeDataView);

    if (target->getType() == sourceType) {
        if (!target->set(static_cast<C*>(source), offset))
            return false;
        return true;
    }

    if (!target->checkInboundData(offset, sourceLength))
        return false;

    switch (sourceType) {
    case ArrayBufferView::TypeInt8:
        for (unsigned i = 0; i < sourceLength; ++i)
            target->set(i + offset, (T)(static_cast<TypedArrayBase<signed char> *>(source)->item(i)));
        break;
    case ArrayBufferView::TypeUint8:
    case ArrayBufferView::TypeUint8Clamped:
        for (unsigned i = 0; i < sourceLength; ++i)
            target->set(i + offset, (T)(static_cast<TypedArrayBase<unsigned char> *>(source)->item(i)));
        break;
    case ArrayBufferView::TypeInt16:
        for (unsigned i = 0; i < sourceLength; ++i)
            target->set(i + offset, (T)(static_cast<TypedArrayBase<signed short> *>(source)->item(i)));
        break;
    case ArrayBufferView::TypeUint16:
        for (unsigned i = 0; i < sourceLength; ++i)
            target->set(i + offset, (T)(static_cast<TypedArrayBase<unsigned short> *>(source)->item(i)));
        break;
    case ArrayBufferView::TypeInt32:
        for (unsigned i = 0; i < sourceLength; ++i)
            target->set(i + offset, (T)(static_cast<TypedArrayBase<int> *>(source)->item(i)));
        break;
    case ArrayBufferView::TypeUint32:
        for (unsigned i = 0; i < sourceLength; ++i)
            target->set(i + offset, (T)(static_cast<TypedArrayBase<unsigned int> *>(source)->item(i)));
        break;
    case ArrayBufferView::TypeFloat32:
        for (unsigned i = 0; i < sourceLength; ++i)
            target->set(i + offset, (T)(static_cast<TypedArrayBase<float> *>(source)->item(i)));
        break;
    case ArrayBufferView::TypeFloat64:
        for (unsigned i = 0; i < sourceLength; ++i)
            target->set(i + offset, (T)(static_cast<TypedArrayBase<double> *>(source)->item(i)));
        break;
    default:
        break;
    }

    return true;
}

template <class C, typename T>
bool setWebGLArrayWithTypedArrayArgument(JSC::ExecState* exec, C* impl)
{
    RefPtr<ArrayBufferView> array = toArrayBufferView(exec->argument(0));
    if (!array)
        return false;

    ArrayBufferView::ViewType arrayType = array->getType();
    if (arrayType == ArrayBufferView::TypeDataView)
        return false;

    unsigned offset = 0;
    if (exec->argumentCount() == 2)
        offset = exec->argument(1).toInt32(exec);

    uint32_t length = asObject(exec->argument(0))->get(exec, exec->vm().propertyNames->length).toUInt32(exec);

    if (!(copyTypedArrayBuffer<C, T>(impl, array.get(), length, offset)))
        throwError(exec, createRangeError(exec, "Index is out of range."));

    return true;
}

template<class C, typename T>
JSC::JSValue setWebGLArrayHelper(JSC::ExecState* exec, C* impl)
{
    if (exec->argumentCount() < 1)
        return JSC::throwError(exec, createNotEnoughArgumentsError(exec));

    if (setWebGLArrayWithTypedArrayArgument<C, T>(exec, impl))
        // void set(in WebGL<>Array array, [Optional] in unsigned long offset);
        return JSC::jsUndefined();

    if (exec->argument(0).isObject()) {
        // void set(in sequence<long> array, [Optional] in unsigned long offset);
        JSC::JSObject* array = JSC::asObject(exec->argument(0));
        uint32_t offset = 0;
        if (exec->argumentCount() == 2)
            offset = exec->argument(1).toInt32(exec);
        uint32_t length = array->get(exec, exec->vm().propertyNames->length).toInt32(exec);
        if (!impl->checkInboundData(offset, length))
            throwError(exec, createRangeError(exec, "Index is out of range."));
        else {
            for (uint32_t i = 0; i < length; i++) {
                JSC::JSValue v = array->get(exec, i);
                if (exec->hadException())
                    return JSC::jsUndefined();
                impl->set(i + offset, v.toNumber(exec));
            }
        }

        return JSC::jsUndefined();
    }

    return JSC::throwTypeError(exec, "Invalid argument");
}

// Template function used by XXXArrayConstructors.
// If this returns 0, it will already have thrown a JavaScript exception.
template<class C, typename T>
PassRefPtr<C> constructArrayBufferViewWithTypedArrayArgument(JSC::ExecState* exec)
{
    RefPtr<ArrayBufferView> source = toArrayBufferView(exec->argument(0));
    if (!source)
        return 0;

    ArrayBufferView::ViewType sourceType = source->getType();
    if (sourceType == ArrayBufferView::TypeDataView)
        return 0;

    uint32_t length = asObject(exec->argument(0))->get(exec, exec->vm().propertyNames->length).toUInt32(exec);
    RefPtr<C> array = C::createUninitialized(length);
    if (!array) {
        throwError(exec, createRangeError(exec, tooLargeSize));
        return array;
    }

    if (!(copyTypedArrayBuffer<C, T>(array.get(), source.get(), length, 0))) {
        throwError(exec, createRangeError(exec, tooLargeSize));
        return array;
    }

    return array;
}

// Template function used by XXXArrayConstructors.
// If this returns 0, it will already have thrown a JavaScript exception.
template<class C, typename T>
PassRefPtr<C> constructArrayBufferViewWithArrayBufferArgument(JSC::ExecState* exec)
{
    RefPtr<ArrayBuffer> buffer = toArrayBuffer(exec->argument(0));
    if (!buffer)
        return 0;

    unsigned offset = (exec->argumentCount() > 1) ? exec->argument(1).toUInt32(exec) : 0;
    unsigned int length = 0;
    if (exec->argumentCount() > 2)
        length = exec->argument(2).toUInt32(exec);
    else {
        if ((buffer->byteLength() - offset) % sizeof(T)) {
            throwError(exec, createRangeError(exec, "ArrayBuffer length minus the byteOffset is not a multiple of the element size."));
            return 0;
        }
        length = (buffer->byteLength() - offset) / sizeof(T);
    }
    RefPtr<C> array = C::create(buffer, offset, length);
    if (!array)
        throwError(exec, createRangeError(exec, tooLargeSize));
    return array;
}

template<class C, typename T>
PassRefPtr<C> constructArrayBufferView(JSC::ExecState* exec)
{
    // There are 3 constructors:
    //
    //  1) (in int size)
    //  2) (in ArrayBuffer buffer, [Optional] in int offset, [Optional] in unsigned int length)
    //  3) (in sequence<T>) - This ends up being a JS "array-like" object
    //    
    // For the 0 args case, just create a zero-length view. We could
    // consider raising a SyntaxError for this case, but not all
    // JavaScript DOM bindings can distinguish between "new
    // <Type>Array()" and what occurs when a previously-constructed
    // ArrayBufferView is returned to JavaScript; e.g., from
    // "array.subset()".
    if (exec->argumentCount() < 1)
        return C::create(0);
    
    if (exec->argument(0).isNull()) {
        // Invalid first argument
        throwTypeError(exec);
        return 0;
    }

    if (exec->argument(0).isObject()) {
        RefPtr<C> view = constructArrayBufferViewWithArrayBufferArgument<C, T>(exec);
        if (view)
            return view;
    
        view = constructArrayBufferViewWithTypedArrayArgument<C, T>(exec);
        if (view)
            return view;

        JSC::JSObject* srcArray = asObject(exec->argument(0));
        uint32_t length = srcArray->get(exec, exec->vm().propertyNames->length).toUInt32(exec);
        RefPtr<C> array = C::createUninitialized(length);
        if (!array) {
            throwError(exec, createRangeError(exec, tooLargeSize));
            return array;
        }

        for (unsigned i = 0; i < length; ++i) {
            JSC::JSValue v = srcArray->get(exec, i);
            array->set(i, v.toNumber(exec));
        }
        return array;
    }

    int length = exec->argument(0).toInt32(exec);
    RefPtr<C> result;
    if (length >= 0)
        result = C::create(static_cast<unsigned>(length));
    if (!result)
        throwError(exec, createRangeError(exec, tooLargeSize));
    return result;
}

template <typename JSType, typename WebCoreType>
static JSC::JSValue toJSArrayBufferView(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, WebCoreType* object)
{
    if (!object)
        return JSC::jsNull();

    if (JSDOMWrapper* wrapper = getCachedWrapper(currentWorld(exec), object))
        return wrapper;

    exec->heap()->reportExtraMemoryCost(object->byteLength());
    return createWrapper<JSType>(exec, globalObject, object);
}

} // namespace WebCore

#endif // JSArrayBufferViewHelper_h
