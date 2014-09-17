/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "FunctionPrototype.h"

#include "Arguments.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "JSString.h"
#include "JSStringBuilder.h"
#include "Interpreter.h"
#include "Lexer.h"

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(FunctionPrototype);

static EncodedJSValue JSC_HOST_CALL functionProtoFuncToString(ExecState*);
static EncodedJSValue JSC_HOST_CALL functionProtoFuncApply(ExecState*);
static EncodedJSValue JSC_HOST_CALL functionProtoFuncCall(ExecState*);

FunctionPrototype::FunctionPrototype(ExecState* exec, JSGlobalObject* globalObject, Structure* structure)
    : InternalFunction(&exec->globalData(), globalObject, structure, exec->propertyNames().nullIdentifier)
{
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(0), DontDelete | ReadOnly | DontEnum);
}

void FunctionPrototype::addFunctionProperties(ExecState* exec, JSGlobalObject* globalObject, Structure* functionStructure, JSFunction** callFunction, JSFunction** applyFunction)
{
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 0, exec->propertyNames().toString, functionProtoFuncToString), DontEnum);
    *applyFunction = new (exec) JSFunction(exec, globalObject, functionStructure, 2, exec->propertyNames().apply, functionProtoFuncApply);
    putDirectFunctionWithoutTransition(exec, *applyFunction, DontEnum);
    *callFunction = new (exec) JSFunction(exec, globalObject, functionStructure, 1, exec->propertyNames().call, functionProtoFuncCall);
    putDirectFunctionWithoutTransition(exec, *callFunction, DontEnum);
}

static EncodedJSValue JSC_HOST_CALL callFunctionPrototype(ExecState*)
{
    return JSValue::encode(jsUndefined());
}

// ECMA 15.3.4
CallType FunctionPrototype::getCallData(CallData& callData)
{
    callData.native.function = callFunctionPrototype;
    return CallTypeHost;
}

// Functions

// Compatibility hack for the Optimost JavaScript library. (See <rdar://problem/6595040>.)
static inline void insertSemicolonIfNeeded(UString& functionBody)
{
    ASSERT(functionBody[0] == '{');
    ASSERT(functionBody[functionBody.length() - 1] == '}');

    for (size_t i = functionBody.length() - 2; i > 0; --i) {
        UChar ch = functionBody[i];
        if (!Lexer::isWhiteSpace(ch) && !Lexer::isLineTerminator(ch)) {
            if (ch != ';' && ch != '}')
                functionBody = makeUString(functionBody.substringSharingImpl(0, i + 1), ";", functionBody.substringSharingImpl(i + 1, functionBody.length() - (i + 1)));
            return;
        }
    }
}

EncodedJSValue JSC_HOST_CALL functionProtoFuncToString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.inherits(&JSFunction::s_info)) {
        JSFunction* function = asFunction(thisValue);
        if (function->isHostFunction())
            return JSValue::encode(jsMakeNontrivialString(exec, "function ", function->name(exec), "() {\n    [native code]\n}"));
        FunctionExecutable* executable = function->jsExecutable();
        UString sourceString = executable->source().toString();
        insertSemicolonIfNeeded(sourceString);
        return JSValue::encode(jsMakeNontrivialString(exec, "function ", function->name(exec), "(", executable->paramString(), ") ", sourceString));
    }

    if (thisValue.inherits(&InternalFunction::s_info)) {
        InternalFunction* function = asInternalFunction(thisValue);
        return JSValue::encode(jsMakeNontrivialString(exec, "function ", function->name(exec), "() {\n    [native code]\n}"));
    }

    return throwVMTypeError(exec);
}

EncodedJSValue JSC_HOST_CALL functionProtoFuncApply(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    CallData callData;
    CallType callType = getCallData(thisValue, callData);
    if (callType == CallTypeNone)
        return throwVMTypeError(exec);

    JSValue array = exec->argument(1);

    MarkedArgumentBuffer applyArgs;
    if (!array.isUndefinedOrNull()) {
        if (!array.isObject())
            return throwVMTypeError(exec);
        if (asObject(array)->classInfo() == &Arguments::s_info)
            asArguments(array)->fillArgList(exec, applyArgs);
        else if (isJSArray(&exec->globalData(), array))
            asArray(array)->fillArgList(exec, applyArgs);
        else if (asObject(array)->inherits(&JSArray::s_info)) {
            unsigned length = asArray(array)->get(exec, exec->propertyNames().length).toUInt32(exec);
            for (unsigned i = 0; i < length; ++i)
                applyArgs.append(asArray(array)->get(exec, i));
        } else
            return throwVMTypeError(exec);
    }

    return JSValue::encode(call(exec, thisValue, callType, callData, exec->argument(0), applyArgs));
}

EncodedJSValue JSC_HOST_CALL functionProtoFuncCall(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    CallData callData;
    CallType callType = getCallData(thisValue, callData);
    if (callType == CallTypeNone)
        return throwVMTypeError(exec);

    ArgList args(exec);
    ArgList callArgs;
    args.getSlice(1, callArgs);
    return JSValue::encode(call(exec, thisValue, callType, callData, exec->argument(0), callArgs));
}

} // namespace JSC
