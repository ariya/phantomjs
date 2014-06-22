# Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

# First come the common protocols that both interpreters use. Note that each
# of these must have an ASSERT() in LLIntData.cpp

# Work-around for the fact that the toolchain's awareness of armv7s results in
# a separate slab in the fat binary, yet the offlineasm doesn't know to expect
# it.
if ARMv7s
end

# These declarations must match interpreter/JSStack.h.
const CallFrameHeaderSize = 48
const ArgumentCount = -48
const CallerFrame = -40
const Callee = -32
const ScopeChain = -24
const ReturnPC = -16
const CodeBlock = -8

const ThisArgumentOffset = -CallFrameHeaderSize - 8

# Some register conventions.
if JSVALUE64
    # - Use a pair of registers to represent the PC: one register for the
    #   base of the stack, and one register for the index.
    # - The PC base (or PB for short) should be stored in the csr. It will
    #   get clobbered on calls to other JS code, but will get saved on calls
    #   to C functions.
    # - C calls are still given the Instruction* rather than the PC index.
    #   This requires an add before the call, and a sub after.
    const PC = t4
    const PB = t6
    const tagTypeNumber = csr1
    const tagMask = csr2
    
    macro loadisFromInstruction(offset, dest)
        loadis offset * 8[PB, PC, 8], dest
    end
    
    macro loadpFromInstruction(offset, dest)
        loadp offset * 8[PB, PC, 8], dest
    end
    
    macro storepToInstruction(value, offset)
        storep value, offset * 8[PB, PC, 8]
    end

else
    const PC = t4
    macro loadisFromInstruction(offset, dest)
        loadis offset * 4[PC], dest
    end
    
    macro loadpFromInstruction(offset, dest)
        loadp offset * 4[PC], dest
    end
end

# Constants for reasoning about value representation.
if BIG_ENDIAN
    const TagOffset = 0
    const PayloadOffset = 4
else
    const TagOffset = 4
    const PayloadOffset = 0
end

# Constant for reasoning about butterflies.
const IsArray                  = 1
const IndexingShapeMask        = 30
const NoIndexingShape          = 0
const Int32Shape               = 20
const DoubleShape              = 22
const ContiguousShape          = 26
const ArrayStorageShape        = 28
const SlowPutArrayStorageShape = 30

# Type constants.
const StringType = 5
const ObjectType = 17

# Type flags constants.
const MasqueradesAsUndefined = 1
const ImplementsHasInstance = 2
const ImplementsDefaultHasInstance = 8

# Bytecode operand constants.
const FirstConstantRegisterIndex = 0x40000000

# Code type constants.
const GlobalCode = 0
const EvalCode = 1
const FunctionCode = 2

# The interpreter steals the tag word of the argument count.
const LLIntReturnPC = ArgumentCount + TagOffset

# String flags.
const HashFlags8BitBuffer = 64

# Copied from PropertyOffset.h
const firstOutOfLineOffset = 100

# From ResolveOperations.h
const ResolveOperationFail = 0
const ResolveOperationSetBaseToUndefined = 1
const ResolveOperationReturnScopeAsBase = 2
const ResolveOperationSetBaseToScope = 3
const ResolveOperationSetBaseToGlobal = 4
const ResolveOperationGetAndReturnScopedVar = 5
const ResolveOperationGetAndReturnGlobalVar = 6
const ResolveOperationGetAndReturnGlobalVarWatchable = 7
const ResolveOperationSkipTopScopeNode = 8
const ResolveOperationSkipScopes = 9
const ResolveOperationReturnGlobalObjectAsBase = 10
const ResolveOperationGetAndReturnGlobalProperty = 11
const ResolveOperationCheckForDynamicEntriesBeforeGlobalScope = 12

const PutToBaseOperationKindUninitialised = 0
const PutToBaseOperationKindGeneric = 1
const PutToBaseOperationKindReadonly = 2
const PutToBaseOperationKindGlobalVariablePut = 3
const PutToBaseOperationKindGlobalVariablePutChecked = 4
const PutToBaseOperationKindGlobalPropertyPut = 5
const PutToBaseOperationKindVariablePut = 6

# Allocation constants
if JSVALUE64
    const JSFinalObjectSizeClassIndex = 1
else
    const JSFinalObjectSizeClassIndex = 3
end

# This must match wtf/Vector.h
const VectorBufferOffset = 0
if JSVALUE64
    const VectorSizeOffset = 12
else
    const VectorSizeOffset = 8
end


# Some common utilities.
macro crash()
    if C_LOOP
        cloopCrash
    else
        storei t0, 0xbbadbeef[]
        move 0, t0
        call t0
    end
end

macro assert(assertion)
    if ASSERT_ENABLED
        assertion(.ok)
        crash()
    .ok:
    end
end

macro preserveReturnAddressAfterCall(destinationRegister)
    if C_LOOP or ARM or ARMv7 or ARMv7_TRADITIONAL or MIPS
        # In C_LOOP case, we're only preserving the bytecode vPC.
        move lr, destinationRegister
    elsif SH4
        stspr destinationRegister
    elsif X86 or X86_64
        pop destinationRegister
    else
        error
    end
