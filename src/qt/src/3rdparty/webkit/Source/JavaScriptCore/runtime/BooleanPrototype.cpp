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
#include "BooleanPrototype.h"

#include "Error.h"
#include "ExceptionHelpers.h"
#include "JSFunction.h"
#include "JSString.h"
#include "ObjectPrototype.h"

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(BooleanPrototype);

// Functions
static EncodedJSValue JSC_HOST_CALL booleanProtoFuncToString(ExecState*);
static EncodedJSValue JSC_HOST_CALL booleanProtoFuncValueOf(ExecState*);

// ECMA 15.6.4

BooleanPrototype::BooleanPrototype(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, Structure* functionStructure)
    : BooleanObject(exec->globalData(), structure)
{
    setInternalValue(exec->globalData(), jsBoolean(false));

    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 0, exec->propertyNames().toString, booleanProtoFuncToString), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 0, exec->propertyNames().valueOf, booleanProtoFuncValueOf), DontEnum);
}


// ------------------------------ Functions --------------------------

// ECMA 15.6.4.2 + 15.6.4.3

EncodedJSValue JSC_HOST_CALL booleanProtoFuncToString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue == jsBoolean(false))
        return JSValue::encode(jsNontrivialString(exec, "false"));

    if (thisValue == jsBoolean(true))
        return JSValue::encode(jsNontrivialString(exec, "true"));

    if (!thisValue.inherits(&BooleanObject::s_info))
        return throwVMTypeError(exec);

    if (asBooleanObject(thisValue)->internalValue() == jsBoolean(false))
        return JSValue::encode(jsNontrivialString(exec, "false"));

    ASSERT(asBooleanObject(thisValue)->internalValue() == jsBoolean(true));
    return JSValue::encode(jsNontrivialString(exec, "true"));
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
