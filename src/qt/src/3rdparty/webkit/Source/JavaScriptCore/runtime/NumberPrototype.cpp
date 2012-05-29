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
#include "NumberPrototype.h"

#include "Error.h"
#include "JSFunction.h"
#include "JSString.h"
#include "Operations.h"
#include "dtoa.h"
#include <wtf/Assertions.h>
#include <wtf/DecimalNumber.h>
#include <wtf/MathExtras.h>
#include <wtf/Vector.h>

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(NumberPrototype);

static EncodedJSValue JSC_HOST_CALL numberProtoFuncToString(ExecState*);
static EncodedJSValue JSC_HOST_CALL numberProtoFuncToLocaleString(ExecState*);
static EncodedJSValue JSC_HOST_CALL numberProtoFuncValueOf(ExecState*);
static EncodedJSValue JSC_HOST_CALL numberProtoFuncToFixed(ExecState*);
static EncodedJSValue JSC_HOST_CALL numberProtoFuncToExponential(ExecState*);
static EncodedJSValue JSC_HOST_CALL numberProtoFuncToPrecision(ExecState*);

// ECMA 15.7.4

NumberPrototype::NumberPrototype(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, Structure* functionStructure)
    : NumberObject(exec->globalData(), structure)
{
    setInternalValue(exec->globalData(), jsNumber(0));

    // The constructor will be added later, after NumberConstructor has been constructed

    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 1, exec->propertyNames().toString, numberProtoFuncToString), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 0, exec->propertyNames().toLocaleString, numberProtoFuncToLocaleString), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 0, exec->propertyNames().valueOf, numberProtoFuncValueOf), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 1, exec->propertyNames().toFixed, numberProtoFuncToFixed), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 1, exec->propertyNames().toExponential, numberProtoFuncToExponential), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 1, exec->propertyNames().toPrecision, numberProtoFuncToPrecision), DontEnum);
}

// ------------------------------ Functions ---------------------------

// ECMA 15.7.4.2 - 15.7.4.7

static ALWAYS_INLINE bool toThisNumber(JSValue thisValue, double &x)
{
    JSValue v = thisValue.getJSNumber();
    if (UNLIKELY(!v))
        return false;
    x = v.uncheckedGetNumber();
    return true;
}

static ALWAYS_INLINE bool getIntegerArgumentInRange(ExecState* exec, int low, int high, int& result, bool& isUndefined)
{
    result = 0;
    isUndefined = false;

    JSValue argument0 = exec->argument(0);
    if (argument0.isUndefined()) {
        isUndefined = true;
        return true;
    }

    double asDouble = argument0.toInteger(exec);
    if (asDouble < low || asDouble > high)
        return false;

    result = static_cast<int>(asDouble);
    return true;
}

// toExponential converts a number to a string, always formatting as an expoential.
// This method takes an optional argument specifying a number of *decimal places*
// to round the significand to (or, put another way, this method optionally rounds
// to argument-plus-one significant figures).
EncodedJSValue JSC_HOST_CALL numberProtoFuncToExponential(ExecState* exec)
{
    // Get x (the double value of this, which should be a Number).
    double x;
    if (!toThisNumber(exec->hostThisValue(), x))
        return throwVMTypeError(exec);

    // Get the argument. 
    int decimalPlacesInExponent;
    bool isUndefined;
    if (!getIntegerArgumentInRange(exec, 0, 20, decimalPlacesInExponent, isUndefined))
        return throwVMError(exec, createRangeError(exec, "toExponential() argument must be between 0 and 20"));

    // Handle NaN and Infinity.
    if (isnan(x) || isinf(x))
        return JSValue::encode(jsString(exec, UString::number(x)));

    // Round if the argument is not undefined, always format as exponential.
    NumberToStringBuffer buffer;
    unsigned length = isUndefined
        ? DecimalNumber(x).toStringExponential(buffer, WTF::NumberToStringBufferLength)
        : DecimalNumber(x, RoundingSignificantFigures, decimalPlacesInExponent + 1).toStringExponential(buffer, WTF::NumberToStringBufferLength);

    return JSValue::encode(jsString(exec, UString(buffer, length)));
}

// toFixed converts a number to a string, always formatting as an a decimal fraction.
// This method takes an argument specifying a number of decimal places to round the
// significand to. However when converting large values (1e+21 and above) this
// method will instead fallback to calling ToString. 
EncodedJSValue JSC_HOST_CALL numberProtoFuncToFixed(ExecState* exec)
{
    // Get x (the double value of this, which should be a Number).
    JSValue thisValue = exec->hostThisValue();
    JSValue v = thisValue.getJSNumber();
    if (!v)
        return throwVMTypeError(exec);
    double x = v.uncheckedGetNumber();

    // Get the argument. 
    int decimalPlaces;
    bool isUndefined; // This is ignored; undefined treated as 0.
    if (!getIntegerArgumentInRange(exec, 0, 20, decimalPlaces, isUndefined))
        return throwVMError(exec, createRangeError(exec, "toFixed() argument must be between 0 and 20"));

    // 15.7.4.5.7 states "If x >= 10^21, then let m = ToString(x)"
    // This also covers Ininity, and structure the check so that NaN
    // values are also handled by numberToString
    if (!(fabs(x) < 1e+21))
        return JSValue::encode(jsString(exec, UString::number(x)));

    // The check above will return false for NaN or Infinity, these will be
    // handled by numberToString.
    ASSERT(!isnan(x) && !isinf(x));

    // Convert to decimal with rounding, and format as decimal.
    NumberToStringBuffer buffer;
    unsigned length = DecimalNumber(x, RoundingDecimalPlaces, decimalPlaces).toStringDecimal(buffer, WTF::NumberToStringBufferLength);
    return JSValue::encode(jsString(exec, UString(buffer, length)));
}

