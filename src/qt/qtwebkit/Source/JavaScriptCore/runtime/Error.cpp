/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Eric Seidel (eric@webkit.org)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "Error.h"

#include "ConstructData.h"
#include "ErrorConstructor.h"
#include "ExceptionHelpers.h"
#include "FunctionPrototype.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "JSObject.h"
#include "JSString.h"
#include "NativeErrorConstructor.h"
#include "Operations.h"
#include "SourceCode.h"

#include <wtf/text/StringBuilder.h>

namespace JSC {

static const char* linePropertyName = "line";
static const char* sourceURLPropertyName = "sourceURL";

JSObject* createError(JSGlobalObject* globalObject, const String& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(globalObject->vm(), globalObject->errorStructure(), message);
}

JSObject* createEvalError(JSGlobalObject* globalObject, const String& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(globalObject->vm(), globalObject->evalErrorConstructor()->errorStructure(), message);
}

JSObject* createRangeError(JSGlobalObject* globalObject, const String& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(globalObject->vm(), globalObject->rangeErrorConstructor()->errorStructure(), message);
}

JSObject* createReferenceError(JSGlobalObject* globalObject, const String& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(globalObject->vm(), globalObject->referenceErrorConstructor()->errorStructure(), message);
}

JSObject* createSyntaxError(JSGlobalObject* globalObject, const String& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(globalObject->vm(), globalObject->syntaxErrorConstructor()->errorStructure(), message);
}

JSObject* createTypeError(JSGlobalObject* globalObject, const String& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(globalObject->vm(), globalObject->typeErrorConstructor()->errorStructure(), message);
}

JSObject* createNotEnoughArgumentsError(JSGlobalObject* globalObject)
{
    return createTypeError(globalObject, ASCIILiteral("Not enough arguments"));
}

JSObject* createURIError(JSGlobalObject* globalObject, const String& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(globalObject->vm(), globalObject->URIErrorConstructor()->errorStructure(), message);
}

JSObject* createError(ExecState* exec, const String& message)
{
    return createError(exec->lexicalGlobalObject(), message);
}

JSObject* createEvalError(ExecState* exec, const String& message)
{
    return createEvalError(exec->lexicalGlobalObject(), message);
}

JSObject* createRangeError(ExecState* exec, const String& message)
{
    return createRangeError(exec->lexicalGlobalObject(), message);
}

JSObject* createReferenceError(ExecState* exec, const String& message)
{
    return createReferenceError(exec->lexicalGlobalObject(), message);
}

JSObject* createSyntaxError(ExecState* exec, const String& message)
{
    return createSyntaxError(exec->lexicalGlobalObject(), message);
}

JSObject* createTypeError(ExecState* exec, const String& message)
{
    return createTypeError(exec->lexicalGlobalObject(), message);
}

JSObject* createNotEnoughArgumentsError(ExecState* exec)
{
    return createNotEnoughArgumentsError(exec->lexicalGlobalObject());
}

JSObject* createURIError(ExecState* exec, const String& message)
{
    return createURIError(exec->lexicalGlobalObject(), message);
}

JSObject* addErrorInfo(CallFrame* callFrame, JSObject* error, int line, const SourceCode& source)
{
    VM* vm = &callFrame->vm();
    const String& sourceURL = source.provider()->url();

    if (line != -1)
        error->putDirect(*vm, Identifier(vm, linePropertyName), jsNumber(line), ReadOnly | DontDelete);
    if (!sourceURL.isNull())
        error->putDirect(*vm, Identifier(vm, sourceURLPropertyName), jsString(vm, sourceURL), ReadOnly | DontDelete);

    vm->interpreter->addStackTraceIfNecessary(callFrame, error);

    return error;
}


bool hasErrorInfo(ExecState* exec, JSObject* error)
{
    return error->hasProperty(exec, Identifier(exec, linePropertyName))
        || error->hasProperty(exec, Identifier(exec, sourceURLPropertyName));
}

JSValue throwError(ExecState* exec, JSValue error)
{
    Interpreter::addStackTraceIfNecessary(exec, error);
    exec->vm().exception = error;
    return error;
}

JSObject* throwError(ExecState* exec, JSObject* error)
{
    Interpreter::addStackTraceIfNecessary(exec, error);
    exec->vm().exception = error;
    return error;
}

JSObject* throwTypeError(ExecState* exec)
{
    return throwError(exec, createTypeError(exec, ASCIILiteral("Type error")));
}

JSObject* throwSyntaxError(ExecState* exec)
{
    return throwError(exec, createSyntaxError(exec, ASCIILiteral("Syntax error")));
}

const ClassInfo StrictModeTypeErrorFunction::s_info = { "Function", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(StrictModeTypeErrorFunction) };

void StrictModeTypeErrorFunction::destroy(JSCell* cell)
{
    static_cast<StrictModeTypeErrorFunction*>(cell)->StrictModeTypeErrorFunction::~StrictModeTypeErrorFunction();
}

} // namespace JSC