end

macro restoreReturnAddressBeforeReturn(sourceRegister)
    if C_LOOP or ARM or ARMv7 or ARMv7_TRADITIONAL or MIPS
        # In C_LOOP case, we're only restoring the bytecode vPC.
        move sourceRegister, lr
    elsif SH4
        ldspr sourceRegister
    elsif X86 or X86_64
        push sourceRegister
    else
        error
    end
end

macro traceExecution()
    if EXECUTION_TRACING
        callSlowPath(_llint_trace)
    end
end

macro callTargetFunction(callLinkInfo)
    if C_LOOP
        cloopCallJSFunction LLIntCallLinkInfo::machineCodeTarget[callLinkInfo]
    else
        call LLIntCallLinkInfo::machineCodeTarget[callLinkInfo]
        dispatchAfterCall()
    end
end

macro slowPathForCall(advance, slowPath)
    callCallSlowPath(
        advance,
        slowPath,
        macro (callee)
            if C_LOOP
                cloopCallJSFunction callee
            else
                call callee
                dispatchAfterCall()
            end
        end)
end

macro arrayProfile(structureAndIndexingType, profile, scratch)
    const structure = structureAndIndexingType
    const indexingType = structureAndIndexingType
    if VALUE_PROFILER
        storep structure, ArrayProfile::m_lastSeenStructure[profile]
    end
    loadb Structure::m_indexingType[structure], indexingType
end

macro checkSwitchToJIT(increment, action)
    if JIT_ENABLED
        loadp CodeBlock[cfr], t0
        baddis increment, CodeBlock::m_llintExecuteCounter + ExecutionCounter::m_counter[t0], .continue
        action()
    .continue:
    end
end

macro checkSwitchToJITForEpilogue()
    checkSwitchToJIT(
        10,
        macro ()
            callSlowPath(_llint_replace)
        end)
end

macro assertNotConstant(index)
    assert(macro (ok) bilt index, FirstConstantRegisterIndex, ok end)
end

macro functionForCallCodeBlockGetter(targetRegister)
    loadp Callee[cfr], targetRegister
    loadp JSFunction::m_executable[targetRegister], targetRegister
    loadp FunctionExecutable::m_codeBlockForCall[targetRegister], targetRegister
end

macro functionForConstructCodeBlockGetter(targetRegister)
    loadp Callee[cfr], targetRegister
    loadp JSFunction::m_executable[targetRegister], targetRegister
    loadp FunctionExecutable::m_codeBlockForConstruct[targetRegister], targetRegister
end

macro notFunctionCodeBlockGetter(targetRegister)
    loadp CodeBlock[cfr], targetRegister
end

macro functionCodeBlockSetter(sourceRegister)
    storep sourceRegister, CodeBlock[cfr]
end

macro notFunctionCodeBlockSetter(sourceRegister)
    # Nothing to do!
end

# Do the bare minimum required to execute code. Sets up the PC, leave the CodeBlock*
# in t1. May also trigger prologue entry OSR.
macro prologue(codeBlockGetter, codeBlockSetter, osrSlowPath, traceSlowPath)
    preserveReturnAddressAfterCall(t2)
    
    # Set up the call frame and check if we should OSR.
    storep t2, ReturnPC[cfr]
    if EXECUTION_TRACING
        callSlowPath(traceSlowPath)
    end
    codeBlockGetter(t1)
    if JIT_ENABLED
        baddis 5, CodeBlock::m_llintExecuteCounter + ExecutionCounter::m_counter[t1], .continue
        cCall2(osrSlowPath, cfr, PC)
        move t1, cfr
        btpz t0, .recover
        loadp ReturnPC[cfr], t2
        restoreReturnAddressBeforeReturn(t2)
        jmp t0
    .recover:
        codeBlockGetter(t1)
    .continue:
    end
    codeBlockSetter(t1)
    
    # Set up the PC.
    if JSVALUE64
        loadp CodeBlock::m_instructions[t1], PB
        move 0, PC
    else
        loadp CodeBlock::m_instructions[t1], PC
    end
end

