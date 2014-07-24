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

#include "config.h"
#include "DFGThunks.h"

#if ENABLE(DFG_JIT)

#include "DFGCCallHelpers.h"
#include "DFGFPRInfo.h"
#include "DFGGPRInfo.h"
#include "DFGOSRExitCompiler.h"
#include "MacroAssembler.h"

namespace JSC { namespace DFG {

MacroAssemblerCodeRef osrExitGenerationThunkGenerator(VM* vm)
{
    MacroAssembler jit;
    
    size_t scratchSize = sizeof(EncodedJSValue) * (GPRInfo::numberOfRegisters + FPRInfo::numberOfRegisters);
    ScratchBuffer* scratchBuffer = vm->scratchBufferForSize(scratchSize);
    EncodedJSValue* buffer = static_cast<EncodedJSValue*>(scratchBuffer->dataBuffer());
    
    for (unsigned i = 0; i < GPRInfo::numberOfRegisters; ++i) {
#if USE(JSVALUE64)
        jit.store64(GPRInfo::toRegister(i), buffer + i);
#else
        jit.store32(GPRInfo::toRegister(i), buffer + i);
#endif
    }
    for (unsigned i = 0; i < FPRInfo::numberOfRegisters; ++i) {
        jit.move(MacroAssembler::TrustedImmPtr(buffer + GPRInfo::numberOfRegisters + i), GPRInfo::regT0);
        jit.storeDouble(FPRInfo::toRegister(i), GPRInfo::regT0);
    }
    
    // Tell GC mark phase how much of the scratch buffer is active during call.
    jit.move(MacroAssembler::TrustedImmPtr(scratchBuffer->activeLengthPtr()), GPRInfo::regT0);
    jit.storePtr(MacroAssembler::TrustedImmPtr(scratchSize), GPRInfo::regT0);

    // Set up one argument.
#if CPU(X86)
    jit.poke(GPRInfo::callFrameRegister, 0);
#else
    jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
#endif

    MacroAssembler::Call functionCall = jit.call();

    jit.move(MacroAssembler::TrustedImmPtr(scratchBuffer->activeLengthPtr()), GPRInfo::regT0);
    jit.storePtr(MacroAssembler::TrustedImmPtr(0), GPRInfo::regT0);

    for (unsigned i = 0; i < FPRInfo::numberOfRegisters; ++i) {
        jit.move(MacroAssembler::TrustedImmPtr(buffer + GPRInfo::numberOfRegisters + i), GPRInfo::regT0);
        jit.loadDouble(GPRInfo::regT0, FPRInfo::toRegister(i));
    }
    for (unsigned i = 0; i < GPRInfo::numberOfRegisters; ++i) {
#if USE(JSVALUE64)
        jit.load64(buffer + i, GPRInfo::toRegister(i));
#else
        jit.load32(buffer + i, GPRInfo::toRegister(i));
#endif
    }
    
    jit.jump(MacroAssembler::AbsoluteAddress(&vm->osrExitJumpDestination));
    
    LinkBuffer patchBuffer(*vm, &jit, GLOBAL_THUNK_ID);
    
    patchBuffer.link(functionCall, compileOSRExit);
    
    return FINALIZE_CODE(patchBuffer, ("DFG OSR exit generation thunk"));
}

inline void emitPointerValidation(CCallHelpers& jit, GPRReg pointerGPR)
{
#if !ASSERT_DISABLED
    CCallHelpers::Jump isNonZero = jit.branchTestPtr(CCallHelpers::NonZero, pointerGPR);
    jit.breakpoint();
    isNonZero.link(&jit);
    jit.push(pointerGPR);
    jit.load8(pointerGPR, pointerGPR);
    jit.pop(pointerGPR);
#else
    UNUSED_PARAM(jit);
    UNUSED_PARAM(pointerGPR);
#endif
}

MacroAssemblerCodeRef throwExceptionFromCallSlowPathGenerator(VM* vm)
{
    CCallHelpers jit(vm);
    
    // We will jump to here if the JIT code thinks it's making a call, but the
    // linking helper (C++ code) decided to throw an exception instead. We will
    // have saved the callReturnIndex in the first arguments of JITStackFrame.
    // Note that the return address will be on the stack at this point, so we
    // need to remove it and drop it on the floor, since we don't care about it.
    // Finally note that the call frame register points at the callee frame, so
    // we need to pop it.
    jit.preserveReturnAddressAfterCall(GPRInfo::nonPreservedNonReturnGPR);
    jit.loadPtr(
        CCallHelpers::Address(
            GPRInfo::callFrameRegister,
            static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::CallerFrame),
        GPRInfo::callFrameRegister);
#if USE(JSVALUE64)
    jit.peek64(GPRInfo::nonPreservedNonReturnGPR, JITSTACKFRAME_ARGS_INDEX);
#else
    jit.peek(GPRInfo::nonPreservedNonReturnGPR, JITSTACKFRAME_ARGS_INDEX);
#endif
    jit.setupArgumentsWithExecState(GPRInfo::nonPreservedNonReturnGPR);
    jit.move(CCallHelpers::TrustedImmPtr(bitwise_cast<void*>(lookupExceptionHandler)), GPRInfo::nonArgGPR0);
    emitPointerValidation(jit, GPRInfo::nonArgGPR0);
    jit.call(GPRInfo::nonArgGPR0);
    emitPointerValidation(jit, GPRInfo::returnValueGPR2);
    jit.jump(GPRInfo::returnValueGPR2);
    
    LinkBuffer patchBuffer(*vm, &jit, GLOBAL_THUNK_ID);
    return FINALIZE_CODE(patchBuffer, ("DFG throw exception from call slow path thunk"));
}

static void slowPathFor(
    CCallHelpers& jit, VM* vm, P_DFGOperation_E slowPathFunction)
{
    jit.preserveReturnAddressAfterCall(GPRInfo::nonArgGPR2);
    emitPointerValidation(jit, GPRInfo::nonArgGPR2);
    jit.storePtr(
        GPRInfo::nonArgGPR2,
        CCallHelpers::Address(
            GPRInfo::callFrameRegister,
            static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ReturnPC));
    jit.storePtr(GPRInfo::callFrameRegister, &vm->topCallFrame);
#if USE(JSVALUE64)
    jit.poke64(GPRInfo::nonPreservedNonReturnGPR, JITSTACKFRAME_ARGS_INDEX);
#else
    jit.poke(GPRInfo::nonPreservedNonReturnGPR, JITSTACKFRAME_ARGS_INDEX);
#endif
    jit.setupArgumentsExecState();
    jit.move(CCallHelpers::TrustedImmPtr(bitwise_cast<void*>(slowPathFunction)), GPRInfo::nonArgGPR0);
    emitPointerValidation(jit, GPRInfo::nonArgGPR0);
    jit.call(GPRInfo::nonArgGPR0);
    
    // This slow call will return the address of one of the following:
    // 1) Exception throwing thunk.
    // 2) Host call return value returner thingy.
    // 3) The function to call.
    jit.loadPtr(
        CCallHelpers::Address(
            GPRInfo::callFrameRegister,
            static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ReturnPC),
        GPRInfo::nonPreservedNonReturnGPR);
    jit.storePtr(
        CCallHelpers::TrustedImmPtr(0),
        CCallHelpers::Address(
            GPRInfo::callFrameRegister,
            static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ReturnPC));
    emitPointerValidation(jit, GPRInfo::nonPreservedNonReturnGPR);
    jit.restoreReturnAddressBeforeReturn(GPRInfo::nonPreservedNonReturnGPR);
    emitPointerValidation(jit, GPRInfo::returnValueGPR);
    jit.jump(GPRInfo::returnValueGPR);
}

static MacroAssemblerCodeRef linkForThunkGenerator(
    VM* vm, CodeSpecializationKind kind)
{
    // The return address is on the stack or in the link register. We will hence
    // save the return address to the call frame while we make a C++ function call
    // to perform linking and lazy compilation if necessary. We expect the callee
    // to be in nonArgGPR0/nonArgGPR1 (payload/tag), the call frame to have already
    // been adjusted, nonPreservedNonReturnGPR holds the exception handler index,
    // and all other registers to be available for use. We use JITStackFrame::args
    // to save important information across calls.
    
    CCallHelpers jit(vm);
    
    slowPathFor(jit, vm, kind == CodeForCall ? operationLinkCall : operationLinkConstruct);
    
    LinkBuffer patchBuffer(*vm, &jit, GLOBAL_THUNK_ID);
    return FINALIZE_CODE(
        patchBuffer,
        ("DFG link %s slow path thunk", kind == CodeForCall ? "call" : "construct"));
}

MacroAssemblerCodeRef linkCallThunkGenerator(VM* vm)
{
    return linkForThunkGenerator(vm, CodeForCall);
}

MacroAssemblerCodeRef linkConstructThunkGenerator(VM* vm)
{
    return linkForThunkGenerator(vm, CodeForConstruct);
}

// For closure optimizations, we only include calls, since if you're using closures for
// object construction then you're going to lose big time anyway.
MacroAssemblerCodeRef linkClosureCallThunkGenerator(VM* vm)
{
    CCallHelpers jit(vm);
    
    slowPathFor(jit, vm, operationLinkClosureCall);
    
    LinkBuffer patchBuffer(*vm, &jit, GLOBAL_THUNK_ID);
    return FINALIZE_CODE(patchBuffer, ("DFG link closure call slow path thunk"));
}

static MacroAssemblerCodeRef virtualForThunkGenerator(
    VM* vm, CodeSpecializationKind kind)
{
    // The return address is on the stack, or in the link register. We will hence
    // jump to the callee, or save the return address to the call frame while we
    // make a C++ function call to the appropriate DFG operation.

    CCallHelpers jit(vm);
    
    CCallHelpers::JumpList slowCase;

    // FIXME: we should have a story for eliminating these checks. In many cases,
    // the DFG knows that the value is definitely a cell, or definitely a function.
    
#if USE(JSVALUE64)
    slowCase.append(
        jit.branchTest64(
            CCallHelpers::NonZero, GPRInfo::nonArgGPR0, GPRInfo::tagMaskRegister));
#else
    slowCase.append(
        jit.branch32(
            CCallHelpers::NotEqual, GPRInfo::nonArgGPR1,
            CCallHelpers::TrustedImm32(JSValue::CellTag)));
#endif
    jit.loadPtr(CCallHelpers::Address(GPRInfo::nonArgGPR0, JSCell::structureOffset()), GPRInfo::nonArgGPR2);
    slowCase.append(
        jit.branchPtr(
            CCallHelpers::NotEqual,
            CCallHelpers::Address(GPRInfo::nonArgGPR2, Structure::classInfoOffset()),
            CCallHelpers::TrustedImmPtr(&JSFunction::s_info)));
    
    // Now we know we have a JSFunction.
    
    jit.loadPtr(
        CCallHelpers::Address(GPRInfo::nonArgGPR0, JSFunction::offsetOfExecutable()),
        GPRInfo::nonArgGPR2);
    slowCase.append(
        jit.branch32(
            CCallHelpers::LessThan,
            CCallHelpers::Address(
                GPRInfo::nonArgGPR2, ExecutableBase::offsetOfNumParametersFor(kind)),
            CCallHelpers::TrustedImm32(0)));
    
    // Now we know that we have a CodeBlock, and we're committed to making a fast
    // call.
    
    jit.loadPtr(
        CCallHelpers::Address(GPRInfo::nonArgGPR0, JSFunction::offsetOfScopeChain()),
        GPRInfo::nonArgGPR1);
#if USE(JSVALUE64)
    jit.store64(
        GPRInfo::nonArgGPR1,
        CCallHelpers::Address(
            GPRInfo::callFrameRegister,
            static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ScopeChain));
#else
    jit.storePtr(
        GPRInfo::nonArgGPR1,
        CCallHelpers::Address(
            GPRInfo::callFrameRegister,
            static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ScopeChain +
            OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
    jit.store32(
        CCallHelpers::TrustedImm32(JSValue::CellTag),
        CCallHelpers::Address(
            GPRInfo::callFrameRegister,
            static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ScopeChain +
            OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
#endif
    
    jit.loadPtr(
        CCallHelpers::Address(GPRInfo::nonArgGPR2, ExecutableBase::offsetOfJITCodeWithArityCheckFor(kind)),
        GPRInfo::regT0);
    
    // Make a tail call. This will return back to DFG code.
    emitPointerValidation(jit, GPRInfo::regT0);
    jit.jump(GPRInfo::regT0);

    slowCase.link(&jit);
    
    // Here we don't know anything, so revert to the full slow path.
    
    slowPathFor(jit, vm, kind == CodeForCall ? operationVirtualCall : operationVirtualConstruct);
    
    LinkBuffer patchBuffer(*vm, &jit, GLOBAL_THUNK_ID);
    return FINALIZE_CODE(
        patchBuffer,
        ("DFG virtual %s slow path thunk", kind == CodeForCall ? "call" : "construct"));
}

MacroAssemblerCodeRef virtualCallThunkGenerator(VM* vm)
{
    return virtualForThunkGenerator(vm, CodeForCall);
}

MacroAssemblerCodeRef virtualConstructThunkGenerator(VM* vm)
{
    return virtualForThunkGenerator(vm, CodeForConstruct);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)
