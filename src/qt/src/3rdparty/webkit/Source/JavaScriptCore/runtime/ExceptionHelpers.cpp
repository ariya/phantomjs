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
#include "UStringConcatenate.h"

namespace JSC {

class InterruptedExecutionError : public JSNonFinalObject {
public:
    InterruptedExecutionError(JSGlobalData* globalData)
        : JSNonFinalObject(*globalData, globalData->interruptedExecutionErrorStructure.get())
    {
    }

    virtual ComplType exceptionType() const { return Interrupted; }

    virtual UString toString(ExecState*) const { return "JavaScript execution exceeded timeout."; }
};

JSObject* createInterruptedExecutionException(JSGlobalData* globalData)
{
    return new (globalData) InterruptedExecutionError(globalData);
}

class TerminatedExecutionError : public JSNonFinalObject {
public:
    TerminatedExecutionError(JSGlobalData* globalData)
        : JSNonFinalObject(*globalData, globalData->terminatedExecutionErrorStructure.get())
    {
    }

    virtual ComplType exceptionType() const { return Terminated; }

    virtual UString toString(ExecState*) const { return "JavaScript execution terminated."; }
};

JSObject* createTerminatedExecutionException(JSGlobalData* globalData)
{
    return new (globalData) TerminatedExecutionError(globalData);
}

JSObject* createStackOverflowError(ExecState* exec)
{
    return createRangeError(exec, "Maximum call stack size exceeded.");
}

JSObject* createStackOverflowError(JSGlobalObject* globalObject)
{
    return createRangeError(globalObject, "Maximum call stack size exceeded.");
}

JSObject* createUndefinedVariableError(ExecState* exec, const Identifier& ident)
{
    UString message(makeUString("Can't find variable: ", ident.ustring()));
    return createReferenceError(exec, message);
}
    
JSObject* createInvalidParamError(ExecState* exec, const char* op, JSValue value)
{
    UString errorMessage = makeUString("'", value.toString(exec), "' is not a valid argument for '", op, "'");
    JSObject* exception = createTypeError(exec, errorMessage);
    ASSERT(exception->isErrorInstance());
    static_cast<ErrorInstance*>(exception)->setAppendSourceToMessage();
    return exception;
}

JSObject* createNotAConstructorError(ExecState* exec, JSValue value)
{
    UString errorMessage = makeUString("'", value.toString(exec), "' is not a constructor");
    JSObject* exception = createTypeError(exec, errorMessage);
    ASSERT(exception->isErrorInstance());
    static_cast<ErrorInstance*>(exception)->setAppendSourceToMessage();
    return exception;
}

JSObject* createNotAFunctionError(ExecState* exec, JSValue value)
{
    UString errorMessage = makeUString("'", value.toString(exec), "' is not a function");
    JSObject* exception = createTypeError(exec, errorMessage);
    ASSERT(exception->isErrorInstance());
    static_cast<ErrorInstance*>(exception)->setAppendSourceToMessage();
    return exception;
}

JSObject* createNotAnObjectError(ExecState* exec, JSValue value)
{
    UString errorMessage = makeUString("'", value.toString(exec), "' is not an object");
    JSObject* exception = createTypeError(exec, errorMessage);
    ASSERT(exception->isErrorInstance());
    static_cast<ErrorInstance*>(exception)->setAppendSourceToMessage();
    return exception;
}

JSObject* createErrorForInvalidGlobalAssignment(ExecState* exec, const UString& propertyName)
{
    return createReferenceError(exec, makeUString("Strict mode forbids implicit creation of global property '", propertyName, "'"));
}

JSObject* createOutOfMemoryError(JSGlobalObject* globalObject)
{
    return createError(globalObject, "Out of memory");
}

JSObject* throwOutOfMemoryError(ExecState* exec)
{
    return throwError(exec, createOutOfMemoryError(exec->lexicalGlobalObject()));
}

JSObject* throwStackOverflowError(ExecState* exec)
{
    return throwError(exec, createStackOverflowError(exec));
}

} // namespace JSC