# Expects that CodeBlock is in t1, which is what prologue() leaves behind.
# Must call dispatch(0) after calling this.
macro functionInitialization(profileArgSkip)
    if VALUE_PROFILER
        # Profile the arguments. Unfortunately, we have no choice but to do this. This
        # code is pretty horrendous because of the difference in ordering between
        # arguments and value profiles, the desire to have a simple loop-down-to-zero
        # loop, and the desire to use only three registers so as to preserve the PC and
        # the code block. It is likely that this code should be rewritten in a more
        # optimal way for architectures that have more than five registers available
        # for arbitrary use in the interpreter.
        loadi CodeBlock::m_numParameters[t1], t0
        addp -profileArgSkip, t0 # Use addi because that's what has the peephole
        assert(macro (ok) bpgteq t0, 0, ok end)
        btpz t0, .argumentProfileDone
        loadp CodeBlock::m_argumentValueProfiles + VectorBufferOffset[t1], t3
        mulp sizeof ValueProfile, t0, t2 # Aaaaahhhh! Need strength reduction!
        negp t0
        lshiftp 3, t0
        addp t2, t3
    .argumentProfileLoop:
        if JSVALUE64
            loadq ThisArgumentOffset + 8 - profileArgSkip * 8[cfr, t0], t2
            subp sizeof ValueProfile, t3
            storeq t2, profileArgSkip * sizeof ValueProfile + ValueProfile::m_buckets[t3]
        else
            loadi ThisArgumentOffset + TagOffset + 8 - profileArgSkip * 8[cfr, t0], t2
            subp sizeof ValueProfile, t3
            storei t2, profileArgSkip * sizeof ValueProfile + ValueProfile::m_buckets + TagOffset[t3]
            loadi ThisArgumentOffset + PayloadOffset + 8 - profileArgSkip * 8[cfr, t0], t2
            storei t2, profileArgSkip * sizeof ValueProfile + ValueProfile::m_buckets + PayloadOffset[t3]
        end
        baddpnz 8, t0, .argumentProfileLoop
    .argumentProfileDone:
    end
        
    # Check stack height.
    loadi CodeBlock::m_numCalleeRegisters[t1], t0
    loadp CodeBlock::m_vm[t1], t2
    loadp VM::interpreter[t2], t2   # FIXME: Can get to the JSStack from the JITStackFrame
    lshifti 3, t0
    addp t0, cfr, t0
    bpaeq Interpreter::m_stack + JSStack::m_end[t2], t0, .stackHeightOK

    # Stack height check failed - need to call a slow_path.
    callSlowPath(_llint_stack_check)
.stackHeightOK:
end

macro allocateJSObject(allocator, structure, result, scratch1, slowCase)
    if ALWAYS_ALLOCATE_SLOW
        jmp slowCase
    else
        const offsetOfFirstFreeCell = 
            MarkedAllocator::m_freeList + 
            MarkedBlock::FreeList::head

        # Get the object from the free list.   
        loadp offsetOfFirstFreeCell[allocator], result
        btpz result, slowCase
        
        # Remove the object from the free list.
        loadp [result], scratch1
        storep scratch1, offsetOfFirstFreeCell[allocator]
    
        # Initialize the object.
        storep structure, JSCell::m_structure[result]
        storep 0, JSObject::m_butterfly[result]
    end
end

macro doReturn()
    loadp ReturnPC[cfr], t2
    loadp CallerFrame[cfr], cfr
    restoreReturnAddressBeforeReturn(t2)
    ret
end


# Indicate the beginning of LLInt.
_llint_begin:
    crash()


_llint_program_prologue:
    prologue(notFunctionCodeBlockGetter, notFunctionCodeBlockSetter, _llint_entry_osr, _llint_trace_prologue)
    dispatch(0)


_llint_eval_prologue:
    prologue(notFunctionCodeBlockGetter, notFunctionCodeBlockSetter, _llint_entry_osr, _llint_trace_prologue)
    dispatch(0)


_llint_function_for_call_prologue:
    prologue(functionForCallCodeBlockGetter, functionCodeBlockSetter, _llint_entry_osr_function_for_call, _llint_trace_prologue_function_for_call)
.functionForCallBegin:
    functionInitialization(0)
    dispatch(0)
    

_llint_function_for_construct_prologue:
    prologue(functionForConstructCodeBlockGetter, functionCodeBlockSetter, _llint_entry_osr_function_for_construct, _llint_trace_prologue_function_for_construct)
.functionForConstructBegin:
    functionInitialization(1)
    dispatch(0)
    

_llint_function_for_call_arity_check:
    prologue(functionForCallCodeBlockGetter, functionCodeBlockSetter, _llint_entry_osr_function_for_call_arityCheck, _llint_trace_arityCheck_for_call)
    functionArityCheck(.functionForCallBegin, _llint_slow_path_call_arityCheck)


_llint_function_for_construct_arity_check:
    prologue(functionForConstructCodeBlockGetter, functionCodeBlockSetter, _llint_entry_osr_function_for_construct_arityCheck, _llint_trace_arityCheck_for_construct)
    functionArityCheck(.functionForConstructBegin, _llint_slow_path_construct_arityCheck)


# Value-representation-specific code.
if JSVALUE64
    include LowLevelInterpreter64
else
    include LowLevelInterpreter32_64
end


# Value-representation-agnostic code.
_llint_op_new_array:
    traceExecution()
    callSlowPath(_llint_slow_path_new_array)
    dispatch(5)


_llint_op_new_array_with_size:
    traceExecution()
    callSlowPath(_llint_slow_path_new_array_with_size)
    dispatch(4)


_llint_op_new_array_buffer:
    traceExecution()
    callSlowPath(_llint_slow_path_new_array_buffer)
    dispatch(5)


_llint_op_new_regexp:
    traceExecution()
    callSlowPath(_llint_slow_path_new_regexp)
    dispatch(3)


_llint_op_less:
    traceExecution()
    callSlowPath(_llint_slow_path_less)
    dispatch(4)


_llint_op_lesseq:
    traceExecution()
    callSlowPath(_llint_slow_path_lesseq)
    dispatch(4)


_llint_op_greater:
    traceExecution()
    callSlowPath(_llint_slow_path_greater)
    dispatch(4)


