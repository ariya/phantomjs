/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2008 Apple Inc. All rights reserved.
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
#include "ObjectPrototype.h"

#include "Error.h"
#include "JSFunction.h"
#include "JSString.h"
#include "JSStringBuilder.h"

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(ObjectPrototype);

static EncodedJSValue JSC_HOST_CALL objectProtoFuncValueOf(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncHasOwnProperty(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncIsPrototypeOf(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncDefineGetter(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncDefineSetter(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncLookupGetter(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncLookupSetter(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncPropertyIsEnumerable(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncToLocaleString(ExecState*);

ObjectPrototype::ObjectPrototype(ExecState* exec, JSGlobalObject* globalObject, Structure* stucture, Structure* functionStructure)
    : JSNonFinalObject(exec->globalData(), stucture)
    , m_hasNoPropertiesWithUInt32Names(true)
{
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 0, exec->propertyNames().toString, objectProtoFuncToString), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 0, exec->propertyNames().toLocaleString, objectProtoFuncToLocaleString), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 0, exec->propertyNames().valueOf, objectProtoFuncValueOf), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 1, exec->propertyNames().hasOwnProperty, objectProtoFuncHasOwnProperty), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 1, exec->propertyNames().propertyIsEnumerable, objectProtoFuncPropertyIsEnumerable), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 1, exec->propertyNames().isPrototypeOf, objectProtoFuncIsPrototypeOf), DontEnum);

    // Mozilla extensions
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 2, exec->propertyNames().__defineGetter__, objectProtoFuncDefineGetter), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 2, exec->propertyNames().__defineSetter__, objectProtoFuncDefineSetter), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 1, exec->propertyNames().__lookupGetter__, objectProtoFuncLookupGetter), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 1, exec->propertyNames().__lookupSetter__, objectProtoFuncLookupSetter), DontEnum);
}

void ObjectPrototype::put(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    JSObject::put(exec, propertyName, value, slot);

    if (m_hasNoPropertiesWithUInt32Names) {
        bool isUInt32;
        propertyName.toUInt32(isUInt32);
        m_hasNoPropertiesWithUInt32Names = !isUInt32;
    }
}

bool ObjectPrototype::getOwnPropertySlot(ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    if (m_hasNoPropertiesWithUInt32Names)
        return false;
    return JSObject::getOwnPropertySlot(exec, propertyName, slot);
}

// ------------------------------ Functions --------------------------------

// ECMA 15.2.4.2, 15.2.4.4, 15.2.4.5, 15.2.4.7

EncodedJSValue JSC_HOST_CALL objectProtoFuncValueOf(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    return JSValue::encode(thisValue.toThisObject(exec));
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncHasOwnProperty(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    return JSValue::encode(jsBoolean(thisValue.toThisObject(exec)->hasOwnProperty(exec, Identifier(exec, exec->argument(0).toString(exec)))));
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncIsPrototypeOf(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    JSObject* thisObj = thisValue.toThisObject(exec);

    if (!exec->argument(0).isObject())
        return JSValue::encode(jsBoolean(false));

    JSValue v = asObject(exec->argument(0))->prototype();

    while (true) {
        if (!v.isObject())
            return JSValue::encode(jsBoolean(false));
        if (v == thisObj)
            return JSValue::encode(jsBoolean(true));
        v = asObject(v)->prototype();
    }
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncDefineGetter(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    CallData callData;
    if (getCallData(exec->argument(1), callData) == CallTypeNone)
        return throwVMError(exec, createSyntaxError(exec, "invalid getter usage"));
    thisValue.toThisObject(exec)->defineGetter(exec, Identifier(exec, exec->argument(0).toString(exec)), asObject(exec->argument(1)));
    return JSValue::encode(jsUndefined());
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncDefineSetter(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    CallData callData;
    if (getCallData(exec->argument(1), callData) == CallTypeNone)
        return throwVMError(exec, createSyntaxError(exec, "invalid setter usage"));
    thisValue.toThisObject(exec)->defineSetter(exec, Identifier(exec, exec->argument(0).toString(exec)), asObject(exec->argument(1)));
    return JSValue::encode(jsUndefined());
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncLookupGetter(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    return JSValue::encode(thisValue.toThisObject(exec)->lookupGetter(exec, Identifier(exec, exec->argument(0).toString(exec))));
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncLookupSetter(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    return JSValue::encode(thisValue.toThisObject(exec)->lookupSetter(exec, Identifier(exec, exec->argument(0).toString(exec))));
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncPropertyIsEnumerable(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    return JSValue::encode(jsBoolean(thisValue.toThisObject(exec)->propertyIsEnumerable(exec, Identifier(exec, exec->argument(0).toString(exec)))));
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncToLocaleString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    return JSValue::encode(thisValue.toThisJSString(exec));
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncToString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    return JSValue::encode(jsMakeNontrivialString(exec, "[object ", thisValue.toThisObject(exec)->className(), "]"));
}

} // namespace JSC
