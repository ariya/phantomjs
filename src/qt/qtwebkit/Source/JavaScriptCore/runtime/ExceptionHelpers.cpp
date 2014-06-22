/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#include "ExceptionHelpers.h"

#include "CodeBlock.h"
#include "CallFrame.h"
#include "ErrorInstance.h"
#include "JSGlobalObjectFunctions.h"
#include "JSObject.h"
#include "JSNotAnObject.h"
#include "Interpreter.h"
#include "Nodes.h"
#include "Operations.h"

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(TerminatedExecutionError);

const ClassInfo TerminatedExecutionError::s_info = { "TerminatedExecutionError", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(TerminatedExecutionError) };

JSValue TerminatedExecutionError::defaultValue(const JSObject*, ExecState* exec, PreferredPrimitiveType hint)
{
    if (hint == PreferString)
        return jsNontrivialString(exec, String(ASCIILiteral("JavaScript execution terminated.")));
    return JSValue(QNaN);
}

JSObject* createTerminatedExecutionException(VM* vm)
{
    return TerminatedExecutionError::create(*vm);
}

bool isTerminatedExecutionException(JSObject* object)
{
    return object->inherits(&TerminatedExecutionError::s_info);
}

bool isTerminatedExecutionException(JSValue value)
{
    return value.inherits(&TerminatedExecutionError::s_info);
}


JSObject* createStackOverflowError(ExecState* exec)
{
    return createRangeError(exec, ASCIILiteral("Maximum call stack size exceeded."));
}

JSObject* createStackOverflowError(JSGlobalObject* globalObject)
{
    return createRangeError(globalObject, ASCIILiteral("Maximum call stack size exceeded."));
}

JSObject* createUndefinedVariableError(ExecState* exec, const Identifier& ident)
{
    String message(makeString("Can't find variable: ", ident.string()));
    return createReferenceError(exec, message);
}
    
JSString* errorDescriptionForValue(ExecState* exec, JSValue v)
{
    VM& vm = exec->vm();
    if (v.isNull())
        return vm.smallStrings.nullString();
    if (v.isUndefined())
        return vm.smallStrings.undefinedString();
    if (v.isInt32())
        return jsString(&vm, vm.numericStrings.add(v.asInt32()));
    if (v.isDouble())
        return jsString(&vm, vm.numericStrings.add(v.asDouble()));
    if (v.isTrue())
        return vm.smallStrings.trueString();
    if (v.isFalse())
        return vm.smallStrings.falseString();
    if (v.isString())
        return jsCast<JSString*>(v.asCell());
    if (v.isObject()) {
        CallData callData;
        JSObject* object = asObject(v);
        if (object->methodTable()->getCallData(object, callData) != CallTypeNone)
            return vm.smallStrings.functionString();
    }
    return jsString(exec, asObject(v)->methodTable()->className(asObject(v)));
}
    
JSObject* createError(ExecState* exec, ErrorFactory errorFactory, JSValue value, const String& message)
{
    String errorMessage = makeString(errorDescriptionForValue(exec, value)->value(exec), " ", message);
    JSObject* exception = errorFactory(exec, errorMessage);
    ASSERT(exception->isErrorInstance());
    static_cast<ErrorInstance*>(exception)->setAppendSourceToMessage();
    return exception;
}

JSObject* createInvalidParameterError(ExecState* exec, const char* op, JSValue value)
{
    return createError(exec, createTypeError, value, makeString("is not a valid argument for '", op, "'"));
}

JSObject* createNotAConstructorError(ExecState* exec, JSValue value)
{
    return createError(exec, createTypeError, value, "is not a constructor");
}

JSObject* createNotAFunctionError(ExecState* exec, JSValue value)
{
    return createError(exec, createTypeError, value, "is not a function");
}

JSObject* createNotAnObjectError(ExecState* exec, JSValue value)
{
    return createError(exec, createTypeError, value, "is not an object");
}

JSObject* createErrorForInvalidGlobalAssignment(ExecState* exec, const String& propertyName)
{
    return createReferenceError(exec, makeString("Strict mode forbids implicit creation of global property '", propertyName, "'"));
}

JSObject* createOutOfMemoryError(JSGlobalObject* globalObject)
{
    return createError(globalObject, ASCIILiteral("Out of memory"));
}

JSObject* throwOutOfMemoryError(ExecState* exec)
{
    return throwError(exec, createOutOfMemoryError(exec->lexicalGlobalObject()));
}

JSObject* throwStackOverflowError(ExecState* exec)
{
    Interpreter::ErrorHandlingMode mode(exec);
    return throwError(exec, createStackOverflowError(exec));
}

JSObject* throwTerminatedExecutionException(ExecState* exec)
{
    Interpreter::ErrorHandlingMode mode(exec);
    return throwError(exec, createTerminatedExecutionException(&exec->vm()));
}

} // namespace JSC