_llint_op_greatereq:
    traceExecution()
    callSlowPath(_llint_slow_path_greatereq)
    dispatch(4)


_llint_op_mod:
    traceExecution()
    callSlowPath(_llint_slow_path_mod)
    dispatch(4)


_llint_op_typeof:
    traceExecution()
    callSlowPath(_llint_slow_path_typeof)
    dispatch(3)


_llint_op_is_object:
    traceExecution()
    callSlowPath(_llint_slow_path_is_object)
    dispatch(3)


_llint_op_is_function:
    traceExecution()
    callSlowPath(_llint_slow_path_is_function)
    dispatch(3)


_llint_op_in:
    traceExecution()
    callSlowPath(_llint_slow_path_in)
    dispatch(4)

macro getPutToBaseOperationField(scratch, scratch1, fieldOffset, fieldGetter)
    loadpFromInstruction(4, scratch)
    fieldGetter(fieldOffset[scratch])
end

macro moveJSValueFromRegisterWithoutProfiling(value, destBuffer, destOffsetReg)
    storeq value, [destBuffer, destOffsetReg, 8]
end


macro moveJSValueFromRegistersWithoutProfiling(tag, payload, destBuffer, destOffsetReg)
    storei tag, TagOffset[destBuffer, destOffsetReg, 8]
    storei payload, PayloadOffset[destBuffer, destOffsetReg, 8]
end

macro putToBaseVariableBody(variableOffset, scratch1, scratch2, scratch3)
    loadisFromInstruction(1, scratch1)
    loadp PayloadOffset[cfr, scratch1, 8], scratch1
    loadp JSVariableObject::m_registers[scratch1], scratch1
    loadisFromInstruction(3, scratch2)
    if JSVALUE64
        loadConstantOrVariable(scratch2, scratch3)
        moveJSValueFromRegisterWithoutProfiling(scratch3, scratch1, variableOffset)
    else
        loadConstantOrVariable2Reg(scratch2, scratch3, scratch2) # scratch3=tag, scratch2=payload
        moveJSValueFromRegistersWithoutProfiling(scratch3, scratch2, scratch1, variableOffset)
    end
end

_llint_op_put_to_base_variable:
    traceExecution()
    getPutToBaseOperationField(t0, t1, PutToBaseOperation::m_offset, macro(addr)
                                              loadis  addr, t0
                                          end)
    putToBaseVariableBody(t0, t1, t2, t3)
    dispatch(5)

_llint_op_put_to_base:
    traceExecution()
    getPutToBaseOperationField(t0, t1, 0, macro(addr)
                                              leap addr, t0
                                              bbneq PutToBaseOperation::m_kindAsUint8[t0], PutToBaseOperationKindVariablePut, .notPutToBaseVariable
                                              loadis PutToBaseOperation::m_offset[t0], t0
                                              putToBaseVariableBody(t0, t1, t2, t3)
                                              dispatch(5)
                                              .notPutToBaseVariable:
                                          end)
    callSlowPath(_llint_slow_path_put_to_base)
    dispatch(5)

macro getResolveOperation(resolveOperationIndex, dest)
    loadpFromInstruction(resolveOperationIndex, dest)
    loadp VectorBufferOffset[dest], dest
end

macro getScope(loadInitialScope, scopeCount, dest, scratch)
    loadInitialScope(dest)
    loadi scopeCount, scratch

    btiz scratch, .done
.loop:
    loadp JSScope::m_next[dest], dest
    subi 1, scratch
    btinz scratch, .loop

.done:
end

macro moveJSValue(sourceBuffer, sourceOffsetReg, destBuffer, destOffsetReg, profileOffset, scratchRegister)
    if JSVALUE64
        loadq [sourceBuffer, sourceOffsetReg, 8], scratchRegister
        storeq scratchRegister, [destBuffer, destOffsetReg, 8]
        loadpFromInstruction(profileOffset, destOffsetReg)
        valueProfile(scratchRegister, destOffsetReg)
    else
        loadi PayloadOffset[sourceBuffer, sourceOffsetReg, 8], scratchRegister
        storei scratchRegister, PayloadOffset[destBuffer, destOffsetReg, 8]
        loadi TagOffset[sourceBuffer, sourceOffsetReg, 8], sourceOffsetReg
        storei sourceOffsetReg, TagOffset[destBuffer, destOffsetReg, 8]
        loadpFromInstruction(profileOffset, destOffsetReg)
        valueProfile(sourceOffsetReg, scratchRegister, destOffsetReg)
    end
end

macro moveJSValueFromSlot(slot, destBuffer, destOffsetReg, profileOffset, scratchRegister)
    if JSVALUE64
        loadq [slot], scratchRegister
        storeq scratchRegister, [destBuffer, destOffsetReg, 8]
        loadpFromInstruction(profileOffset, destOffsetReg)
        valueProfile(scratchRegister, destOffsetReg)
    else
        loadi PayloadOffset[slot], scratchRegister
        storei scratchRegister, PayloadOffset[destBuffer, destOffsetReg, 8]
        loadi TagOffset[slot], slot
        storei slot, TagOffset[destBuffer, destOffsetReg, 8]
        loadpFromInstruction(profileOffset, destOffsetReg)
        valueProfile(slot, scratchRegister, destOffsetReg)
    end