// toPrecision converts a number to a string, takeing an argument specifying a
// number of significant figures to round the significand to. For positive
// exponent, all values that can be represented using a decimal fraction will
// be, e.g. when rounding to 3 s.f. any value up to 999 will be formated as a
// decimal, whilst 1000 is converted to the exponential representation 1.00e+3.
// For negative exponents values >= 1e-6 are formated as decimal fractions,
// with smaller values converted to exponential representation.
EncodedJSValue JSC_HOST_CALL numberProtoFuncToPrecision(ExecState* exec)
{
    // Get x (the double value of this, which should be a Number).
    JSValue thisValue = exec->hostThisValue();
    JSValue v = thisValue.getJSNumber();
    if (!v)
        return throwVMTypeError(exec);
    double x = v.uncheckedGetNumber();

    // Get the argument. 
    int significantFigures;
    bool isUndefined;
    if (!getIntegerArgumentInRange(exec, 1, 21, significantFigures, isUndefined))
        return throwVMError(exec, createRangeError(exec, "toPrecision() argument must be between 1 and 21"));

    // To precision called with no argument is treated as ToString.
    if (isUndefined)
        return JSValue::encode(jsString(exec, UString::number(x)));

    // Handle NaN and Infinity.
    if (isnan(x) || isinf(x))
        return JSValue::encode(jsString(exec, UString::number(x)));

    // Convert to decimal with rounding.
    DecimalNumber number(x, RoundingSignificantFigures, significantFigures);
    // If number is in the range 1e-6 <= x < pow(10, significantFigures) then format
    // as decimal. Otherwise, format the number as an exponential.  Decimal format
    // demands a minimum of (exponent + 1) digits to represent a number, for example
    // 1234 (1.234e+3) requires 4 digits. (See ECMA-262 15.7.4.7.10.c)
    NumberToStringBuffer buffer;
    unsigned length = number.exponent() >= -6 && number.exponent() < significantFigures
        ? number.toStringDecimal(buffer, WTF::NumberToStringBufferLength)
        : number.toStringExponential(buffer, WTF::NumberToStringBufferLength);
    return JSValue::encode(jsString(exec, UString(buffer, length)));
}

EncodedJSValue JSC_HOST_CALL numberProtoFuncToString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    JSValue v = thisValue.getJSNumber();
    if (!v)
        return throwVMTypeError(exec);

    JSValue radixValue = exec->argument(0);
    int radix;
    if (radixValue.isInt32())
        radix = radixValue.asInt32();
    else if (radixValue.isUndefined())
        radix = 10;
    else
        radix = static_cast<int>(radixValue.toInteger(exec)); // nan -> 0

    if (radix == 10)
        return JSValue::encode(jsString(exec, v.toString(exec)));

    static const char* const digits = "0123456789abcdefghijklmnopqrstuvwxyz";

    // Fast path for number to character conversion.
    if (radix == 36) {
        if (v.isInt32()) {
            int x = v.asInt32();
            if (static_cast<unsigned>(x) < 36) { // Exclude negatives
                JSGlobalData* globalData = &exec->globalData();
                return JSValue::encode(globalData->smallStrings.singleCharacterString(globalData, digits[x]));
            }
        }
    }

    if (radix < 2 || radix > 36)
        return throwVMError(exec, createRangeError(exec, "toString() radix argument must be between 2 and 36"));

    // INT_MAX results in 1024 characters left of the dot with radix 2
    // give the same space on the right side. safety checks are in place
    // unless someone finds a precise rule.
    char s[2048 + 3];
    const char* lastCharInString = s + sizeof(s) - 1;
    double x = v.uncheckedGetNumber();
    if (isnan(x) || isinf(x))
        return JSValue::encode(jsString(exec, UString::number(x)));

    bool isNegative = x < 0.0;
    if (isNegative)
        x = -x;

    double integerPart = floor(x);
    char* decimalPoint = s + sizeof(s) / 2;

    // convert integer portion
    char* p = decimalPoint;
    double d = integerPart;
    do {
        int remainderDigit = static_cast<int>(fmod(d, radix));
        *--p = digits[remainderDigit];
        d /= radix;
    } while ((d <= -1.0 || d >= 1.0) && s < p);

    if (isNegative)
        *--p = '-';
    char* startOfResultString = p;
    ASSERT(s <= startOfResultString);

    d = x - integerPart;
    p = decimalPoint;
    const double epsilon = 0.001; // TODO: guessed. base on radix ?
    bool hasFractionalPart = (d < -epsilon || d > epsilon);
    if (hasFractionalPart) {
        *p++ = '.';
        do {
            d *= radix;
            const int digit = static_cast<int>(d);
            *p++ = digits[digit];
            d -= digit;
        } while ((d < -epsilon || d > epsilon) && p < lastCharInString);
    }
    *p = '\0';
    ASSERT(p < s + sizeof(s));

    return JSValue::encode(jsString(exec, startOfResultString));
}

EncodedJSValue JSC_HOST_CALL numberProtoFuncToLocaleString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    // FIXME: Not implemented yet.

    JSValue v = thisValue.getJSNumber();
    if (!v)
        return throwVMTypeError(exec);

    return JSValue::encode(jsString(exec, v.toString(exec)));
}

EncodedJSValue JSC_HOST_CALL numberProtoFuncValueOf(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    JSValue v = thisValue.getJSNumber();
    if (!v)
        return throwVMTypeError(exec);

    return JSValue::encode(v);
}

} // namespace JSC
