/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 2003 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
#include "ArrayConstructor.h"

#include "ArrayPrototype.h"
#include "Error.h"
#include "ExceptionHelpers.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "Lookup.h"

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(ArrayConstructor);
    
static EncodedJSValue JSC_HOST_CALL arrayConstructorIsArray(ExecState*);

ArrayConstructor::ArrayConstructor(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, ArrayPrototype* arrayPrototype, Structure* functionStructure)
    : InternalFunction(&exec->globalData(), globalObject, structure, Identifier(exec, arrayPrototype->classInfo()->className))
{
    // ECMA 15.4.3.1 Array.prototype
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().prototype, arrayPrototype, DontEnum | DontDelete | ReadOnly);

    // no. of arguments for constructor
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(1), ReadOnly | DontEnum | DontDelete);

    // ES5
    putDirectFunctionWithoutTransition(exec, new (exec) JSFunction(exec, globalObject, functionStructure, 1, exec->propertyNames().isArray, arrayConstructorIsArray), DontEnum);
}

static inline JSObject* constructArrayWithSizeQuirk(ExecState* exec, const ArgList& args)
{
    JSGlobalObject* globalObject = asInternalFunction(exec->callee())->globalObject();

    // a single numeric argument denotes the array size (!)
    if (args.size() == 1 && args.at(0).isNumber()) {
        uint32_t n = args.at(0).toUInt32(exec);
        if (n != args.at(0).toNumber(exec))
            return throwError(exec, createRangeError(exec, "Array size is not a small enough positive integer."));
        return new (exec) JSArray(exec->globalData(), globalObject->arrayStructure(), n, CreateInitialized);
    }

    // otherwise the array is constructed with the arguments in it
    return new (exec) JSArray(exec->globalData(), globalObject->arrayStructure(), args);
}

static EncodedJSValue JSC_HOST_CALL constructWithArrayConstructor(ExecState* exec)
{
    ArgList args(exec);
    return JSValue::encode(constructArrayWithSizeQuirk(exec, args));
}

// ECMA 15.4.2
ConstructType ArrayConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructWithArrayConstructor;
    return ConstructTypeHost;
}

static EncodedJSValue JSC_HOST_CALL callArrayConstructor(ExecState* exec)
{
    ArgList args(exec);
    return JSValue::encode(constructArrayWithSizeQuirk(exec, args));
}

// ECMA 15.6.1
CallType ArrayConstructor::getCallData(CallData& callData)
{
    // equivalent to 'new Array(....)'
    callData.native.function = callArrayConstructor;
    return CallTypeHost;
}

EncodedJSValue JSC_HOST_CALL arrayConstructorIsArray(ExecState* exec)
{
    return JSValue::encode(jsBoolean(exec->argument(0).inherits(&JSArray::s_info)));
}

} // namespace JSC
