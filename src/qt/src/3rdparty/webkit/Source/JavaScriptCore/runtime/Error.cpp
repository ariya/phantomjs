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
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "JSObject.h"
#include "JSString.h"
#include "NativeErrorConstructor.h"
#include "SourceCode.h"

namespace JSC {

static const char* linePropertyName = "line";
static const char* sourceIdPropertyName = "sourceId";
static const char* sourceURLPropertyName = "sourceURL";

JSObject* createError(JSGlobalObject* globalObject, const UString& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(&globalObject->globalData(), globalObject->errorStructure(), message);
}

JSObject* createEvalError(JSGlobalObject* globalObject, const UString& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(&globalObject->globalData(), globalObject->evalErrorConstructor()->errorStructure(), message);
}

JSObject* createRangeError(JSGlobalObject* globalObject, const UString& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(&globalObject->globalData(), globalObject->rangeErrorConstructor()->errorStructure(), message);
}

JSObject* createReferenceError(JSGlobalObject* globalObject, const UString& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(&globalObject->globalData(), globalObject->referenceErrorConstructor()->errorStructure(), message);
}

JSObject* createSyntaxError(JSGlobalObject* globalObject, const UString& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(&globalObject->globalData(), globalObject->syntaxErrorConstructor()->errorStructure(), message);
}

JSObject* createTypeError(JSGlobalObject* globalObject, const UString& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(&globalObject->globalData(), globalObject->typeErrorConstructor()->errorStructure(), message);
}

JSObject* createURIError(JSGlobalObject* globalObject, const UString& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(&globalObject->globalData(), globalObject->URIErrorConstructor()->errorStructure(), message);
}

JSObject* createError(ExecState* exec, const UString& message)
{
    return createError(exec->lexicalGlobalObject(), message);
}

JSObject* createEvalError(ExecState* exec, const UString& message)
{
    return createEvalError(exec->lexicalGlobalObject(), message);
}

JSObject* createRangeError(ExecState* exec, const UString& message)
{
    return createRangeError(exec->lexicalGlobalObject(), message);
}

JSObject* createReferenceError(ExecState* exec, const UString& message)
{
    return createReferenceError(exec->lexicalGlobalObject(), message);
}

JSObject* createSyntaxError(ExecState* exec, const UString& message)
{
    return createSyntaxError(exec->lexicalGlobalObject(), message);
}

JSObject* createTypeError(ExecState* exec, const UString& message)
{
    return createTypeError(exec->lexicalGlobalObject(), message);
}

JSObject* createURIError(ExecState* exec, const UString& message)
{
    return createURIError(exec->lexicalGlobalObject(), message);
}

JSObject* addErrorInfo(JSGlobalData* globalData, JSObject* error, int line, const SourceCode& source)
{
    intptr_t sourceID = source.provider()->asID();
    const UString& sourceURL = source.provider()->url();

    if (line != -1)
        error->putWithAttributes(globalData, Identifier(globalData, linePropertyName), jsNumber(line), ReadOnly | DontDelete);
    if (sourceID != -1)
        error->putWithAttributes(globalData, Identifier(globalData, sourceIdPropertyName), jsNumber((double)sourceID), ReadOnly | DontDelete);
    if (!sourceURL.isNull())
        error->putWithAttributes(globalData, Identifier(globalData, sourceURLPropertyName), jsString(globalData, sourceURL), ReadOnly | DontDelete);

    return error;
}

JSObject* addErrorInfo(ExecState* exec, JSObject* error, int line, const SourceCode& source)
{
    return addErrorInfo(&exec->globalData(), error, line, source);
}

bool hasErrorInfo(ExecState* exec, JSObject* error)
{
    return error->hasProperty(exec, Identifier(exec, linePropertyName))
        || error->hasProperty(exec, Identifier(exec, sourceIdPropertyName))
        || error->hasProperty(exec, Identifier(exec, sourceURLPropertyName));
}

JSValue throwError(ExecState* exec, JSValue error)
{
    exec->globalData().exception = error;
    return error;
}

JSObject* throwError(ExecState* exec, JSObject* error)
{
    exec->globalData().exception = error;
    return error;
}

JSObject* throwTypeError(ExecState* exec)
{
    return throwError(exec, createTypeError(exec, "Type error"));
}

JSObject* throwSyntaxError(ExecState* exec)
{
    return throwError(exec, createSyntaxError(exec, "Syntax error"));
}

class StrictModeTypeErrorFunction : public InternalFunction {
public:
    StrictModeTypeErrorFunction(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, const UString& message)
        : InternalFunction(&exec->globalData(), globalObject, structure, exec->globalData().propertyNames->emptyIdentifier)
        , m_message(message)
    {
    }
    
    static EncodedJSValue JSC_HOST_CALL constructThrowTypeError(ExecState* exec)
    {
        throwTypeError(exec, static_cast<StrictModeTypeErrorFunction*>(exec->callee())->m_message);
        return JSValue::encode(jsNull());
    }
    
    ConstructType getConstructData(ConstructData& constructData)
    {
        constructData.native.function = constructThrowTypeError;
        return ConstructTypeHost;
    }
    
    static EncodedJSValue JSC_HOST_CALL callThrowTypeError(ExecState* exec)
    {
        throwTypeError(exec, static_cast<StrictModeTypeErrorFunction*>(exec->callee())->m_message);
        return JSValue::encode(jsNull());
    }

    CallType getCallData(CallData& callData)
    {
        callData.native.function = callThrowTypeError;
        return CallTypeHost;
    }

private:
    UString m_message;
};

ASSERT_CLASS_FITS_IN_CELL(StrictModeTypeErrorFunction);

JSValue createTypeErrorFunction(ExecState* exec, const UString& message)
{
    return new (exec) StrictModeTypeErrorFunction(exec, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->internalFunctionStructure(), message);
}

} // namespace JSC
