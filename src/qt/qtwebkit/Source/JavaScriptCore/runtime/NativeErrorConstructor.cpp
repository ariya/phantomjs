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
#include "NativeErrorConstructor.h"

#include "ErrorInstance.h"
#include "JSFunction.h"
#include "JSString.h"
#include "NativeErrorPrototype.h"
#include "Operations.h"

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(NativeErrorConstructor);

const ClassInfo NativeErrorConstructor::s_info = { "Function", &InternalFunction::s_info, 0, 0, CREATE_METHOD_TABLE(NativeErrorConstructor) };

NativeErrorConstructor::NativeErrorConstructor(JSGlobalObject* globalObject, Structure* structure)
    : InternalFunction(globalObject, structure)
{
}

void NativeErrorConstructor::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    NativeErrorConstructor* thisObject = jsCast<NativeErrorConstructor*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());

    InternalFunction::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_errorStructure);
}

static EncodedJSValue JSC_HOST_CALL constructWithNativeErrorConstructor(ExecState* exec)
{
    JSValue message = exec->argumentCount() ? exec->argument(0) : jsUndefined();
    Structure* errorStructure = static_cast<NativeErrorConstructor*>(exec->callee())->errorStructure();
    ASSERT(errorStructure);
    return JSValue::encode(ErrorInstance::create(exec, errorStructure, message));
}

ConstructType NativeErrorConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructWithNativeErrorConstructor;
    return ConstructTypeHost;
}
    
static EncodedJSValue JSC_HOST_CALL callNativeErrorConstructor(ExecState* exec)
{
    JSValue message = exec->argumentCount() ? exec->argument(0) : jsUndefined();
    Structure* errorStructure = static_cast<NativeErrorConstructor*>(exec->callee())->errorStructure();
    return JSValue::encode(ErrorInstance::create(exec, errorStructure, message));
}

CallType NativeErrorConstructor::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callNativeErrorConstructor;
    return CallTypeHost;
}

} // namespace JSC
