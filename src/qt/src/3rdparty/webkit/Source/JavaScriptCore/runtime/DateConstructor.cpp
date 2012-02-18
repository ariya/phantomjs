/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "DateConstructor.h"

#include "DateConversion.h"
#include "DateInstance.h"
#include "DatePrototype.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "JSString.h"
#include "JSStringBuilder.h"
#include "ObjectPrototype.h"
#include <math.h>
#include <time.h>
#include <wtf/DateMath.h>
#include <wtf/MathExtras.h>

#if OS(WINCE) && !PLATFORM(QT)
extern "C" time_t time(time_t* timer); // Provided by libce.
#endif

#if HAVE(SYS_TIME_H)
#include <sys/time.h>
#endif

#if HAVE(SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

using namespace WTF;

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(DateConstructor);

static EncodedJSValue JSC_HOST_CALL dateParse(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateNow(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateUTC(ExecState*);

DateConstructor::DateConstructor(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, Structure* functionStructure, DatePrototype* datePrototype)
    : InternalFunction(&exec->globalData(), globalObject, structure, Identifier(exec, datePrototype->classInfo()->className))
{
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().prototype, datePrototype, DontEnum | DontDelete | ReadOnly);

    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 1, exec->propertyNames().parse, dateParse), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 7, exec->propertyNames().UTC, dateUTC), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 0, exec->propertyNames().now, dateNow), DontEnum);

    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(7), ReadOnly | DontEnum | DontDelete);
}

// ECMA 15.9.3
JSObject* constructDate(ExecState* exec, JSGlobalObject* globalObject, const ArgList& args)
{
    int numArgs = args.size();

    double value;

    if (numArgs == 0) // new Date() ECMA 15.9.3.3
        value = jsCurrentTime();
    else if (numArgs == 1) {
        if (args.at(0).inherits(&DateInstance::s_info))
            value = asDateInstance(args.at(0))->internalNumber();
        else {
            JSValue primitive = args.at(0).toPrimitive(exec);
            if (primitive.isString())
                value = parseDate(exec, primitive.getString(exec));
            else
                value = primitive.toNumber(exec);
        }
    } else {
        double doubleArguments[7] = {
            args.at(0).toNumber(exec), 
            args.at(1).toNumber(exec), 
            args.at(2).toNumber(exec), 
            args.at(3).toNumber(exec), 
            args.at(4).toNumber(exec), 
            args.at(5).toNumber(exec), 
            args.at(6).toNumber(exec)
        };
        if (isnan(doubleArguments[0])
                || isnan(doubleArguments[1])
                || (numArgs >= 3 && isnan(doubleArguments[2]))
                || (numArgs >= 4 && isnan(doubleArguments[3]))
                || (numArgs >= 5 && isnan(doubleArguments[4]))
                || (numArgs >= 6 && isnan(doubleArguments[5]))
                || (numArgs >= 7 && isnan(doubleArguments[6])))
            value = NaN;
        else {
            GregorianDateTime t;
            int year = JSC::toInt32(doubleArguments[0]);
            t.year = (year >= 0 && year <= 99) ? year : year - 1900;
            t.month = JSC::toInt32(doubleArguments[1]);
            t.monthDay = (numArgs >= 3) ? JSC::toInt32(doubleArguments[2]) : 1;
            t.hour = JSC::toInt32(doubleArguments[3]);
            t.minute = JSC::toInt32(doubleArguments[4]);
            t.second = JSC::toInt32(doubleArguments[5]);
            t.isDST = -1;
            double ms = (numArgs >= 7) ? doubleArguments[6] : 0;
            value = gregorianDateTimeToMS(exec, t, ms, false);
        }
    }

    return new (exec) DateInstance(exec, globalObject->dateStructure(), value);
}
    
static EncodedJSValue JSC_HOST_CALL constructWithDateConstructor(ExecState* exec)
{
    ArgList args(exec);
    return JSValue::encode(constructDate(exec, asInternalFunction(exec->callee())->globalObject(), args));
}

ConstructType DateConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructWithDateConstructor;
    return ConstructTypeHost;
}

// ECMA 15.9.2
static EncodedJSValue JSC_HOST_CALL callDate(ExecState* exec)
{
    time_t localTime = time(0);
    tm localTM;
    getLocalTime(&localTime, &localTM);
    GregorianDateTime ts(exec, localTM);
    DateConversionBuffer date;
    DateConversionBuffer time;
    formatDate(ts, date);
    formatTime(ts, time);
    return JSValue::encode(jsMakeNontrivialString(exec, date, " ", time));
}

CallType DateConstructor::getCallData(CallData& callData)
{
    callData.native.function = callDate;
    return CallTypeHost;
}

static EncodedJSValue JSC_HOST_CALL dateParse(ExecState* exec)
{
    return JSValue::encode(jsNumber(parseDate(exec, exec->argument(0).toString(exec))));
}

static EncodedJSValue JSC_HOST_CALL dateNow(ExecState*)
{
    return JSValue::encode(jsNumber(jsCurrentTime()));
}

static EncodedJSValue JSC_HOST_CALL dateUTC(ExecState* exec) 
{
    double doubleArguments[7] = {
        exec->argument(0).toNumber(exec), 
        exec->argument(1).toNumber(exec), 
        exec->argument(2).toNumber(exec), 
        exec->argument(3).toNumber(exec), 
        exec->argument(4).toNumber(exec), 
        exec->argument(5).toNumber(exec), 
        exec->argument(6).toNumber(exec)
    };
    int n = exec->argumentCount();
    if (isnan(doubleArguments[0])
            || isnan(doubleArguments[1])
            || (n >= 3 && isnan(doubleArguments[2]))
            || (n >= 4 && isnan(doubleArguments[3]))
            || (n >= 5 && isnan(doubleArguments[4]))
            || (n >= 6 && isnan(doubleArguments[5]))
            || (n >= 7 && isnan(doubleArguments[6])))
        return JSValue::encode(jsNaN());

    GregorianDateTime t;
    int year = JSC::toInt32(doubleArguments[0]);
    t.year = (year >= 0 && year <= 99) ? year : year - 1900;
    t.month = JSC::toInt32(doubleArguments[1]);
    t.monthDay = (n >= 3) ? JSC::toInt32(doubleArguments[2]) : 1;
    t.hour = JSC::toInt32(doubleArguments[3]);
    t.minute = JSC::toInt32(doubleArguments[4]);
    t.second = JSC::toInt32(doubleArguments[5]);
    double ms = (n >= 7) ? doubleArguments[6] : 0;
    return JSValue::encode(jsNumber(timeClip(gregorianDateTimeToMS(exec, t, ms, true))));
}

} // namespace JSC
