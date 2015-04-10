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

#include "DFGJITCompiler.h"
#include "PutKind.h"

namespace JSC {

namespace DFG {

extern "C" {

#if CALLING_CONVENTION_IS_STDCALL
#define DFG_OPERATION CDECL
#else
#define DFG_OPERATION
#endif

// These typedefs provide typechecking when generating calls out to helper routines;
// this helps prevent calling a helper routine with the wrong arguments!
/*
    Key:
    V: void
    J: JSValue
    P: pointer (void*)
    C: JSCell*
    A: JSArray*
    S: size_t
    Z: int32_t
    D: double
    I: Identifier*
    G: GlobalResolveInfo*
*/
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_E)(ExecState*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EA)(ExecState*, JSArray*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EAZ)(ExecState*, JSArray*, int32_t);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EC)(ExecState*, JSCell*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_ECC)(ExecState*, JSCell*, JSCell*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_ECI)(ExecState*, JSCell*, Identifier*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_ECJ)(ExecState*, JSCell*, EncodedJSValue);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EDA)(ExecState*, double, JSArray*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EGriJsgI)(ExecState*, ResolveOperation*, JSGlobalObject*, Identifier*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EI)(ExecState*, Identifier*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EIRo)(ExecState*, Identifier*, ResolveOperations*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EIRoPtbo)(ExecState*, Identifier*, ResolveOperations*, PutToBaseOperation*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EJ)(ExecState*, EncodedJSValue);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EJA)(ExecState*, EncodedJSValue, JSArray*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EJI)(ExecState*, EncodedJSValue, Identifier*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EJJ)(ExecState*, EncodedJSValue, EncodedJSValue);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EJP)(ExecState*, EncodedJSValue, void*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EP)(ExecState*, void*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EPP)(ExecState*, void*, void*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EPS)(ExecState*, void*, size_t);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_ESS)(ExecState*, size_t, size_t);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EZ)(ExecState*, int32_t);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EZIcfZ)(ExecState*, int32_t, InlineCallFrame*, int32_t);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EZZ)(ExecState*, int32_t, int32_t);
typedef JSCell* DFG_OPERATION (*C_DFGOperation_E)(ExecState*);
typedef JSCell* DFG_OPERATION (*C_DFGOperation_EZ)(ExecState*, int32_t);
typedef JSCell* DFG_OPERATION (*C_DFGOperation_EC)(ExecState*, JSCell*);
typedef JSCell* DFG_OPERATION (*C_DFGOperation_ECC)(ExecState*, JSCell*, JSCell*);
typedef JSCell* DFG_OPERATION (*C_DFGOperation_EIcf)(ExecState*, InlineCallFrame*);
typedef JSCell* DFG_OPERATION (*C_DFGOperation_EJ)(ExecState*, EncodedJSValue);
typedef JSCell* DFG_OPERATION (*C_DFGOperation_EJssSt)(ExecState*, JSString*, Structure*);
typedef JSCell* DFG_OPERATION (*C_DFGOperation_EJssJss)(ExecState*, JSString*, JSString*);
typedef JSCell* DFG_OPERATION (*C_DFGOperation_EJssJssJss)(ExecState*, JSString*, JSString*, JSString*);
typedef JSCell* DFG_OPERATION (*C_DFGOperation_EOZ)(ExecState*, JSObject*, int32_t);
typedef JSCell* DFG_OPERATION (*C_DFGOperation_ESt)(ExecState*, Structure*);
typedef JSCell* DFG_OPERATION (*C_DFGOperation_EZ)(ExecState*, int32_t);
typedef double DFG_OPERATION (*D_DFGOperation_DD)(double, double);
typedef double DFG_OPERATION (*D_DFGOperation_ZZ)(int32_t, int32_t);
typedef double DFG_OPERATION (*D_DFGOperation_EJ)(ExecState*, EncodedJSValue);
typedef int32_t DFG_OPERATION (*Z_DFGOperation_D)(double);
typedef size_t DFG_OPERATION (*S_DFGOperation_ECC)(ExecState*, JSCell*, JSCell*);
typedef size_t DFG_OPERATION (*S_DFGOperation_EJ)(ExecState*, EncodedJSValue);
typedef size_t DFG_OPERATION (*S_DFGOperation_EJJ)(ExecState*, EncodedJSValue, EncodedJSValue);
typedef size_t DFG_OPERATION (*S_DFGOperation_J)(EncodedJSValue);
typedef void DFG_OPERATION (*V_DFGOperation_EOZD)(ExecState*, JSObject*, int32_t, double);
typedef void DFG_OPERATION (*V_DFGOperation_EOZJ)(ExecState*, JSObject*, int32_t, EncodedJSValue);
typedef void DFG_OPERATION (*V_DFGOperation_EC)(ExecState*, JSCell*);
typedef void DFG_OPERATION (*V_DFGOperation_ECIcf)(ExecState*, JSCell*, InlineCallFrame*);
typedef void DFG_OPERATION (*V_DFGOperation_ECCIcf)(ExecState*, JSCell*, JSCell*, InlineCallFrame*);
typedef void DFG_OPERATION (*V_DFGOperation_ECJJ)(ExecState*, JSCell*, EncodedJSValue, EncodedJSValue);
typedef void DFG_OPERATION (*V_DFGOperation_ECZ)(ExecState*, JSCell*, int32_t);
typedef void DFG_OPERATION (*V_DFGOperation_ECC)(ExecState*, JSCell*, JSCell*);
typedef void DFG_OPERATION (*V_DFGOperation_EJCI)(ExecState*, EncodedJSValue, JSCell*, Identifier*);
typedef void DFG_OPERATION (*V_DFGOperation_EJJJ)(ExecState*, EncodedJSValue, EncodedJSValue, EncodedJSValue);
typedef void DFG_OPERATION (*V_DFGOperation_EJPP)(ExecState*, EncodedJSValue, EncodedJSValue, void*);
typedef void DFG_OPERATION (*V_DFGOperation_EPZJ)(ExecState*, void*, int32_t, EncodedJSValue);
typedef void DFG_OPERATION (*V_DFGOperation_W)(WatchpointSet*);
typedef char* DFG_OPERATION (*P_DFGOperation_E)(ExecState*);
typedef char* DFG_OPERATION (*P_DFGOperation_EC)(ExecState*, JSCell*);
typedef char* DFG_OPERATION (*P_DFGOperation_EO)(ExecState*, JSObject*);
typedef char* DFG_OPERATION (*P_DFGOperation_EOS)(ExecState*, JSObject*, size_t);
typedef char* DFG_OPERATION (*P_DFGOperation_EOZ)(ExecState*, JSObject*, int32_t);
typedef char* DFG_OPERATION (*P_DFGOperation_EPS)(ExecState*, void*, size_t);
typedef char* DFG_OPERATION (*P_DFGOperation_ES)(ExecState*, size_t);
typedef char* DFG_OPERATION (*P_DFGOperation_ESt)(ExecState*, Structure*);
typedef char* DFG_OPERATION (*P_DFGOperation_EStPS)(ExecState*, Structure*, void*, size_t);
typedef char* DFG_OPERATION (*P_DFGOperation_EStSS)(ExecState*, Structure*, size_t, size_t);
typedef char* DFG_OPERATION (*P_DFGOperation_EStZ)(ExecState*, Structure*, int32_t);
typedef StringImpl* DFG_OPERATION (*Str_DFGOperation_EJss)(ExecState*, JSString*);
JSCell* DFG_OPERATION operationStringFromCharCode(ExecState*, int32_t)  WTF_INTERNAL; 

