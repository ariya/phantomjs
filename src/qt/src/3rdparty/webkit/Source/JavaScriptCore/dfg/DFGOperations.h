/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef DFGOperations_h
#define DFGOperations_h

#if ENABLE(DFG_JIT)

#include <dfg/DFGJITCompiler.h>

namespace JSC {

class Identifier;

namespace DFG {

// These typedefs provide typechecking when generating calls out to helper routines;
// this helps prevent calling a helper routine with the wrong arguments!
typedef EncodedJSValue (*J_DFGOperation_EJJ)(ExecState*, EncodedJSValue, EncodedJSValue);
typedef EncodedJSValue (*J_DFGOperation_EJ)(ExecState*, EncodedJSValue);
typedef EncodedJSValue (*J_DFGOperation_EJP)(ExecState*, EncodedJSValue, void*);
typedef EncodedJSValue (*J_DFGOperation_EJI)(ExecState*, EncodedJSValue, Identifier*);
typedef bool (*Z_DFGOperation_EJ)(ExecState*, EncodedJSValue);
typedef bool (*Z_DFGOperation_EJJ)(ExecState*, EncodedJSValue, EncodedJSValue);
typedef void (*V_DFGOperation_EJJJ)(ExecState*, EncodedJSValue, EncodedJSValue, EncodedJSValue);
typedef void (*V_DFGOperation_EJJP)(ExecState*, EncodedJSValue, EncodedJSValue, void*);
typedef void (*V_DFGOperation_EJJI)(ExecState*, EncodedJSValue, EncodedJSValue, Identifier*);
typedef double (*D_DFGOperation_DD)(double, double);

// These routines are provide callbacks out to C++ implementations of operations too complex to JIT.
EncodedJSValue operationConvertThis(ExecState*, EncodedJSValue encodedOp1);
EncodedJSValue operationValueAdd(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
EncodedJSValue operationGetByVal(ExecState*, EncodedJSValue encodedBase, EncodedJSValue encodedProperty);
EncodedJSValue operationGetById(ExecState*, EncodedJSValue encodedBase, Identifier*);
void operationPutByValStrict(ExecState*, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue);
void operationPutByValNonStrict(ExecState*, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue);
void operationPutByIdStrict(ExecState*, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier*);
void operationPutByIdNonStrict(ExecState*, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier*);
void operationPutByIdDirectStrict(ExecState*, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier*);
void operationPutByIdDirectNonStrict(ExecState*, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier*);
bool operationCompareLess(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
bool operationCompareLessEq(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
bool operationCompareEq(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
bool operationCompareStrictEq(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);

// This method is used to lookup an exception hander, keyed by faultLocation, which is
// the return location from one of the calls out to one of the helper operations above.
struct DFGHandler {
    DFGHandler(ExecState* exec, void* handler)
        : exec(exec)
        , handler(handler)
    {
    }

    ExecState* exec;
    void* handler;
};
DFGHandler lookupExceptionHandler(ExecState*, ReturnAddressPtr faultLocation);

// These operations implement the implicitly called ToInt32, ToNumber, and ToBoolean conversions from ES5.
double dfgConvertJSValueToNumber(ExecState*, EncodedJSValue);
int32_t dfgConvertJSValueToInt32(ExecState*, EncodedJSValue);
bool dfgConvertJSValueToBoolean(ExecState*, EncodedJSValue);

} } // namespace JSC::DFG

#endif
#endif
