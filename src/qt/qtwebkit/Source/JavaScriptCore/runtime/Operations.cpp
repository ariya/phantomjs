/*
 * Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#include "Operations.h"

#include "Error.h"
#include "JSObject.h"
#include "JSString.h"
#include <math.h>
#include <stdio.h>
#include <wtf/MathExtras.h>

namespace JSC {

bool JSValue::equalSlowCase(ExecState* exec, JSValue v1, JSValue v2)
{
    return equalSlowCaseInline(exec, v1, v2);
}

bool JSValue::strictEqualSlowCase(ExecState* exec, JSValue v1, JSValue v2)
{
    return strictEqualSlowCaseInline(exec, v1, v2);
}

NEVER_INLINE JSValue jsAddSlowCase(CallFrame* callFrame, JSValue v1, JSValue v2)
{
    // exception for the Date exception in defaultValue()
    JSValue p1 = v1.toPrimitive(callFrame);
    JSValue p2 = v2.toPrimitive(callFrame);

    if (p1.isString())
        return jsString(callFrame, asString(p1), p2.toString(callFrame));

    if (p2.isString())
        return jsString(callFrame, p1.toString(callFrame), asString(p2));

    return jsNumber(p1.toNumber(callFrame) + p2.toNumber(callFrame));
}

JSValue jsTypeStringForValue(VM& vm, JSGlobalObject* globalObject, JSValue v)
{
    if (v.isUndefined())
        return vm.smallStrings.undefinedString();
    if (v.isBoolean())
        return vm.smallStrings.booleanString();
    if (v.isNumber())
        return vm.smallStrings.numberString();
    if (v.isString())
        return vm.smallStrings.stringString();
    if (v.isObject()) {
        // Return "undefined" for objects that should be treated
        // as null when doing comparisons.
        if (asObject(v)->structure()->masqueradesAsUndefined(globalObject))
            return vm.smallStrings.undefinedString();
        CallData callData;
        JSObject* object = asObject(v);
        if (object->methodTable()->getCallData(object, callData) != CallTypeNone)
            return vm.smallStrings.functionString();
    }
    return vm.smallStrings.objectString();
}

JSValue jsTypeStringForValue(CallFrame* callFrame, JSValue v)
{
    return jsTypeStringForValue(callFrame->vm(), callFrame->lexicalGlobalObject(), v);
}

bool jsIsObjectType(CallFrame* callFrame, JSValue v)
{
    if (!v.isCell())
        return v.isNull();

    JSType type = v.asCell()->structure()->typeInfo().type();
    if (type == StringType)
        return false;
    if (type >= ObjectType) {
        if (asObject(v)->structure()->masqueradesAsUndefined(callFrame->lexicalGlobalObject()))
            return false;
        CallData callData;
        JSObject* object = asObject(v);
        if (object->methodTable()->getCallData(object, callData) != CallTypeNone)
            return false;
    }
    return true;
}

bool jsIsFunctionType(JSValue v)
{
    if (v.isObject()) {
        CallData callData;
        JSObject* object = asObject(v);
        if (object->methodTable()->getCallData(object, callData) != CallTypeNone)
            return true;
    }
    return false;
}

} // namespace JSC