end

macro moveJSValueFromRegister(value, destBuffer, destOffsetReg, profileOffset)
    storeq value, [destBuffer, destOffsetReg, 8]
    loadpFromInstruction(profileOffset, destOffsetReg)
    valueProfile(value, destOffsetReg)
end

macro moveJSValueFromRegisters(tag, payload, destBuffer, destOffsetReg, profileOffset)
    storei tag, TagOffset[destBuffer, destOffsetReg, 8]
    storei payload, PayloadOffset[destBuffer, destOffsetReg, 8]
    loadpFromInstruction(profileOffset, destOffsetReg)
    valueProfile(tag, payload, destOffsetReg)
end

_llint_op_resolve_global_property:
    traceExecution()
    getResolveOperation(3, t0)
    loadp CodeBlock[cfr], t1
    loadp CodeBlock::m_globalObject[t1], t1
    loadp ResolveOperation::m_structure[t0], t2
    bpneq JSCell::m_structure[t1], t2, .llint_op_resolve_local
    loadis ResolveOperation::m_offset[t0], t0
    if JSVALUE64
        loadPropertyAtVariableOffsetKnownNotInline(t0, t1, t2)
        loadisFromInstruction(1, t0)
        moveJSValueFromRegister(t2, cfr, t0, 4)
    else
        loadPropertyAtVariableOffsetKnownNotInline(t0, t1, t2, t3)
        loadisFromInstruction(1, t0)
        moveJSValueFromRegisters(t2, t3, cfr, t0, 4)
    end
    dispatch(5)

_llint_op_resolve_global_var:
    traceExecution()
    getResolveOperation(3, t0)
    loadp ResolveOperation::m_registerAddress[t0], t0
    loadisFromInstruction(1, t1)
    moveJSValueFromSlot(t0, cfr, t1, 4, t3)
    dispatch(5)

macro resolveScopedVarBody(resolveOperations)
    # First ResolveOperation is to skip scope chain nodes
    getScope(macro(dest)
                 loadp ScopeChain + PayloadOffset[cfr], dest
             end,
             ResolveOperation::m_scopesToSkip[resolveOperations], t1, t2)
    loadp JSVariableObject::m_registers[t1], t1 # t1 now contains the activation registers
    
    # Second ResolveOperation tells us what offset to use
    loadis ResolveOperation::m_offset + sizeof ResolveOperation[resolveOperations], t2
    loadisFromInstruction(1, t3)
    moveJSValue(t1, t2, cfr, t3, 4, t0)
end

_llint_op_resolve_scoped_var:
    traceExecution()
    getResolveOperation(3, t0)
    resolveScopedVarBody(t0)
    dispatch(5)
    
_llint_op_resolve_scoped_var_on_top_scope:
    traceExecution()
    getResolveOperation(3, t0)

    # Load destination index
    loadisFromInstruction(1, t3)

    # We know we want the top scope chain entry
    loadp ScopeChain + PayloadOffset[cfr], t1
    loadp JSVariableObject::m_registers[t1], t1 # t1 now contains the activation registers
    
    # Second ResolveOperation tells us what offset to use
    loadis ResolveOperation::m_offset + sizeof ResolveOperation[t0], t2

    moveJSValue(t1, t2, cfr, t3, 4, t0)
    dispatch(5)

_llint_op_resolve_scoped_var_with_top_scope_check:
    traceExecution()
    getResolveOperation(3, t0)
    # First ResolveOperation tells us what register to check
    loadis ResolveOperation::m_activationRegister[t0], t1

    loadp PayloadOffset[cfr, t1, 8], t1

    getScope(macro(dest)
                 btpz t1, .scopeChainNotCreated
                     loadp JSScope::m_next[t1], dest
                 jmp .done
                 .scopeChainNotCreated:
                     loadp ScopeChain + PayloadOffset[cfr], dest
                 .done:
             end, 
             # Second ResolveOperation tells us how many more nodes to skip
             ResolveOperation::m_scopesToSkip + sizeof ResolveOperation[t0], t1, t2)
    loadp JSVariableObject::m_registers[t1], t1 # t1 now contains the activation registers
    
    # Third operation tells us what offset to use
    loadis ResolveOperation::m_offset + 2 * sizeof ResolveOperation[t0], t2
    loadisFromInstruction(1, t3)
    moveJSValue(t1, t2, cfr, t3, 4, t0)
    dispatch(5)

_llint_op_resolve:
.llint_op_resolve_local:
    traceExecution()
    getResolveOperation(3, t0)
    btpz t0, .noInstructions
    loadis ResolveOperation::m_operation[t0], t1
    bineq t1, ResolveOperationSkipScopes, .notSkipScopes
        resolveScopedVarBody(t0)
        dispatch(5)
.notSkipScopes:
    bineq t1, ResolveOperationGetAndReturnGlobalVar, .notGetAndReturnGlobalVar
        loadp ResolveOperation::m_registerAddress[t0], t0
        loadisFromInstruction(1, t1)
        moveJSValueFromSlot(t0, cfr, t1, 4, t3)
        dispatch(5)
