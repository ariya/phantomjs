/*
 *  Copyright (C) 1999-2000,2003 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008, 2011 Apple Inc. All rights reserved.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "NumberConstructor.h"

#include "Lookup.h"
#include "NumberObject.h"
#include "NumberPrototype.h"
#include "Operations.h"

namespace JSC {

static JSValue numberConstructorNaNValue(ExecState*, JSValue, PropertyName);
static JSValue numberConstructorNegInfinity(ExecState*, JSValue, PropertyName);
static JSValue numberConstructorPosInfinity(ExecState*, JSValue, PropertyName);
static JSValue numberConstructorMaxValue(ExecState*, JSValue, PropertyName);
static JSValue numberConstructorMinValue(ExecState*, JSValue, PropertyName);

} // namespace JSC

#include "NumberConstructor.lut.h"

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(NumberConstructor);

const ClassInfo NumberConstructor::s_info = { "Function", &InternalFunction::s_info, 0, ExecState::numberConstructorTable, CREATE_METHOD_TABLE(NumberConstructor) };

/* Source for NumberConstructor.lut.h
@begin numberConstructorTable
   NaN                   numberConstructorNaNValue       DontEnum|DontDelete|ReadOnly
   NEGATIVE_INFINITY     numberConstructorNegInfinity    DontEnum|DontDelete|ReadOnly
   POSITIVE_INFINITY     numberConstructorPosInfinity    DontEnum|DontDelete|ReadOnly
   MAX_VALUE             numberConstructorMaxValue       DontEnum|DontDelete|ReadOnly
   MIN_VALUE             numberConstructorMinValue       DontEnum|DontDelete|ReadOnly
@end
*/

NumberConstructor::NumberConstructor(JSGlobalObject* globalObject, Structure* structure)
    : InternalFunction(globalObject, structure) 
{
}

void NumberConstructor::finishCreation(ExecState* exec, NumberPrototype* numberPrototype)
{
    Base::finishCreation(exec->vm(), numberPrototype->s_info.className);
    ASSERT(inherits(&s_info));

    // Number.Prototype
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().prototype, numberPrototype, DontEnum | DontDelete | ReadOnly);

    // no. of arguments for constructor
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().length, jsNumber(1), ReadOnly | DontEnum | DontDelete);
}

bool NumberConstructor::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<NumberConstructor, InternalFunction>(exec, ExecState::numberConstructorTable(exec), jsCast<NumberConstructor*>(cell), propertyName, slot);
}

bool NumberConstructor::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    return getStaticValueDescriptor<NumberConstructor, InternalFunction>(exec, ExecState::numberConstructorTable(exec), jsCast<NumberConstructor*>(object), propertyName, descriptor);
}

void NumberConstructor::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    lookupPut<NumberConstructor, InternalFunction>(exec, propertyName, value, ExecState::numberConstructorTable(exec), jsCast<NumberConstructor*>(cell), slot);
}

static JSValue numberConstructorNaNValue(ExecState*, JSValue, PropertyName)
{
    return jsNaN();
}

static JSValue numberConstructorNegInfinity(ExecState*, JSValue, PropertyName)
{
    return jsNumber(-std::numeric_limits<double>::infinity());
}

static JSValue numberConstructorPosInfinity(ExecState*, JSValue, PropertyName)
{
    return jsNumber(std::numeric_limits<double>::infinity());
}

static JSValue numberConstructorMaxValue(ExecState*, JSValue, PropertyName)
{
    return jsNumber(1.7976931348623157E+308);
}

static JSValue numberConstructorMinValue(ExecState*, JSValue, PropertyName)
{
    return jsNumber(5E-324);
}

// ECMA 15.7.1
static EncodedJSValue JSC_HOST_CALL constructWithNumberConstructor(ExecState* exec)
{
    NumberObject* object = NumberObject::create(exec->vm(), asInternalFunction(exec->callee())->globalObject()->numberObjectStructure());
    double n = exec->argumentCount() ? exec->argument(0).toNumber(exec) : 0;
    object->setInternalValue(exec->vm(), jsNumber(n));
    return JSValue::encode(object);
}

ConstructType NumberConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructWithNumberConstructor;
    return ConstructTypeHost;
}

// ECMA 15.7.2
static EncodedJSValue JSC_HOST_CALL callNumberConstructor(ExecState* exec)
{
    return JSValue::encode(jsNumber(!exec->argumentCount() ? 0 : exec->argument(0).toNumber(exec)));
}

CallType NumberConstructor::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callNumberConstructor;
    return CallTypeHost;
}

} // namespace JSC
