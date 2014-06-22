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
#include "JSBoundFunction.h"

#include "GetterSetter.h"
#include "JSGlobalObject.h"
#include "Operations.h"

namespace JSC {

const ClassInfo JSBoundFunction::s_info = { "Function", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(JSBoundFunction) };

EncodedJSValue JSC_HOST_CALL boundFunctionCall(ExecState* exec)
{
    JSBoundFunction* boundFunction = jsCast<JSBoundFunction*>(exec->callee());

    ASSERT(isJSArray(boundFunction->boundArgs())); // Currently this is true!
    JSArray* boundArgs = asArray(boundFunction->boundArgs());

    MarkedArgumentBuffer args;
    for (unsigned i = 0; i < boundArgs->length(); ++i)
        args.append(boundArgs->getIndexQuickly(i));
    for (unsigned i = 0; i < exec->argumentCount(); ++i)
        args.append(exec->argument(i));

    JSObject* targetFunction = boundFunction->targetFunction();
    CallData callData;
    CallType callType = getCallData(targetFunction, callData);
    ASSERT(callType != CallTypeNone);
    return JSValue::encode(call(exec, targetFunction, callType, callData, boundFunction->boundThis(), args));
}

EncodedJSValue JSC_HOST_CALL boundFunctionConstruct(ExecState* exec)
{
    JSBoundFunction* boundFunction = jsCast<JSBoundFunction*>(exec->callee());

    ASSERT(isJSArray(boundFunction->boundArgs())); // Currently this is true!
    JSArray* boundArgs = asArray(boundFunction->boundArgs());

    MarkedArgumentBuffer args;
    for (unsigned i = 0; i < boundArgs->length(); ++i)
        args.append(boundArgs->getIndexQuickly(i));
    for (unsigned i = 0; i < exec->argumentCount(); ++i)
        args.append(exec->argument(i));

    JSObject* targetFunction = boundFunction->targetFunction();
    ConstructData constructData;
    ConstructType constructType = getConstructData(targetFunction, constructData);
    ASSERT(constructType != ConstructTypeNone);
    return JSValue::encode(construct(exec, targetFunction, constructType, constructData, args));
}

JSBoundFunction* JSBoundFunction::create(ExecState* exec, JSGlobalObject* globalObject, JSObject* targetFunction, JSValue boundThis, JSValue boundArgs, int length, const String& name)
{
    ConstructData constructData;
    ConstructType constructType = JSC::getConstructData(targetFunction, constructData);
    bool canConstruct = constructType != ConstructTypeNone;
    NativeExecutable* executable = exec->vm().getHostFunction(boundFunctionCall, canConstruct ? boundFunctionConstruct : callHostFunctionAsConstructor);
    JSBoundFunction* function = new (NotNull, allocateCell<JSBoundFunction>(*exec->heap())) JSBoundFunction(exec, globalObject, globalObject->boundFunctionStructure(), targetFunction, boundThis, boundArgs);

    function->finishCreation(exec, executable, length, name);
    return function;
}

void JSBoundFunction::destroy(JSCell* cell)
{
    static_cast<JSBoundFunction*>(cell)->JSBoundFunction::~JSBoundFunction();
}

bool JSBoundFunction::customHasInstance(JSObject* object, ExecState* exec, JSValue value)
{
    return jsCast<JSBoundFunction*>(object)->m_targetFunction->hasInstance(exec, value);
}

JSBoundFunction::JSBoundFunction(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, JSObject* targetFunction, JSValue boundThis, JSValue boundArgs)
    : Base(exec, globalObject, structure)
    , m_targetFunction(exec->vm(), this, targetFunction)
    , m_boundThis(exec->vm(), this, boundThis)
    , m_boundArgs(exec->vm(), this, boundArgs)
{
}

void JSBoundFunction::finishCreation(ExecState* exec, NativeExecutable* executable, int length, const String& name)
{
    Base::finishCreation(exec, executable, length, name);
    ASSERT(inherits(&s_info));

    putDirectAccessor(exec, exec->propertyNames().arguments, globalObject()->throwTypeErrorGetterSetter(exec), DontDelete | DontEnum | Accessor);
    putDirectAccessor(exec, exec->propertyNames().caller, globalObject()->throwTypeErrorGetterSetter(exec), DontDelete | DontEnum | Accessor);
}

void JSBoundFunction::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSBoundFunction* thisObject = jsCast<JSBoundFunction*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    Base::visitChildren(thisObject, visitor);

    visitor.append(&thisObject->m_targetFunction);
    visitor.append(&thisObject->m_boundThis);
    visitor.append(&thisObject->m_boundArgs);
}

} // namespace JSC