.notGetAndReturnGlobalVar:

.noInstructions:
    callSlowPath(_llint_slow_path_resolve)
    dispatch(5)

_llint_op_resolve_base_to_global:
    traceExecution()
    loadp CodeBlock[cfr], t1
    loadp CodeBlock::m_globalObject[t1], t1
    loadisFromInstruction(1, t3)
    if JSVALUE64
        moveJSValueFromRegister(t1, cfr, t3, 6)
    else
        move CellTag, t2
        moveJSValueFromRegisters(t2, t1, cfr, t3, 6)
    end
    dispatch(7)

_llint_op_resolve_base_to_global_dynamic:
    jmp _llint_op_resolve_base

_llint_op_resolve_base_to_scope:
    traceExecution()
    getResolveOperation(4, t0)
    # First ResolveOperation is to skip scope chain nodes
    getScope(macro(dest)
                 loadp ScopeChain + PayloadOffset[cfr], dest
             end,
             ResolveOperation::m_scopesToSkip[t0], t1, t2)
    loadisFromInstruction(1, t3)
    if JSVALUE64
        moveJSValueFromRegister(t1, cfr, t3, 6)
    else
        move CellTag, t2
        moveJSValueFromRegisters(t2, t1, cfr, t3, 6)
    end
    dispatch(7)

_llint_op_resolve_base_to_scope_with_top_scope_check:
    traceExecution()
    getResolveOperation(4, t0)
    # First ResolveOperation tells us what register to check
    loadis ResolveOperation::m_activationRegister[t0], t1

    loadp PayloadOffset[cfr, t1, 8], t1

    getScope(macro(dest)
                 btpz t1, .scopeChainNotCreated
                     loadp JSScope::m_next[t1], dest
                 jmp .done
                 .scopeChainNotCreated:
                     loadp ScopeChain + PayloadOffset[cfr], dest
                 .done:
             end, 
             # Second ResolveOperation tells us how many more nodes to skip
             ResolveOperation::m_scopesToSkip + sizeof ResolveOperation[t0], t1, t2)

    loadisFromInstruction(1, t3)
    if JSVALUE64
        moveJSValueFromRegister(t1, cfr, t3, 6)
    else
        move CellTag, t2
        moveJSValueFromRegisters(t2, t1, cfr, t3, 6)
    end
    dispatch(7)

_llint_op_resolve_base:
    traceExecution()
    callSlowPath(_llint_slow_path_resolve_base)
    dispatch(7)

macro interpretResolveWithBase(opcodeLength, slowPath)
    traceExecution()
    getResolveOperation(4, t0)
    btpz t0, .slowPath

    loadp ScopeChain[cfr], t3
    # Get the base
    loadis ResolveOperation::m_operation[t0], t2

    bineq t2, ResolveOperationSkipScopes, .notSkipScopes
        getScope(macro(dest) move t3, dest end,
                 ResolveOperation::m_scopesToSkip[t0], t1, t2)
        move t1, t3
        addp sizeof ResolveOperation, t0, t0
        jmp .haveCorrectScope

    .notSkipScopes:

    bineq t2, ResolveOperationSkipTopScopeNode, .notSkipTopScopeNode
        loadis ResolveOperation::m_activationRegister[t0], t1
        loadp PayloadOffset[cfr, t1, 8], t1

        getScope(macro(dest)
                     btpz t1, .scopeChainNotCreated
                         loadp JSScope::m_next[t1], dest
                     jmp .done
                     .scopeChainNotCreated:
                         loadp ScopeChain + PayloadOffset[cfr], dest
                     .done:
                 end,
                 sizeof ResolveOperation + ResolveOperation::m_scopesToSkip[t0], t1, t2)
        move t1, t3
        # We've handled two opcodes here
        addp 2 * sizeof ResolveOperation, t0, t0

    .notSkipTopScopeNode:

    .haveCorrectScope:

    # t3 now contains the correct Scope
    # t0 contains a pointer to the current ResolveOperation

    loadis ResolveOperation::m_operation[t0], t2
    # t2 contains the next instruction

    loadisFromInstruction(1, t1)
    # t1 now contains the index for the base register

    bineq t2, ResolveOperationSetBaseToScope, .notSetBaseToScope
        if JSVALUE64
            storeq t3, [cfr, t1, 8]
        else
            storei t3, PayloadOffset[cfr, t1, 8]
            storei CellTag, TagOffset[cfr, t1, 8]
        end
        jmp .haveSetBase

    .notSetBaseToScope:

    bineq t2, ResolveOperationSetBaseToUndefined, .notSetBaseToUndefined
        if JSVALUE64
            storeq ValueUndefined, [cfr, t1, 8]
        else
            storei 0, PayloadOffset[cfr, t1, 8]
            storei UndefinedTag, TagOffset[cfr, t1, 8]
        end
        jmp .haveSetBase

    .notSetBaseToUndefined:
    bineq t2, ResolveOperationSetBaseToGlobal, .slowPath
        loadp JSCell::m_structure[t3], t2
        loadp Structure::m_globalObject[t2], t2
        if JSVALUE64
            storeq t2, [cfr, t1, 8]
        else
            storei t2, PayloadOffset[cfr, t1, 8]
            storei CellTag, TagOffset[cfr, t1, 8]
        end

    .haveSetBase:

    # Get the value

    # Load the operation into t2
    loadis ResolveOperation::m_operation + sizeof ResolveOperation[t0], t2

    # Load the index for the value register into t1
    loadisFromInstruction(2, t1)

    bineq t2, ResolveOperationGetAndReturnScopedVar, .notGetAndReturnScopedVar
        loadp JSVariableObject::m_registers[t3], t3 # t3 now contains the activation registers

        # Second ResolveOperation tells us what offset to use
        loadis ResolveOperation::m_offset + sizeof ResolveOperation[t0], t2
        moveJSValue(t3, t2, cfr, t1, opcodeLength - 1, t0)
        dispatch(opcodeLength)

    .notGetAndReturnScopedVar:
    bineq t2, ResolveOperationGetAndReturnGlobalProperty, .slowPath
        callSlowPath(slowPath)
        dispatch(opcodeLength)

