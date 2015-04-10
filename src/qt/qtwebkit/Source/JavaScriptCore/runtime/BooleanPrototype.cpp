/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2008, 2011 Apple Inc. All rights reserved.
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
#include "BooleanPrototype.h"

#include "Error.h"
#include "ExceptionHelpers.h"
#include "JSFunction.h"
#include "JSString.h"
#include "ObjectPrototype.h"
#include "Operations.h"

namespace JSC {

static EncodedJSValue JSC_HOST_CALL booleanProtoFuncToString(ExecState*);
static EncodedJSValue JSC_HOST_CALL booleanProtoFuncValueOf(ExecState*);

}

#include "BooleanPrototype.lut.h"

namespace JSC {

const ClassInfo BooleanPrototype::s_info = { "Boolean", &BooleanObject::s_info, 0, ExecState::booleanPrototypeTable, CREATE_METHOD_TABLE(BooleanPrototype) };

/* Source for BooleanPrototype.lut.h
@begin booleanPrototypeTable
  toString  booleanProtoFuncToString    DontEnum|Function 0
  valueOf   booleanProtoFuncValueOf     DontEnum|Function 0
@end
*/

ASSERT_HAS_TRIVIAL_DESTRUCTOR(BooleanPrototype);

BooleanPrototype::BooleanPrototype(ExecState* exec, Structure* structure)
    : BooleanObject(exec->vm(), structure)
{
}

void BooleanPrototype::finishCreation(ExecState* exec, JSGlobalObject*)
{
    Base::finishCreation(exec->vm());
    setInternalValue(exec->vm(), jsBoolean(false));

    ASSERT(inherits(&s_info));
}

bool BooleanPrototype::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<BooleanObject>(exec, ExecState::booleanPrototypeTable(exec), jsCast<BooleanPrototype*>(cell), propertyName, slot);
}

bool BooleanPrototype::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<BooleanObject>(exec, ExecState::booleanPrototypeTable(exec), jsCast<BooleanPrototype*>(object), propertyName, descriptor);
}

// ------------------------------ Functions ---------------------------

EncodedJSValue JSC_HOST_CALL booleanProtoFuncToString(ExecState* exec)
{
    VM* vm = &exec->vm();
    JSValue thisValue = exec->hostThisValue();
    if (thisValue == jsBoolean(false))
        return JSValue::encode(vm->smallStrings.falseString());

    if (thisValue == jsBoolean(true))
        return JSValue::encode(vm->smallStrings.trueString());

    if (!thisValue.inherits(&BooleanObject::s_info))
        return throwVMTypeError(exec);

    if (asBooleanObject(thisValue)->internalValue() == jsBoolean(false))
        return JSValue::encode(vm->smallStrings.falseString());

    ASSERT(asBooleanObject(thisValue)->internalValue() == jsBoolean(true));
    return JSValue::encode(vm->smallStrings.trueString());
}

EncodedJSValue JSC_HOST_CALL booleanProtoFuncValueOf(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isBoolean())
        return JSValue::encode(thisValue);

    if (!thisValue.inherits(&BooleanObject::s_info))
        return throwVMTypeError(exec);

    return JSValue::encode(asBooleanObject(thisValue)->internalValue());
}

} // namespace JSC