// These routines are provide callbacks out to C++ implementations of operations too complex to JIT.
JSCell* DFG_OPERATION operationNewObject(ExecState*, Structure*) WTF_INTERNAL;
JSCell* DFG_OPERATION operationCreateThis(ExecState*, JSObject* constructor, int32_t inlineCapacity) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationConvertThis(ExecState*, EncodedJSValue encodedOp1) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationValueAdd(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationValueAddNotNumber(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationGetByVal(ExecState*, EncodedJSValue encodedBase, EncodedJSValue encodedProperty) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationGetByValCell(ExecState*, JSCell*, EncodedJSValue encodedProperty) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationGetByValArrayInt(ExecState*, JSArray*, int32_t) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationGetById(ExecState*, EncodedJSValue, Identifier*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationGetByIdBuildList(ExecState*, EncodedJSValue, Identifier*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationGetByIdProtoBuildList(ExecState*, EncodedJSValue, Identifier*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationGetByIdOptimize(ExecState*, EncodedJSValue, Identifier*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationCallCustomGetter(ExecState*, JSCell*, PropertySlot::GetValueFunc, Identifier*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationCallGetter(ExecState*, JSCell*, JSCell*) WTF_INTERNAL;
void DFG_OPERATION operationNotifyGlobalVarWrite(WatchpointSet* watchpointSet) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationResolve(ExecState*, Identifier*, ResolveOperations*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationResolveBase(ExecState*, Identifier*, ResolveOperations*, PutToBaseOperation*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationResolveBaseStrictPut(ExecState*, Identifier*, ResolveOperations*, PutToBaseOperation*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationResolveGlobal(ExecState*, ResolveOperation*, JSGlobalObject*, Identifier*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationToPrimitive(ExecState*, EncodedJSValue) WTF_INTERNAL;
char* DFG_OPERATION operationNewArray(ExecState*, Structure*, void*, size_t) WTF_INTERNAL;
char* DFG_OPERATION operationNewArrayBuffer(ExecState*, Structure*, size_t, size_t) WTF_INTERNAL;
char* DFG_OPERATION operationNewEmptyArray(ExecState*, Structure*) WTF_INTERNAL;
char* DFG_OPERATION operationNewArrayWithSize(ExecState*, Structure*, int32_t) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationNewRegexp(ExecState*, void*) WTF_INTERNAL;
void DFG_OPERATION operationPutByValStrict(ExecState*, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue) WTF_INTERNAL;
void DFG_OPERATION operationPutByValNonStrict(ExecState*, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue) WTF_INTERNAL;
void DFG_OPERATION operationPutByValCellStrict(ExecState*, JSCell*, EncodedJSValue encodedProperty, EncodedJSValue encodedValue) WTF_INTERNAL;
void DFG_OPERATION operationPutByValCellNonStrict(ExecState*, JSCell*, EncodedJSValue encodedProperty, EncodedJSValue encodedValue) WTF_INTERNAL;
void DFG_OPERATION operationPutByValBeyondArrayBoundsStrict(ExecState*, JSObject*, int32_t index, EncodedJSValue encodedValue) WTF_INTERNAL;
void DFG_OPERATION operationPutByValBeyondArrayBoundsNonStrict(ExecState*, JSObject*, int32_t index, EncodedJSValue encodedValue) WTF_INTERNAL;
void DFG_OPERATION operationPutDoubleByValBeyondArrayBoundsStrict(ExecState*, JSObject*, int32_t index, double value) WTF_INTERNAL;
void DFG_OPERATION operationPutDoubleByValBeyondArrayBoundsNonStrict(ExecState*, JSObject*, int32_t index, double value) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationArrayPush(ExecState*, EncodedJSValue encodedValue, JSArray*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationArrayPushDouble(ExecState*, double value, JSArray*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationArrayPop(ExecState*, JSArray*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationArrayPopAndRecoverLength(ExecState*, JSArray*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationRegExpExec(ExecState*, JSCell*, JSCell*) WTF_INTERNAL;
void DFG_OPERATION operationPutByIdStrict(ExecState*, EncodedJSValue encodedValue, JSCell* base, Identifier*) WTF_INTERNAL;
void DFG_OPERATION operationPutByIdNonStrict(ExecState*, EncodedJSValue encodedValue, JSCell* base, Identifier*) WTF_INTERNAL;
void DFG_OPERATION operationPutByIdDirectStrict(ExecState*, EncodedJSValue encodedValue, JSCell* base, Identifier*) WTF_INTERNAL;
void DFG_OPERATION operationPutByIdDirectNonStrict(ExecState*, EncodedJSValue encodedValue, JSCell* base, Identifier*) WTF_INTERNAL;
void DFG_OPERATION operationPutByIdStrictOptimize(ExecState*, EncodedJSValue encodedValue, JSCell* base, Identifier*) WTF_INTERNAL;
void DFG_OPERATION operationPutByIdNonStrictOptimize(ExecState*, EncodedJSValue encodedValue, JSCell* base, Identifier*) WTF_INTERNAL;
void DFG_OPERATION operationPutByIdDirectStrictOptimize(ExecState*, EncodedJSValue encodedValue, JSCell* base, Identifier*) WTF_INTERNAL;
void DFG_OPERATION operationPutByIdDirectNonStrictOptimize(ExecState*, EncodedJSValue encodedValue, JSCell* base, Identifier*) WTF_INTERNAL;
void DFG_OPERATION operationPutByIdStrictBuildList(ExecState*, EncodedJSValue encodedValue, JSCell* base, Identifier*) WTF_INTERNAL;
void DFG_OPERATION operationPutByIdNonStrictBuildList(ExecState*, EncodedJSValue encodedValue, JSCell* base, Identifier*) WTF_INTERNAL;
void DFG_OPERATION operationPutByIdDirectStrictBuildList(ExecState*, EncodedJSValue encodedValue, JSCell* base, Identifier*) WTF_INTERNAL;
void DFG_OPERATION operationPutByIdDirectNonStrictBuildList(ExecState*, EncodedJSValue encodedValue, JSCell* base, Identifier*) WTF_INTERNAL;
// These comparisons return a boolean within a size_t such that the value is zero extended to fill the register.
size_t DFG_OPERATION operationRegExpTest(ExecState*, JSCell*, JSCell*) WTF_INTERNAL;
size_t DFG_OPERATION operationCompareLess(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
size_t DFG_OPERATION operationCompareLessEq(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
size_t DFG_OPERATION operationCompareGreater(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
size_t DFG_OPERATION operationCompareGreaterEq(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
size_t DFG_OPERATION operationCompareEq(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
#if USE(JSVALUE64)
EncodedJSValue DFG_OPERATION operationCompareStringEq(ExecState*, JSCell* left, JSCell* right) WTF_INTERNAL;
#else
size_t DFG_OPERATION operationCompareStringEq(ExecState*, JSCell* left, JSCell* right) WTF_INTERNAL;
#endif
size_t DFG_OPERATION operationCompareStrictEqCell(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
size_t DFG_OPERATION operationCompareStrictEq(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
char* DFG_OPERATION operationVirtualCall(ExecState*) WTF_INTERNAL;
char* DFG_OPERATION operationLinkCall(ExecState*) WTF_INTERNAL;
char* DFG_OPERATION operationLinkClosureCall(ExecState*) WTF_INTERNAL;
char* DFG_OPERATION operationVirtualConstruct(ExecState*) WTF_INTERNAL;
char* DFG_OPERATION operationLinkConstruct(ExecState*) WTF_INTERNAL;
JSCell* DFG_OPERATION operationCreateActivation(ExecState*) WTF_INTERNAL;
JSCell* DFG_OPERATION operationCreateArguments(ExecState*) WTF_INTERNAL;
JSCell* DFG_OPERATION operationCreateInlinedArguments(ExecState*, InlineCallFrame*) WTF_INTERNAL;
void DFG_OPERATION operationTearOffArguments(ExecState*, JSCell*, JSCell*) WTF_INTERNAL;
void DFG_OPERATION operationTearOffInlinedArguments(ExecState*, JSCell*, JSCell*, InlineCallFrame*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationGetArgumentsLength(ExecState*, int32_t) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationGetInlinedArgumentByVal(ExecState*, int32_t, InlineCallFrame*, int32_t) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationGetArgumentByVal(ExecState*, int32_t, int32_t) WTF_INTERNAL;
JSCell* DFG_OPERATION operationNewFunctionNoCheck(ExecState*, JSCell*) WTF_INTERNAL;
EncodedJSValue DFG_OPERATION operationNewFunction(ExecState*, JSCell*) WTF_INTERNAL;
JSCell* DFG_OPERATION operationNewFunctionExpression(ExecState*, JSCell*) WTF_INTERNAL;
double DFG_OPERATION operationFModOnInts(int32_t, int32_t) WTF_INTERNAL;
size_t DFG_OPERATION operationIsObject(ExecState*, EncodedJSValue) WTF_INTERNAL;
size_t DFG_OPERATION operationIsFunction(EncodedJSValue) WTF_INTERNAL;
JSCell* DFG_OPERATION operationTypeOf(ExecState*, JSCell*) WTF_INTERNAL;
void DFG_OPERATION operationReallocateStorageAndFinishPut(ExecState*, JSObject*, Structure*, PropertyOffset, EncodedJSValue) WTF_INTERNAL;
char* DFG_OPERATION operationAllocatePropertyStorageWithInitialCapacity(ExecState*) WTF_INTERNAL;
char* DFG_OPERATION operationAllocatePropertyStorage(ExecState*, size_t newSize) WTF_INTERNAL;
char* DFG_OPERATION operationReallocateButterflyToHavePropertyStorageWithInitialCapacity(ExecState*, JSObject*) WTF_INTERNAL;
char* DFG_OPERATION operationReallocateButterflyToGrowPropertyStorage(ExecState*, JSObject*, size_t newSize) WTF_INTERNAL;
char* DFG_OPERATION operationEnsureInt32(ExecState*, JSCell*);
char* DFG_OPERATION operationEnsureDouble(ExecState*, JSCell*);
char* DFG_OPERATION operationEnsureContiguous(ExecState*, JSCell*);
char* DFG_OPERATION operationRageEnsureContiguous(ExecState*, JSCell*);
char* DFG_OPERATION operationEnsureArrayStorage(ExecState*, JSCell*);
StringImpl* DFG_OPERATION operationResolveRope(ExecState*, JSString*);
JSCell* DFG_OPERATION operationNewStringObject(ExecState*, JSString*, Structure*);
JSCell* DFG_OPERATION operationToStringOnCell(ExecState*, JSCell*);
JSCell* DFG_OPERATION operationToString(ExecState*, EncodedJSValue);
JSCell* DFG_OPERATION operationMakeRope2(ExecState*, JSString*, JSString*);
JSCell* DFG_OPERATION operationMakeRope3(ExecState*, JSString*, JSString*, JSString*);

// This method is used to lookup an exception hander, keyed by faultLocation, which is
// the return location from one of the calls out to one of the helper operations above.

// According to C++ rules, a type used for the return signature of function with C linkage (i.e.
// 'extern "C"') needs to be POD; hence putting any constructors into it could cause either compiler
// warnings, or worse, a change in the ABI used to return these types.
struct DFGHandler {
    union Union {
        struct Struct {
            ExecState* exec;
            void* handler;
        } s;
        uint64_t encoded;
    } u;
};

inline DFGHandler createDFGHandler(ExecState* exec, void* handler)
{
    DFGHandler result;
    result.u.s.exec = exec;
    result.u.s.handler = handler;
    return result;
}

#if CPU(X86_64)
typedef DFGHandler DFGHandlerEncoded;
inline DFGHandlerEncoded dfgHandlerEncoded(ExecState* exec, void* handler)
{
    return createDFGHandler(exec, handler);
}
#else
typedef uint64_t DFGHandlerEncoded;
inline DFGHandlerEncoded dfgHandlerEncoded(ExecState* exec, void* handler)
{
    COMPILE_ASSERT(sizeof(DFGHandler::Union) == sizeof(uint64_t), DFGHandler_Union_is_64bit);
    return createDFGHandler(exec, handler).u.encoded;
}
#endif
DFGHandlerEncoded DFG_OPERATION lookupExceptionHandler(ExecState*, uint32_t) WTF_INTERNAL;
DFGHandlerEncoded DFG_OPERATION lookupExceptionHandlerInStub(ExecState*, StructureStubInfo*) WTF_INTERNAL;

// These operations implement the implicitly called ToInt32 and ToBoolean conversions from ES5.
// This conversion returns an int32_t within a size_t such that the value is zero extended to fill the register.
size_t DFG_OPERATION dfgConvertJSValueToInt32(ExecState*, EncodedJSValue) WTF_INTERNAL;
size_t DFG_OPERATION dfgConvertJSValueToBoolean(ExecState*, EncodedJSValue) WTF_INTERNAL;

void DFG_OPERATION debugOperationPrintSpeculationFailure(ExecState*, void*, void*) WTF_INTERNAL;

void DFG_OPERATION triggerReoptimizationNow(CodeBlock*) WTF_INTERNAL;

} // extern "C"
} } // namespace JSC::DFG

#endif
#endif