.slowPath:
    callSlowPath(slowPath)
    dispatch(opcodeLength)
end

_llint_op_resolve_with_base:
    interpretResolveWithBase(7, _llint_slow_path_resolve_with_base)


_llint_op_resolve_with_this:
    interpretResolveWithBase(6, _llint_slow_path_resolve_with_this)


macro withInlineStorage(object, propertyStorage, continuation)
    # Indicate that the object is the property storage, and that the
    # property storage register is unused.
    continuation(object, propertyStorage)
end

macro withOutOfLineStorage(object, propertyStorage, continuation)
    loadp JSObject::m_butterfly[object], propertyStorage
    # Indicate that the propertyStorage register now points to the
    # property storage, and that the object register may be reused
    # if the object pointer is not needed anymore.
    continuation(propertyStorage, object)
end


_llint_op_del_by_id:
    traceExecution()
    callSlowPath(_llint_slow_path_del_by_id)
    dispatch(4)


_llint_op_del_by_val:
    traceExecution()
    callSlowPath(_llint_slow_path_del_by_val)
    dispatch(4)


_llint_op_put_by_index:
    traceExecution()
    callSlowPath(_llint_slow_path_put_by_index)
    dispatch(4)


_llint_op_put_getter_setter:
    traceExecution()
    callSlowPath(_llint_slow_path_put_getter_setter)
    dispatch(5)


_llint_op_jtrue:
    traceExecution()
    jumpTrueOrFalse(
        macro (value, target) btinz value, target end,
        _llint_slow_path_jtrue)


_llint_op_jfalse:
    traceExecution()
    jumpTrueOrFalse(
        macro (value, target) btiz value, target end,
        _llint_slow_path_jfalse)


_llint_op_jless:
    traceExecution()
    compare(
        macro (left, right, target) bilt left, right, target end,
        macro (left, right, target) bdlt left, right, target end,
        _llint_slow_path_jless)


_llint_op_jnless:
    traceExecution()
    compare(
        macro (left, right, target) bigteq left, right, target end,
        macro (left, right, target) bdgtequn left, right, target end,
        _llint_slow_path_jnless)


_llint_op_jgreater:
    traceExecution()
    compare(
        macro (left, right, target) bigt left, right, target end,
        macro (left, right, target) bdgt left, right, target end,
        _llint_slow_path_jgreater)


_llint_op_jngreater:
    traceExecution()
    compare(
        macro (left, right, target) bilteq left, right, target end,
        macro (left, right, target) bdltequn left, right, target end,
        _llint_slow_path_jngreater)


_llint_op_jlesseq:
    traceExecution()
    compare(
        macro (left, right, target) bilteq left, right, target end,
        macro (left, right, target) bdlteq left, right, target end,
        _llint_slow_path_jlesseq)


_llint_op_jnlesseq:
    traceExecution()
    compare(
        macro (left, right, target) bigt left, right, target end,
        macro (left, right, target) bdgtun left, right, target end,
        _llint_slow_path_jnlesseq)


_llint_op_jgreatereq:
    traceExecution()
    compare(
        macro (left, right, target) bigteq left, right, target end,
        macro (left, right, target) bdgteq left, right, target end,
        _llint_slow_path_jgreatereq)


_llint_op_jngreatereq:
    traceExecution()
    compare(
        macro (left, right, target) bilt left, right, target end,
        macro (left, right, target) bdltun left, right, target end,
        _llint_slow_path_jngreatereq)


_llint_op_loop_hint:
    traceExecution()
    loadp JITStackFrame::vm[sp], t1
    loadb VM::watchdog+Watchdog::m_timerDidFire[t1], t0
    btbnz t0, .handleWatchdogTimer
.afterWatchdogTimerCheck:
    checkSwitchToJITForLoop()
    dispatch(1)
.handleWatchdogTimer:
    callWatchdogTimerHandler(.throwHandler)
    jmp .afterWatchdogTimerCheck
.throwHandler:
    jmp _llint_throw_from_slow_path_trampoline

