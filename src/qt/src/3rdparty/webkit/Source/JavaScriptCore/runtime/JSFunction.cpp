/*
 *  Copyright (C) 1999-2002 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *  Copyright (C) 2007 Maks Orlovich
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
#include "JSFunction.h"

#include "CodeBlock.h"
#include "CommonIdentifiers.h"
#include "CallFrame.h"
#include "ExceptionHelpers.h"
#include "FunctionPrototype.h"
#include "JSGlobalObject.h"
#include "JSNotAnObject.h"
#include "Interpreter.h"
#include "ObjectPrototype.h"
#include "Parser.h"
#include "PropertyNameArray.h"
#include "ScopeChainMark.h"

using namespace WTF;
using namespace Unicode;

namespace JSC {
EncodedJSValue JSC_HOST_CALL callHostFunctionAsConstructor(ExecState* exec)
{
    return throwVMError(exec, createNotAConstructorError(exec, exec->callee()));
}

ASSERT_CLASS_FITS_IN_CELL(JSFunction);

const ClassInfo JSFunction::s_info = { "Function", &Base::s_info, 0, 0 };

bool JSFunction::isHostFunctionNonInline() const
{
    return isHostFunction();
}

JSFunction::JSFunction(VPtrStealingHackType)
    : Base(VPtrStealingHack)
{
}

JSFunction::JSFunction(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, int length, const Identifier& name, NativeExecutable* thunk)
    : Base(globalObject, structure)
    , m_executable(exec->globalData(), this, thunk)
    , m_scopeChain(exec->globalData(), this, globalObject->globalScopeChain())
{
    ASSERT(inherits(&s_info));
    putDirect(exec->globalData(), exec->globalData().propertyNames->name, jsString(exec, name.isNull() ? "" : name.ustring()), DontDelete | ReadOnly | DontEnum);
    putDirect(exec->globalData(), exec->propertyNames().length, jsNumber(length), DontDelete | ReadOnly | DontEnum);
}

JSFunction::JSFunction(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, int length, const Identifier& name, NativeFunction func)
    : Base(globalObject, structure)
    , m_scopeChain(exec->globalData(), this, globalObject->globalScopeChain())
{
    ASSERT(inherits(&s_info));
    
    // Can't do this during initialization because getHostFunction might do a GC allocation.
    m_executable.set(exec->globalData(), this, exec->globalData().getHostFunction(func));
    
    putDirect(exec->globalData(), exec->globalData().propertyNames->name, jsString(exec, name.isNull() ? "" : name.ustring()), DontDelete | ReadOnly | DontEnum);
    putDirect(exec->globalData(), exec->propertyNames().length, jsNumber(length), DontDelete | ReadOnly | DontEnum);
}

JSFunction::JSFunction(ExecState* exec, FunctionExecutable* executable, ScopeChainNode* scopeChainNode)
    : Base(scopeChainNode->globalObject.get(), scopeChainNode->globalObject->functionStructure())
    , m_executable(exec->globalData(), this, executable)
    , m_scopeChain(exec->globalData(), this, scopeChainNode)
{
    ASSERT(inherits(&s_info));
    const Identifier& name = static_cast<FunctionExecutable*>(m_executable.get())->name();
    putDirect(exec->globalData(), exec->globalData().propertyNames->name, jsString(exec, name.isNull() ? "" : name.ustring()), DontDelete | ReadOnly | DontEnum);
}

JSFunction::~JSFunction()
{
    ASSERT(vptr() == JSGlobalData::jsFunctionVPtr);
}

static const char* StrictModeCallerAccessError = "Cannot access caller property of a strict mode function";
static const char* StrictModeArgumentsAccessError = "Cannot access arguments property of a strict mode function";

static void createDescriptorForThrowingProperty(ExecState* exec, PropertyDescriptor& descriptor, const char* message)
{
    JSValue thrower = createTypeErrorFunction(exec, message);
    descriptor.setAccessorDescriptor(thrower, thrower, DontEnum | DontDelete | Getter | Setter);
}

const UString& JSFunction::name(ExecState* exec)
{
    return asString(getDirect(exec->globalData(), exec->globalData().propertyNames->name))->tryGetValue();
}

const UString JSFunction::displayName(ExecState* exec)
{
    JSValue displayName = getDirect(exec->globalData(), exec->globalData().propertyNames->displayName);
    
    if (displayName && isJSString(&exec->globalData(), displayName))
        return asString(displayName)->tryGetValue();
    
    return UString();
}

const UString JSFunction::calculatedDisplayName(ExecState* exec)
{
    const UString explicitName = displayName(exec);
    
    if (!explicitName.isEmpty())
        return explicitName;
    
    return name(exec);
}

void JSFunction::visitChildren(SlotVisitor& visitor)
{
    Base::visitChildren(visitor);

    visitor.append(&m_scopeChain);
    if (m_executable)
        visitor.append(&m_executable);
}

CallType JSFunction::getCallData(CallData& callData)
{
    if (isHostFunction()) {
        callData.native.function = nativeFunction();
        return CallTypeHost;
    }
    callData.js.functionExecutable = jsExecutable();
    callData.js.scopeChain = scope();
    return CallTypeJS;
}

JSValue JSFunction::argumentsGetter(ExecState* exec, JSValue slotBase, const Identifier&)
{
    JSFunction* thisObj = asFunction(slotBase);
    ASSERT(!thisObj->isHostFunction());
    return exec->interpreter()->retrieveArguments(exec, thisObj);
}

JSValue JSFunction::callerGetter(ExecState* exec, JSValue slotBase, const Identifier&)
{
    JSFunction* thisObj = asFunction(slotBase);
    ASSERT(!thisObj->isHostFunction());
    return exec->interpreter()->retrieveCaller(exec, thisObj);
}

JSValue JSFunction::lengthGetter(ExecState*, JSValue slotBase, const Identifier&)
{
    JSFunction* thisObj = asFunction(slotBase);
    ASSERT(!thisObj->isHostFunction());
    return jsNumber(thisObj->jsExecutable()->parameterCount());
}

static inline WriteBarrierBase<Unknown>* createPrototypeProperty(JSGlobalData& globalData, JSGlobalObject* globalObject, JSFunction* function)
{
    ExecState* exec = globalObject->globalExec();
    if (WriteBarrierBase<Unknown>* location = function->getDirectLocation(globalData, exec->propertyNames().prototype))
        return location;
    JSObject* prototype = constructEmptyObject(exec, globalObject->emptyObjectStructure());
    prototype->putDirect(globalData, exec->propertyNames().constructor, function, DontEnum);
    function->putDirect(globalData, exec->propertyNames().prototype, prototype, DontDelete | DontEnum);
    return function->getDirectLocation(exec->globalData(), exec->propertyNames().prototype);
}

void JSFunction::preventExtensions(JSGlobalData& globalData)
{
    createPrototypeProperty(globalData, scope()->globalObject.get(), this);
    JSObject::preventExtensions(globalData);
}

bool JSFunction::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    if (isHostFunction())
        return Base::getOwnPropertySlot(exec, propertyName, slot);

    if (propertyName == exec->propertyNames().prototype) {
        WriteBarrierBase<Unknown>* location = getDirectLocation(exec->globalData(), propertyName);

        if (!location)
            location = createPrototypeProperty(exec->globalData(), scope()->globalObject.get(), this);

        slot.setValue(this, location->get(), offsetForLocation(location));
    }

    if (propertyName == exec->propertyNames().arguments) {
        if (jsExecutable()->isStrictMode()) {
            throwTypeError(exec, "Can't access arguments object of a strict mode function");
            slot.setValue(jsNull());
            return true;
        }
   
        slot.setCacheableCustom(this, argumentsGetter);
        return true;
    }

    if (propertyName == exec->propertyNames().length) {
        slot.setCacheableCustom(this, lengthGetter);
        return true;
    }

    if (propertyName == exec->propertyNames().caller) {
        if (jsExecutable()->isStrictMode()) {
            throwTypeError(exec, StrictModeCallerAccessError);
            slot.setValue(jsNull());
            return true;
        }
        slot.setCacheableCustom(this, callerGetter);
        return true;
    }

    return Base::getOwnPropertySlot(exec, propertyName, slot);
}

bool JSFunction::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    if (isHostFunction())
        return Base::getOwnPropertyDescriptor(exec, propertyName, descriptor);
    
    if (propertyName == exec->propertyNames().prototype) {
        PropertySlot slot;
        getOwnPropertySlot(exec, propertyName, slot);
        return Base::getOwnPropertyDescriptor(exec, propertyName, descriptor);
    }
    
    if (propertyName == exec->propertyNames().arguments) {
        if (jsExecutable()->isStrictMode())
            createDescriptorForThrowingProperty(exec, descriptor, StrictModeArgumentsAccessError);
        else
            descriptor.setDescriptor(exec->interpreter()->retrieveArguments(exec, this), ReadOnly | DontEnum | DontDelete);
        return true;
    }
    
    if (propertyName == exec->propertyNames().length) {
        descriptor.setDescriptor(jsNumber(jsExecutable()->parameterCount()), ReadOnly | DontEnum | DontDelete);
        return true;
    }
    
    if (propertyName == exec->propertyNames().caller) {
        if (jsExecutable()->isStrictMode())
            createDescriptorForThrowingProperty(exec, descriptor, StrictModeCallerAccessError);
        else
            descriptor.setDescriptor(exec->interpreter()->retrieveCaller(exec, this), ReadOnly | DontEnum | DontDelete);
        return true;
    }
    
    return Base::getOwnPropertyDescriptor(exec, propertyName, descriptor);
}

void JSFunction::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    if (!isHostFunction() && (mode == IncludeDontEnumProperties)) {
        // Make sure prototype has been reified.
        PropertySlot slot;
        getOwnPropertySlot(exec, exec->propertyNames().prototype, slot);

        propertyNames.add(exec->propertyNames().arguments);
        propertyNames.add(exec->propertyNames().callee);
        propertyNames.add(exec->propertyNames().caller);
        propertyNames.add(exec->propertyNames().length);
    }
    Base::getOwnPropertyNames(exec, propertyNames, mode);
}

void JSFunction::put(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    if (isHostFunction()) {
        Base::put(exec, propertyName, value, slot);
        return;
    }
    if (propertyName == exec->propertyNames().prototype) {
        // Make sure prototype has been reified, such that it can only be overwritten
        // following the rules set out in ECMA-262 8.12.9.
        PropertySlot slot;
        getOwnPropertySlot(exec, propertyName, slot);
    }
    if (jsExecutable()->isStrictMode()) {
        if (propertyName == exec->propertyNames().arguments) {
            throwTypeError(exec, StrictModeArgumentsAccessError);
            return;
        }
        if (propertyName == exec->propertyNames().caller) {
            throwTypeError(exec, StrictModeCallerAccessError);
            return;
        }
    }
    if (propertyName == exec->propertyNames().arguments || propertyName == exec->propertyNames().length)
        return;
    Base::put(exec, propertyName, value, slot);
}

bool JSFunction::deleteProperty(ExecState* exec, const Identifier& propertyName)
{
    if (isHostFunction())
        return Base::deleteProperty(exec, propertyName);
    if (propertyName == exec->propertyNames().arguments || propertyName == exec->propertyNames().length)
        return false;
    return Base::deleteProperty(exec, propertyName);
}

// ECMA 13.2.2 [[Construct]]
ConstructType JSFunction::getConstructData(ConstructData& constructData)
{
    if (isHostFunction())
        return ConstructTypeNone;
    constructData.js.functionExecutable = jsExecutable();
    constructData.js.scopeChain = scope();
    return ConstructTypeJS;
}

} // namespace JSC
