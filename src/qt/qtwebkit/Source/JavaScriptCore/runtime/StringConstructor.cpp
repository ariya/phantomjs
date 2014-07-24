/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "StringConstructor.h"

#include "Executable.h"
#include "JITCode.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "Operations.h"
#include "StringPrototype.h"

namespace JSC {

static EncodedJSValue JSC_HOST_CALL stringFromCharCode(ExecState*);

}

#include "StringConstructor.lut.h"

namespace JSC {

const ClassInfo StringConstructor::s_info = { "Function", &InternalFunction::s_info, 0, ExecState::stringConstructorTable, CREATE_METHOD_TABLE(StringConstructor) };

/* Source for StringConstructor.lut.h
@begin stringConstructorTable
  fromCharCode          stringFromCharCode         DontEnum|Function 1
@end
*/

ASSERT_HAS_TRIVIAL_DESTRUCTOR(StringConstructor);

StringConstructor::StringConstructor(JSGlobalObject* globalObject, Structure* structure)
    : InternalFunction(globalObject, structure)
{
}

void StringConstructor::finishCreation(ExecState* exec, StringPrototype* stringPrototype)
{
    Base::finishCreation(exec->vm(), stringPrototype->classInfo()->className);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().prototype, stringPrototype, ReadOnly | DontEnum | DontDelete);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().length, jsNumber(1), ReadOnly | DontEnum | DontDelete);
}

bool StringConstructor::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<InternalFunction>(exec, ExecState::stringConstructorTable(exec), jsCast<StringConstructor*>(cell), propertyName, slot);
}

bool StringConstructor::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<InternalFunction>(exec, ExecState::stringConstructorTable(exec), jsCast<StringConstructor*>(object), propertyName, descriptor);
}

// ------------------------------ Functions --------------------------------

static NEVER_INLINE JSValue stringFromCharCodeSlowCase(ExecState* exec)
{
    unsigned length = exec->argumentCount();
    UChar* buf;
    PassRefPtr<StringImpl> impl = StringImpl::createUninitialized(length, buf);
    for (unsigned i = 0; i < length; ++i)
        buf[i] = static_cast<UChar>(exec->argument(i).toUInt32(exec));
    return jsString(exec, impl);
}

static EncodedJSValue JSC_HOST_CALL stringFromCharCode(ExecState* exec)
{
    if (LIKELY(exec->argumentCount() == 1))
        return JSValue::encode(jsSingleCharacterString(exec, exec->argument(0).toUInt32(exec)));
    return JSValue::encode(stringFromCharCodeSlowCase(exec));
}

JSCell* JSC_HOST_CALL stringFromCharCode(ExecState* exec, int32_t arg)
{
    return jsSingleCharacterString(exec, arg);
}

static EncodedJSValue JSC_HOST_CALL constructWithStringConstructor(ExecState* exec)
{
    JSGlobalObject* globalObject = asInternalFunction(exec->callee())->globalObject();
    if (!exec->argumentCount())
        return JSValue::encode(StringObject::create(exec, globalObject->stringObjectStructure()));
    
    return JSValue::encode(StringObject::create(exec, globalObject->stringObjectStructure(), exec->argument(0).toString(exec)));
}

ConstructType StringConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructWithStringConstructor;
    return ConstructTypeHost;
}

static EncodedJSValue JSC_HOST_CALL callStringConstructor(ExecState* exec)
{
    if (!exec->argumentCount())
        return JSValue::encode(jsEmptyString(exec));
    return JSValue::encode(exec->argument(0).toString(exec));
}

CallType StringConstructor::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callStringConstructor;
    return CallTypeHost;
}

} // namespace JSC