_llint_op_switch_string:
    traceExecution()
    callSlowPath(_llint_slow_path_switch_string)
    dispatch(0)


_llint_op_new_func_exp:
    traceExecution()
    callSlowPath(_llint_slow_path_new_func_exp)
    dispatch(3)


_llint_op_call:
    traceExecution()
    arrayProfileForCall()
    doCall(_llint_slow_path_call)


_llint_op_construct:
    traceExecution()
    doCall(_llint_slow_path_construct)


_llint_op_call_varargs:
    traceExecution()
    slowPathForCall(6, _llint_slow_path_call_varargs)


_llint_op_call_eval:
    traceExecution()
    
    # Eval is executed in one of two modes:
    #
    # 1) We find that we're really invoking eval() in which case the
    #    execution is perfomed entirely inside the slow_path, and it
    #    returns the PC of a function that just returns the return value
    #    that the eval returned.
    #
    # 2) We find that we're invoking something called eval() that is not
    #    the real eval. Then the slow_path returns the PC of the thing to
    #    call, and we call it.
    #
    # This allows us to handle two cases, which would require a total of
    # up to four pieces of state that cannot be easily packed into two
    # registers (C functions can return up to two registers, easily):
    #
    # - The call frame register. This may or may not have been modified
    #   by the slow_path, but the convention is that it returns it. It's not
    #   totally clear if that's necessary, since the cfr is callee save.
    #   But that's our style in this here interpreter so we stick with it.
    #
    # - A bit to say if the slow_path successfully executed the eval and has
    #   the return value, or did not execute the eval but has a PC for us
    #   to call.
    #
    # - Either:
    #   - The JS return value (two registers), or
    #
    #   - The PC to call.
    #
    # It turns out to be easier to just always have this return the cfr
    # and a PC to call, and that PC may be a dummy thunk that just
    # returns the JS value that the eval returned.
    
    slowPathForCall(4, _llint_slow_path_call_eval)


_llint_generic_return_point:
    dispatchAfterCall()


_llint_op_strcat:
    traceExecution()
    callSlowPath(_llint_slow_path_strcat)
    dispatch(4)


_llint_op_get_pnames:
    traceExecution()
    callSlowPath(_llint_slow_path_get_pnames)
    dispatch(0) # The slow_path either advances the PC or jumps us to somewhere else.


_llint_op_push_with_scope:
    traceExecution()
    callSlowPath(_llint_slow_path_push_with_scope)
    dispatch(2)


_llint_op_pop_scope:
    traceExecution()
    callSlowPath(_llint_slow_path_pop_scope)
    dispatch(1)


_llint_op_push_name_scope:
    traceExecution()
    callSlowPath(_llint_slow_path_push_name_scope)
    dispatch(4)


_llint_op_throw:
    traceExecution()
    callSlowPath(_llint_slow_path_throw)
    dispatch(2)


_llint_op_throw_static_error:
    traceExecution()
    callSlowPath(_llint_slow_path_throw_static_error)
    dispatch(3)


_llint_op_profile_will_call:
    traceExecution()
    callSlowPath(_llint_slow_path_profile_will_call)
    dispatch(2)


_llint_op_profile_did_call:
    traceExecution()
    callSlowPath(_llint_slow_path_profile_did_call)
    dispatch(2)


_llint_op_debug:
    traceExecution()
    callSlowPath(_llint_slow_path_debug)
    dispatch(5)


_llint_native_call_trampoline:
    nativeCallTrampoline(NativeExecutable::m_function)


_llint_native_construct_trampoline:
    nativeCallTrampoline(NativeExecutable::m_constructor)


# Lastly, make sure that we can link even though we don't support all opcodes.
# These opcodes should never arise when using LLInt or either JIT. We assert
# as much.

macro notSupported()
    if ASSERT_ENABLED
        crash()
    else
        # We should use whatever the smallest possible instruction is, just to
        # ensure that there is a gap between instruction labels. If multiple
        # smallest instructions exist, we should pick the one that is most
        # likely result in execution being halted. Currently that is the break
        # instruction on all architectures we're interested in. (Break is int3
        # on Intel, which is 1 byte, and bkpt on ARMv7, which is 2 bytes.)
        break
    end
end

_llint_op_get_by_id_chain:
    notSupported()

_llint_op_get_by_id_custom_chain:
    notSupported()

_llint_op_get_by_id_custom_proto:
    notSupported()

_llint_op_get_by_id_custom_self:
    notSupported()

_llint_op_get_by_id_generic:
    notSupported()

_llint_op_get_by_id_getter_chain:
    notSupported()

_llint_op_get_by_id_getter_proto:
    notSupported()

_llint_op_get_by_id_getter_self:
    notSupported()

_llint_op_get_by_id_proto:
    notSupported()

_llint_op_get_by_id_self:
    notSupported()

_llint_op_get_string_length:
    notSupported()

_llint_op_put_by_id_generic:
    notSupported()

_llint_op_put_by_id_replace:
    notSupported()

_llint_op_put_by_id_transition:
    notSupported()

_llint_op_init_global_const_nop:
    dispatch(5)

# Indicate the end of LLInt.
_llint_end:
    crash()

