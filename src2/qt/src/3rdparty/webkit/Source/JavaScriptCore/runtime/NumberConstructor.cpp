/*
 *  Copyright (C) 1999-2000,2003 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(NumberConstructor);

static JSValue numberConstructorNaNValue(ExecState*, JSValue, const Identifier&);
static JSValue numberConstructorNegInfinity(ExecState*, JSValue, const Identifier&);
static JSValue numberConstructorPosInfinity(ExecState*, JSValue, const Identifier&);
static JSValue numberConstructorMaxValue(ExecState*, JSValue, const Identifier&);
static JSValue numberConstructorMinValue(ExecState*, JSValue, const Identifier&);

} // namespace JSC

#include "NumberConstructor.lut.h"

namespace JSC {

const ClassInfo NumberConstructor::s_info = { "Function", &InternalFunction::s_info, 0, ExecState::numberTable };

/* Source for NumberConstructor.lut.h
@begin numberTable
   NaN                   numberConstructorNaNValue       DontEnum|DontDelete|ReadOnly
   NEGATIVE_INFINITY     numberConstructorNegInfinity    DontEnum|DontDelete|ReadOnly
   POSITIVE_INFINITY     numberConstructorPosInfinity    DontEnum|DontDelete|ReadOnly
   MAX_VALUE             numberConstructorMaxValue       DontEnum|DontDelete|ReadOnly
   MIN_VALUE             numberConstructorMinValue       DontEnum|DontDelete|ReadOnly
@end
*/

NumberConstructor::NumberConstructor(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, NumberPrototype* numberPrototype)
    : InternalFunction(&exec->globalData(), globalObject, structure, Identifier(exec, numberPrototype->s_info.className))
{
    ASSERT(inherits(&s_info));

    // Number.Prototype
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().prototype, numberPrototype, DontEnum | DontDelete | ReadOnly);

    // no. of arguments for constructor
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(1), ReadOnly | DontEnum | DontDelete);
}

bool NumberConstructor::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<NumberConstructor, InternalFunction>(exec, ExecState::numberTable(exec), this, propertyName, slot);
}

bool NumberConstructor::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticValueDescriptor<NumberConstructor, InternalFunction>(exec, ExecState::numberTable(exec), this, propertyName, descriptor);
}

static JSValue numberConstructorNaNValue(ExecState*, JSValue, const Identifier&)
{
    return jsNaN();
}

static JSValue numberConstructorNegInfinity(ExecState*, JSValue, const Identifier&)
{
    return jsNumber(-Inf);
}

static JSValue numberConstructorPosInfinity(ExecState*, JSValue, const Identifier&)
{
    return jsNumber(Inf);
}

static JSValue numberConstructorMaxValue(ExecState*, JSValue, const Identifier&)
{
    return jsNumber(1.7976931348623157E+308);
}

static JSValue numberConstructorMinValue(ExecState*, JSValue, const Identifier&)
{
    return jsNumber(5E-324);
}

// ECMA 15.7.1
static EncodedJSValue JSC_HOST_CALL constructWithNumberConstructor(ExecState* exec)
{
    NumberObject* object = new (exec) NumberObject(exec->globalData(), asInternalFunction(exec->callee())->globalObject()->numberObjectStructure());
    double n = exec->argumentCount() ? exec->argument(0).toNumber(exec) : 0;
    object->setInternalValue(exec->globalData(), jsNumber(n));
    return JSValue::encode(object);
}

ConstructType NumberConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructWithNumberConstructor;
    return ConstructTypeHost;
}

// ECMA 15.7.2
static EncodedJSValue JSC_HOST_CALL callNumberConstructor(ExecState* exec)
{
    return JSValue::encode(jsNumber(!exec->argumentCount() ? 0 : exec->argument(0).toNumber(exec)));
}

CallType NumberConstructor::getCallData(CallData& callData)
{
    callData.native.function = callNumberConstructor;
    return CallTypeHost;
}

} // namespace JSC
