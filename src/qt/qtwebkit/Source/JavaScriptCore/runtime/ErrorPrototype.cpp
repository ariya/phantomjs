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
#include "ErrorPrototype.h"

#include "Error.h"
#include "JSFunction.h"
#include "JSString.h"
#include "JSStringBuilder.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include "StringRecursionChecker.h"

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(ErrorPrototype);

static EncodedJSValue JSC_HOST_CALL errorProtoFuncToString(ExecState*);

}

#include "ErrorPrototype.lut.h"

namespace JSC {

const ClassInfo ErrorPrototype::s_info = { "Error", &ErrorInstance::s_info, 0, ExecState::errorPrototypeTable, CREATE_METHOD_TABLE(ErrorPrototype) };

/* Source for ErrorPrototype.lut.h
@begin errorPrototypeTable
  toString          errorProtoFuncToString         DontEnum|Function 0
@end
*/

ErrorPrototype::ErrorPrototype(ExecState* exec, Structure* structure)
    : ErrorInstance(exec->vm(), structure)
{
}

void ErrorPrototype::finishCreation(ExecState* exec, JSGlobalObject*)
{
    Base::finishCreation(exec->vm(), "");
    ASSERT(inherits(&s_info));
    putDirect(exec->vm(), exec->propertyNames().name, jsNontrivialString(exec, String(ASCIILiteral("Error"))), DontEnum);
}

bool ErrorPrototype::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<ErrorInstance>(exec, ExecState::errorPrototypeTable(exec), jsCast<ErrorPrototype*>(cell), propertyName, slot);
}

bool ErrorPrototype::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<ErrorInstance>(exec, ExecState::errorPrototypeTable(exec), jsCast<ErrorPrototype*>(object), propertyName, descriptor);
}

// ------------------------------ Functions ---------------------------

// ECMA-262 5.1, 15.11.4.4
EncodedJSValue JSC_HOST_CALL errorProtoFuncToString(ExecState* exec)
{
    // 1. Let O be the this value.
    JSValue thisValue = exec->hostThisValue();

    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisValue.isObject())
        return throwVMTypeError(exec);
    JSObject* thisObj = asObject(thisValue);

    // Guard against recursion!
    StringRecursionChecker checker(exec, thisObj);
    if (JSValue earlyReturnValue = checker.earlyReturnValue())
        return JSValue::encode(earlyReturnValue);

    // 3. Let name be the result of calling the [[Get]] internal method of O with argument "name".
    JSValue name = thisObj->get(exec, exec->propertyNames().name);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    // 4. If name is undefined, then let name be "Error"; else let name be ToString(name).
    String nameString;
    if (name.isUndefined())
        nameString = ASCIILiteral("Error");
    else {
        nameString = name.toString(exec)->value(exec);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
    }

    // 5. Let msg be the result of calling the [[Get]] internal method of O with argument "message".
    JSValue message = thisObj->get(exec, exec->propertyNames().message);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    // (sic)
    // 6. If msg is undefined, then let msg be the empty String; else let msg be ToString(msg).
    // 7. If msg is undefined, then let msg be the empty String; else let msg be ToString(msg).
    String messageString;
    if (message.isUndefined())
        messageString = String();
    else {
        messageString = message.toString(exec)->value(exec);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
    }

    // 8. If name is the empty String, return msg.
    if (!nameString.length())
        return JSValue::encode(message.isString() ? message : jsString(exec, messageString));

    // 9. If msg is the empty String, return name.
    if (!messageString.length())
        return JSValue::encode(name.isString() ? name : jsNontrivialString(exec, nameString));

    // 10. Return the result of concatenating name, ":", a single space character, and msg.
    return JSValue::encode(jsMakeNontrivialString(exec, nameString, ": ", messageString));
}

} // namespace JSC
