/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "DFGOperations.h"

#if ENABLE(DFG_JIT)

#include "CodeBlock.h"
#include "Interpreter.h"
#include "JSByteArray.h"
#include "JSGlobalData.h"
#include "Operations.h"

namespace JSC { namespace DFG {

EncodedJSValue operationConvertThis(ExecState* exec, EncodedJSValue encodedOp)
{
    return JSValue::encode(JSValue::decode(encodedOp).toThisObject(exec));
}

EncodedJSValue operationValueAdd(ExecState* exec, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2)
{
    JSValue op1 = JSValue::decode(encodedOp1);
    JSValue op2 = JSValue::decode(encodedOp2);

    if (op1.isInt32() && op2.isInt32()) {
        int64_t result64 = static_cast<int64_t>(op1.asInt32()) + static_cast<int64_t>(op2.asInt32());
        int32_t result32 = static_cast<int32_t>(result64);
        if (LIKELY(result32 == result64))
            return JSValue::encode(jsNumber(result32));
        return JSValue::encode(jsNumber((double)result64));
    }
    
    double number1;
    double number2;
    if (op1.getNumber(number1) && op2.getNumber(number2))
        return JSValue::encode(jsNumber(number1 + number2));

    return JSValue::encode(jsAddSlowCase(exec, op1, op2));
}

EncodedJSValue operationGetByVal(ExecState* exec, EncodedJSValue encodedBase, EncodedJSValue encodedProperty)
{
    JSValue baseValue = JSValue::decode(encodedBase);
    JSValue property = JSValue::decode(encodedProperty);

    if (LIKELY(baseValue.isCell())) {
        JSCell* base = baseValue.asCell();

        if (property.isUInt32()) {
            JSGlobalData* globalData = &exec->globalData();
            uint32_t i = property.asUInt32();

            // FIXME: the JIT used to handle these in compiled code!
            if (isJSArray(globalData, base) && asArray(base)->canGetIndex(i))
                return JSValue::encode(asArray(base)->getIndex(i));

            // FIXME: the JITstub used to relink this to an optimized form!
            if (isJSString(globalData, base) && asString(base)->canGetIndex(i))
                return JSValue::encode(asString(base)->getIndex(exec, i));

            // FIXME: the JITstub used to relink this to an optimized form!
            if (isJSByteArray(globalData, base) && asByteArray(base)->canAccessIndex(i))
                return JSValue::encode(asByteArray(base)->getIndex(exec, i));

            return JSValue::encode(baseValue.get(exec, i));
        }

        if (property.isString()) {
            Identifier propertyName(exec, asString(property)->value(exec));
            PropertySlot slot(base);
            if (base->fastGetOwnPropertySlot(exec, propertyName, slot))
                return JSValue::encode(slot.getValue(exec, propertyName));
        }
    }

    Identifier ident(exec, property.toString(exec));
    return JSValue::encode(baseValue.get(exec, ident));
}

EncodedJSValue operationGetById(ExecState* exec, EncodedJSValue encodedBase, Identifier* identifier)
{
    JSValue baseValue = JSValue::decode(encodedBase);
    PropertySlot slot(baseValue);
    return JSValue::encode(baseValue.get(exec, *identifier, slot));
}

template<bool strict>
ALWAYS_INLINE static void operationPutByValInternal(ExecState* exec, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue)
{
    JSGlobalData* globalData = &exec->globalData();

    JSValue baseValue = JSValue::decode(encodedBase);
    JSValue property = JSValue::decode(encodedProperty);
    JSValue value = JSValue::decode(encodedValue);

    if (LIKELY(property.isUInt32())) {
        uint32_t i = property.asUInt32();

        if (isJSArray(globalData, baseValue)) {
            JSArray* jsArray = asArray(baseValue);
            if (jsArray->canSetIndex(i)) {
                jsArray->setIndex(*globalData, i, value);
                return;
            }

            jsArray->JSArray::put(exec, i, value);
            return;
        }

        if (isJSByteArray(globalData, baseValue) && asByteArray(baseValue)->canAccessIndex(i)) {
            JSByteArray* jsByteArray = asByteArray(baseValue);
            // FIXME: the JITstub used to relink this to an optimized form!
            if (value.isInt32()) {
                jsByteArray->setIndex(i, value.asInt32());
                return;
            }

            double dValue = 0;
            if (value.getNumber(dValue)) {
                jsByteArray->setIndex(i, dValue);
                return;
            }
        }

        baseValue.put(exec, i, value);
        return;
    }

    // Don't put to an object if toString throws an exception.
    Identifier ident(exec, property.toString(exec));
    if (!globalData->exception) {
        PutPropertySlot slot(strict);
        baseValue.put(exec, ident, value, slot);
    }
}

void operationPutByValStrict(ExecState* exec, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue)
{
    operationPutByValInternal<true>(exec, encodedBase, encodedProperty, encodedValue);
}

void operationPutByValNonStrict(ExecState* exec, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue)
{
    operationPutByValInternal<false>(exec, encodedBase, encodedProperty, encodedValue);
}

void operationPutByIdStrict(ExecState* exec, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier* identifier)
{
    PutPropertySlot slot(true);
    JSValue::decode(encodedBase).put(exec, *identifier, JSValue::decode(encodedValue), slot);
}

void operationPutByIdNonStrict(ExecState* exec, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier* identifier)
{
    PutPropertySlot slot(false);
    JSValue::decode(encodedBase).put(exec, *identifier, JSValue::decode(encodedValue), slot);
}

void operationPutByIdDirectStrict(ExecState* exec, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier* identifier)
{
    PutPropertySlot slot(true);
    JSValue::decode(encodedBase).putDirect(exec, *identifier, JSValue::decode(encodedValue), slot);
}

void operationPutByIdDirectNonStrict(ExecState* exec, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier* identifier)
{
    PutPropertySlot slot(false);
    JSValue::decode(encodedBase).putDirect(exec, *identifier, JSValue::decode(encodedValue), slot);
}

bool operationCompareLess(ExecState* exec, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2)
{
    return jsLess(exec, JSValue::decode(encodedOp1), JSValue::decode(encodedOp2));
}

bool operationCompareLessEq(ExecState* exec, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2)
{
    return jsLessEq(exec, JSValue::decode(encodedOp1), JSValue::decode(encodedOp2));
}

bool operationCompareEq(ExecState* exec, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2)
{
    return JSValue::equal(exec, JSValue::decode(encodedOp1), JSValue::decode(encodedOp2));
}

bool operationCompareStrictEq(ExecState* exec, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2)
{
    return JSValue::strictEqual(exec, JSValue::decode(encodedOp1), JSValue::decode(encodedOp2));
}

DFGHandler lookupExceptionHandler(ExecState* exec, ReturnAddressPtr faultLocation)
{
    JSValue exceptionValue = exec->exception();
    ASSERT(exceptionValue);

    unsigned vPCIndex = exec->codeBlock()->bytecodeOffset(faultLocation);
    HandlerInfo* handler = exec->globalData().interpreter->throwException(exec, exceptionValue, vPCIndex);

    void* catchRoutine = handler ? handler->nativeCode.executableAddress() : (void*)ctiOpThrowNotCaught;
    ASSERT(catchRoutine);
    return DFGHandler(exec, catchRoutine);
}

double dfgConvertJSValueToNumber(ExecState* exec, EncodedJSValue value)
{
    return JSValue::decode(value).toNumber(exec);
}

int32_t dfgConvertJSValueToInt32(ExecState* exec, EncodedJSValue value)
{
    return JSValue::decode(value).toInt32(exec);
}

bool dfgConvertJSValueToBoolean(ExecState* exec, EncodedJSValue encodedOp)
{
    return JSValue::decode(encodedOp).toBoolean(exec);
}

} } // namespace JSC::DFG

#endif
