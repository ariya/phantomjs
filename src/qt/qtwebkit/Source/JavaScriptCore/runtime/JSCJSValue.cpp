/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2007, 2008, 2012 Apple Inc. All rights reserved.
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
#include "JSCJSValue.h"

#include "BooleanConstructor.h"
#include "BooleanPrototype.h"
#include "Error.h"
#include "ExceptionHelpers.h"
#include "GetterSetter.h"
#include "JSCJSValueInlines.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "JSNotAnObject.h"
#include "NumberObject.h"
#include <wtf/MathExtras.h>
#include <wtf/StringExtras.h>

namespace JSC {

static const double D32 = 4294967296.0;

// ECMA 9.4
double JSValue::toInteger(ExecState* exec) const
{
    if (isInt32())
        return asInt32();
    double d = toNumber(exec);
    return std::isnan(d) ? 0.0 : trunc(d);
}

double JSValue::toIntegerPreserveNaN(ExecState* exec) const
{
    if (isInt32())
        return asInt32();
    return trunc(toNumber(exec));
}

double JSValue::toNumberSlowCase(ExecState* exec) const
{
    ASSERT(!isInt32() && !isDouble());
    if (isCell())
        return asCell()->toNumber(exec);
    if (isTrue())
        return 1.0;
    return isUndefined() ? QNaN : 0; // null and false both convert to 0.
}

JSObject* JSValue::toObjectSlowCase(ExecState* exec, JSGlobalObject* globalObject) const
{
    ASSERT(!isCell());

    if (isInt32() || isDouble())
        return constructNumber(exec, globalObject, asValue());
    if (isTrue() || isFalse())
        return constructBooleanFromImmediateBoolean(exec, globalObject, asValue());

    ASSERT(isUndefinedOrNull());
    throwError(exec, createNotAnObjectError(exec, *this));
    return JSNotAnObject::create(exec);
}

JSObject* JSValue::toThisObjectSlowCase(ExecState* exec) const
{
    ASSERT(!isCell());

    if (isInt32() || isDouble())
        return constructNumber(exec, exec->lexicalGlobalObject(), asValue());
    if (isTrue() || isFalse())
        return constructBooleanFromImmediateBoolean(exec, exec->lexicalGlobalObject(), asValue());
    ASSERT(isUndefinedOrNull());
    return exec->globalThisValue();
}

JSObject* JSValue::synthesizePrototype(ExecState* exec) const
{
    if (isCell()) {
        ASSERT(isString());
        return exec->lexicalGlobalObject()->stringPrototype();
    }

    if (isNumber())
        return exec->lexicalGlobalObject()->numberPrototype();
    if (isBoolean())
        return exec->lexicalGlobalObject()->booleanPrototype();

    ASSERT(isUndefinedOrNull());
    throwError(exec, createNotAnObjectError(exec, *this));
    return JSNotAnObject::create(exec);
}

// ECMA 8.7.2
void JSValue::putToPrimitive(ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    VM& vm = exec->vm();

    unsigned index = propertyName.asIndex();
    if (index != PropertyName::NotAnIndex) {
        putToPrimitiveByIndex(exec, index, value, slot.isStrictMode());
        return;
    }

    // Check if there are any setters or getters in the prototype chain
    JSObject* obj = synthesizePrototype(exec);
    JSValue prototype;
    if (propertyName != exec->propertyNames().underscoreProto) {
        for (; !obj->structure()->hasReadOnlyOrGetterSetterPropertiesExcludingProto(); obj = asObject(prototype)) {
            prototype = obj->prototype();
            if (prototype.isNull()) {
                if (slot.isStrictMode())
                    throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
                return;
            }
        }
    }

    for (; ; obj = asObject(prototype)) {
        unsigned attributes;
        JSCell* specificValue;
        PropertyOffset offset = obj->structure()->get(vm, propertyName, attributes, specificValue);
        if (offset != invalidOffset) {
            if (attributes & ReadOnly) {
                if (slot.isStrictMode())
                    throwError(exec, createTypeError(exec, StrictModeReadonlyPropertyWriteError));
                return;
            }

            JSValue gs = obj->getDirect(offset);
            if (gs.isGetterSetter()) {
                JSObject* setterFunc = asGetterSetter(gs)->setter();        
                if (!setterFunc) {
                    if (slot.isStrictMode())
                        throwError(exec, createTypeError(exec, ASCIILiteral("setting a property that has only a getter")));
                    return;
                }
                
                CallData callData;
                CallType callType = setterFunc->methodTable()->getCallData(setterFunc, callData);
                MarkedArgumentBuffer args;
                args.append(value);

                // If this is WebCore's global object then we need to substitute the shell.
                call(exec, setterFunc, callType, callData, *this, args);
                return;
            }

            // If there's an existing property on the object or one of its 
            // prototypes it should be replaced, so break here.
            break;
        }

        prototype = obj->prototype();
        if (prototype.isNull())
            break;
    }
    
    if (slot.isStrictMode())
        throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
    return;
}

void JSValue::putToPrimitiveByIndex(ExecState* exec, unsigned propertyName, JSValue value, bool shouldThrow)
{
    if (propertyName > MAX_ARRAY_INDEX) {
        PutPropertySlot slot(shouldThrow);
        putToPrimitive(exec, Identifier::from(exec, propertyName), value, slot);
        return;
    }
    
    if (synthesizePrototype(exec)->attemptToInterceptPutByIndexOnHoleForPrototype(exec, *this, propertyName, value, shouldThrow))
        return;
    
    if (shouldThrow)
        throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
}

void JSValue::dump(PrintStream& out) const
{
    if (!*this)
        out.print("<JSValue()>");
    else if (isInt32())
        out.printf("Int32: %d", asInt32());
    else if (isDouble()) {
#if USE(JSVALUE64)
        out.printf("Double: %lld, %lf", (long long)reinterpretDoubleToInt64(asDouble()), asDouble());
#else
        union {
            double asDouble;
            uint32_t asTwoInt32s[2];
        } u;
        u.asDouble = asDouble();
        out.printf("Double: %08x:%08x, %lf", u.asTwoInt32s[1], u.asTwoInt32s[0], asDouble());
#endif
    } else if (isCell()) {
        if (asCell()->inherits(&JSString::s_info)) {
            JSString* string = jsCast<JSString*>(asCell());
            out.print("String: ");
            if (string->isRope())
                out.print("(rope) ");
            out.print(string->tryGetValue());
        } else if (asCell()->inherits(&Structure::s_info)) {
            Structure* structure = jsCast<Structure*>(asCell());
            out.print(
                "Structure: ", RawPointer(structure), ": ", structure->classInfo()->className,
                ", ", IndexingTypeDump(structure->indexingTypeIncludingHistory()));
        } else {
            out.print("Cell: ", RawPointer(asCell()));
            if (isObject() && asObject(*this)->butterfly())
                out.print("->", RawPointer(asObject(*this)->butterfly()));
            out.print(
                " (", RawPointer(asCell()->structure()), ": ", asCell()->structure()->classInfo()->className,
                ", ", IndexingTypeDump(asCell()->structure()->indexingTypeIncludingHistory()), ")");
        }
    } else if (isTrue())
        out.print("True");
    else if (isFalse())
        out.print("False");
    else if (isNull())
        out.print("Null");
    else if (isUndefined())
        out.print("Undefined");
    else
        out.print("INVALID");
}

// This in the ToInt32 operation is defined in section 9.5 of the ECMA-262 spec.
// Note that this operation is identical to ToUInt32 other than to interpretation
// of the resulting bit-pattern (as such this metod is also called to implement
// ToUInt32).
//
// The operation can be descibed as round towards zero, then select the 32 least
// bits of the resulting value in 2s-complement representation.
int32_t toInt32(double number)
{
    int64_t bits = WTF::bitwise_cast<int64_t>(number);
    int32_t exp = (static_cast<int32_t>(bits >> 52) & 0x7ff) - 0x3ff;

    // If exponent < 0 there will be no bits to the left of the decimal point
    // after rounding; if the exponent is > 83 then no bits of precision can be
    // left in the low 32-bit range of the result (IEEE-754 doubles have 52 bits
    // of fractional precision).
    // Note this case handles 0, -0, and all infinte, NaN, & denormal value. 
    if (exp < 0 || exp > 83)
        return 0;

    // Select the appropriate 32-bits from the floating point mantissa.  If the
    // exponent is 52 then the bits we need to select are already aligned to the
    // lowest bits of the 64-bit integer representation of tghe number, no need
    // to shift.  If the exponent is greater than 52 we need to shift the value
    // left by (exp - 52), if the value is less than 52 we need to shift right
    // accordingly.
    int32_t result = (exp > 52)
        ? static_cast<int32_t>(bits << (exp - 52))
        : static_cast<int32_t>(bits >> (52 - exp));

    // IEEE-754 double precision values are stored omitting an implicit 1 before
    // the decimal point; we need to reinsert this now.  We may also the shifted
    // invalid bits into the result that are not a part of the mantissa (the sign
    // and exponent bits from the floatingpoint representation); mask these out.
    if (exp < 32) {
        int32_t missingOne = 1 << exp;
        result &= missingOne - 1;
        result += missingOne;
    }

    // If the input value was negative (we could test either 'number' or 'bits',
    // but testing 'bits' is likely faster) invert the result appropriately.
    return bits < 0 ? -result : result;
}

bool JSValue::isValidCallee()
{
    return asObject(asCell())->globalObject();
}

JSString* JSValue::toStringSlowCase(ExecState* exec) const
{
    VM& vm = exec->vm();
    ASSERT(!isString());
    if (isInt32())
        return jsString(&vm, vm.numericStrings.add(asInt32()));
    if (isDouble())
        return jsString(&vm, vm.numericStrings.add(asDouble()));
    if (isTrue())
        return vm.smallStrings.trueString();
    if (isFalse())
        return vm.smallStrings.falseString();
    if (isNull())
        return vm.smallStrings.nullString();
    if (isUndefined())
        return vm.smallStrings.undefinedString();

    ASSERT(isCell());
    JSValue value = asCell()->toPrimitive(exec, PreferString);
    if (exec->hadException())
        return jsEmptyString(exec);
    ASSERT(!value.isObject());
    return value.toString(exec);
}

String JSValue::toWTFStringSlowCase(ExecState* exec) const
{
    return inlineJSValueNotStringtoString(*this, exec);
}

} // namespace JSC
