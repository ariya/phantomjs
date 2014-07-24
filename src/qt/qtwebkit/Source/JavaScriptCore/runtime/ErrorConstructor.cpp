/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2008 Apple Inc. All rights reserved.
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
#include "ErrorConstructor.h"

#include "ErrorPrototype.h"
#include "JSGlobalObject.h"
#include "JSString.h"
#include "Operations.h"

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(ErrorConstructor);

const ClassInfo ErrorConstructor::s_info = { "Function", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(ErrorConstructor) };

ErrorConstructor::ErrorConstructor(JSGlobalObject* globalObject, Structure* structure)
    : InternalFunction(globalObject, structure)
{
}

void ErrorConstructor::finishCreation(ExecState* exec, ErrorPrototype* errorPrototype)
{
    Base::finishCreation(exec->vm(), errorPrototype->classInfo()->className);
    // ECMA 15.11.3.1 Error.prototype
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().prototype, errorPrototype, DontEnum | DontDelete | ReadOnly);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().length, jsNumber(1), DontDelete | ReadOnly | DontEnum);
}

// ECMA 15.9.3

static EncodedJSValue JSC_HOST_CALL constructWithErrorConstructor(ExecState* exec)
{
    JSValue message = exec->argumentCount() ? exec->argument(0) : jsUndefined();
    Structure* errorStructure = asInternalFunction(exec->callee())->globalObject()->errorStructure();
    return JSValue::encode(ErrorInstance::create(exec, errorStructure, message));
}

ConstructType ErrorConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructWithErrorConstructor;
    return ConstructTypeHost;
}

static EncodedJSValue JSC_HOST_CALL callErrorConstructor(ExecState* exec)
{
    JSValue message = exec->argumentCount() ? exec->argument(0) : jsUndefined();
    Structure* errorStructure = asInternalFunction(exec->callee())->globalObject()->errorStructure();
    return JSValue::encode(ErrorInstance::create(exec, errorStructure, message));
}

CallType ErrorConstructor::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callErrorConstructor;
    return CallTypeHost;
}

} // namespace JSC
