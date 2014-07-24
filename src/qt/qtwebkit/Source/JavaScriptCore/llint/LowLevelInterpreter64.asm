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


# Some value representation constants.
const TagBitTypeOther = 0x2
const TagBitBool      = 0x4
const TagBitUndefined = 0x8
const ValueEmpty      = 0x0
const ValueFalse      = TagBitTypeOther | TagBitBool
const ValueTrue       = TagBitTypeOther | TagBitBool | 1
const ValueUndefined  = TagBitTypeOther | TagBitUndefined
const ValueNull       = TagBitTypeOther

# Utilities.
macro jumpToInstruction()
    jmp [PB, PC, 8]
end

macro dispatch(advance)
    addp advance, PC
    jumpToInstruction()
end

macro dispatchInt(advance)
    addi advance, PC
    jumpToInstruction()
end

macro dispatchIntIndirect(offset)
    dispatchInt(offset * 8[PB, PC, 8])
end

macro dispatchAfterCall()
    loadi ArgumentCount + TagOffset[cfr], PC
    loadp CodeBlock[cfr], PB
    loadp CodeBlock::m_instructions[PB], PB
    jumpToInstruction()
end

macro cCall2(function, arg1, arg2)
    if X86_64
        move arg1, t5
        move arg2, t4
        call function
    elsif C_LOOP
        cloopCallSlowPath function, arg1, arg2
    else
        error
    end
end

# This barely works. arg3 and arg4 should probably be immediates.
macro cCall4(function, arg1, arg2, arg3, arg4)
    if X86_64
        move arg1, t5
        move arg2, t4
        move arg3, t1
        move arg4, t2
        call function
    elsif C_LOOP
        error
    else
        error
    end
end

macro prepareStateForCCall()
    leap [PB, PC, 8], PC
    move PB, t3
end

macro restoreStateAfterCCall()
    move t0, PC
    move t1, cfr
    move t3, PB
    subp PB, PC
    rshiftp 3, PC
end

macro callSlowPath(slowPath)
    prepareStateForCCall()
    cCall2(slowPath, cfr, PC)
    restoreStateAfterCCall()
end

macro traceOperand(fromWhere, operand)
    prepareStateForCCall()
    cCall4(_llint_trace_operand, cfr, PC, fromWhere, operand)
    restoreStateAfterCCall()
end

macro traceValue(fromWhere, operand)
    prepareStateForCCall()
    cCall4(_llint_trace_value, cfr, PC, fromWhere, operand)
    restoreStateAfterCCall()
end

# Call a slow path for call call opcodes.
macro callCallSlowPath(advance, slowPath, action)
    addi advance, PC, t0
    storei t0, ArgumentCount + TagOffset[cfr]
    prepareStateForCCall()
    cCall2(slowPath, cfr, PC)
    move t1, cfr
    action(t0)
end

macro callWatchdogTimerHandler(throwHandler)
    storei PC, ArgumentCount + TagOffset[cfr]
    prepareStateForCCall()
    cCall2(_llint_slow_path_handle_watchdog_timer, cfr, PC)
    move t1, cfr
    btpnz t0, throwHandler
    move t3, PB
    loadi ArgumentCount + TagOffset[cfr], PC
end

macro checkSwitchToJITForLoop()
    checkSwitchToJIT(
        1,
        macro()
            storei PC, ArgumentCount + TagOffset[cfr]
            prepareStateForCCall()
            cCall2(_llint_loop_osr, cfr, PC)
            move t1, cfr
            btpz t0, .recover
            jmp t0
        .recover:
            move t3, PB
            loadi ArgumentCount + TagOffset[cfr], PC
        end)
end

# Index and value must be different registers. Index may be clobbered.
macro loadConstantOrVariable(index, value)
    bpgteq index, FirstConstantRegisterIndex, .constant
    loadq [cfr, index, 8], value
    jmp .done
.constant:
    loadp CodeBlock[cfr], value
    loadp CodeBlock::m_constantRegisters + VectorBufferOffset[value], value
    subp FirstConstantRegisterIndex, index
    loadq [value, index, 8], value
.done:
end

macro loadConstantOrVariableInt32(index, value, slow)
    loadConstantOrVariable(index, value)
    bqb value, tagTypeNumber, slow
end

macro loadConstantOrVariableCell(index, value, slow)
    loadConstantOrVariable(index, value)
    btqnz value, tagMask, slow
end

macro writeBarrier(value)
    # Nothing to do, since we don't have a generational or incremental collector.
end

macro valueProfile(value, profile)
    if VALUE_PROFILER
        storeq value, ValueProfile::m_buckets[profile]
    end
end


# Entrypoints into the interpreter.

# Expects that CodeBlock is in t1, which is what prologue() leaves behind.
macro functionArityCheck(doneLabel, slow_path)
    loadi PayloadOffset + ArgumentCount[cfr], t0
    biaeq t0, CodeBlock::m_numParameters[t1], doneLabel
    prepareStateForCCall()
    cCall2(slow_path, cfr, PC)   # This slow_path has a simple protocol: t0 = 0 => no error, t0 != 0 => error
    move t1, cfr
    btiz t0, .continue
    loadp JITStackFrame::vm[sp], t1
    loadp VM::callFrameForThrow[t1], t0
    jmp VM::targetMachinePCForThrow[t1]
.continue:
    # Reload CodeBlock and reset PC, since the slow_path clobbered them.
    loadp CodeBlock[cfr], t1
    loadp CodeBlock::m_instructions[t1], PB
    move 0, PC
    jmp doneLabel
end


# Instruction implementations

_llint_op_enter:
    traceExecution()
    loadp CodeBlock[cfr], t2                // t2<CodeBlock> = cfr.CodeBlock
    loadi CodeBlock::m_numVars[t2], t2      // t2<size_t> = t2<CodeBlock>.m_numVars
    btiz t2, .opEnterDone
    move ValueUndefined, t0
.opEnterLoop:
    subi 1, t2
    storeq t0, [cfr, t2, 8]
    btinz t2, .opEnterLoop
.opEnterDone:
    dispatch(1)


_llint_op_create_activation:
    traceExecution()
    loadisFromInstruction(1, t0)
    bqneq [cfr, t0, 8], ValueEmpty, .opCreateActivationDone
    callSlowPath(_llint_slow_path_create_activation)
.opCreateActivationDone:
    dispatch(2)


_llint_op_init_lazy_reg:
    traceExecution()
    loadisFromInstruction(1, t0)
    storeq ValueEmpty, [cfr, t0, 8]
    dispatch(2)


_llint_op_create_arguments:
    traceExecution()
    loadisFromInstruction(1, t0)
    bqneq [cfr, t0, 8], ValueEmpty, .opCreateArgumentsDone
    callSlowPath(_llint_slow_path_create_arguments)
.opCreateArgumentsDone:
    dispatch(2)


_llint_op_create_this:
    traceExecution()
    loadisFromInstruction(2, t0)
    loadp [cfr, t0, 8], t0
    loadp JSFunction::m_allocationProfile + ObjectAllocationProfile::m_allocator[t0], t1
    loadp JSFunction::m_allocationProfile + ObjectAllocationProfile::m_structure[t0], t2
    btpz t1, .opCreateThisSlow
    allocateJSObject(t1, t2, t0, t3, .opCreateThisSlow)
    loadisFromInstruction(1, t1)
    storeq t0, [cfr, t1, 8]
    dispatch(4)

.opCreateThisSlow:
    callSlowPath(_llint_slow_path_create_this)
    dispatch(4)


_llint_op_get_callee:
    traceExecution()
    loadisFromInstruction(1, t0)
    loadpFromInstruction(2, t2)
    loadp Callee[cfr], t1
    valueProfile(t1, t2)
    storep t1, [cfr, t0, 8]
    dispatch(3)


_llint_op_convert_this:
    traceExecution()
    loadisFromInstruction(1, t0)
    loadq [cfr, t0, 8], t0
    btqnz t0, tagMask, .opConvertThisSlow
    loadp JSCell::m_structure[t0], t0
    bbb Structure::m_typeInfo + TypeInfo::m_type[t0], ObjectType, .opConvertThisSlow
    loadpFromInstruction(2, t1)
    valueProfile(t0, t1)
    dispatch(3)

.opConvertThisSlow:
    callSlowPath(_llint_slow_path_convert_this)
    dispatch(3)


_llint_op_new_object:
    traceExecution()
    loadpFromInstruction(3, t0)
    loadp ObjectAllocationProfile::m_allocator[t0], t1
    loadp ObjectAllocationProfile::m_structure[t0], t2
    allocateJSObject(t1, t2, t0, t3, .opNewObjectSlow)
    loadisFromInstruction(1, t1)
    storeq t0, [cfr, t1, 8]
    dispatch(4)

.opNewObjectSlow:
    callSlowPath(_llint_slow_path_new_object)
    dispatch(4)


_llint_op_mov:
    traceExecution()
    loadisFromInstruction(2, t1)
    loadisFromInstruction(1, t0)
    loadConstantOrVariable(t1, t2)
    storeq t2, [cfr, t0, 8]
    dispatch(3)


_llint_op_not:
    traceExecution()
    loadisFromInstruction(2, t0)
    loadisFromInstruction(1, t1)
    loadConstantOrVariable(t0, t2)
    xorq ValueFalse, t2
    btqnz t2, ~1, .opNotSlow
    xorq ValueTrue, t2
    storeq t2, [cfr, t1, 8]
    dispatch(3)

.opNotSlow:
    callSlowPath(_llint_slow_path_not)
    dispatch(3)


macro equalityComparison(integerComparison, slowPath)
    traceExecution()
    loadisFromInstruction(3, t0)
    loadisFromInstruction(2, t2)
    loadisFromInstruction(1, t3)
    loadConstantOrVariableInt32(t0, t1, .slow)
    loadConstantOrVariableInt32(t2, t0, .slow)
    integerComparison(t0, t1, t0)
    orq ValueFalse, t0
    storeq t0, [cfr, t3, 8]
    dispatch(4)

.slow:
    callSlowPath(slowPath)
    dispatch(4)
end

_llint_op_eq:
    equalityComparison(
        macro (left, right, result) cieq left, right, result end,
        _llint_slow_path_eq)


_llint_op_neq:
    equalityComparison(
        macro (left, right, result) cineq left, right, result end,
        _llint_slow_path_neq)


macro equalNullComparison()
    loadisFromInstruction(2, t0)
    loadq [cfr, t0, 8], t0
    btqnz t0, tagMask, .immediate
    loadp JSCell::m_structure[t0], t2
    btbnz Structure::m_typeInfo + TypeInfo::m_flags[t2], MasqueradesAsUndefined, .masqueradesAsUndefined
    move 0, t0
    jmp .done
.masqueradesAsUndefined:
    loadp CodeBlock[cfr], t0
    loadp CodeBlock::m_globalObject[t0], t0
    cpeq Structure::m_globalObject[t2], t0, t0
    jmp .done
.immediate:
    andq ~TagBitUndefined, t0
    cqeq t0, ValueNull, t0
.done:
end

_llint_op_eq_null:
    traceExecution()
    equalNullComparison()
    loadisFromInstruction(1, t1)
    orq ValueFalse, t0
    storeq t0, [cfr, t1, 8]
    dispatch(3)


_llint_op_neq_null:
    traceExecution()
    equalNullComparison()
    loadisFromInstruction(1, t1)
    xorq ValueTrue, t0
    storeq t0, [cfr, t1, 8]
    dispatch(3)


macro strictEq(equalityOperation, slowPath)
    traceExecution()
    loadisFromInstruction(3, t0)
    loadisFromInstruction(2, t2)
    loadConstantOrVariable(t0, t1)
    loadConstantOrVariable(t2, t0)
    move t0, t2
    orq t1, t2
    btqz t2, tagMask, .slow
    bqaeq t0, tagTypeNumber, .leftOK
    btqnz t0, tagTypeNumber, .slow
.leftOK:
    bqaeq t1, tagTypeNumber, .rightOK
    btqnz t1, tagTypeNumber, .slow
.rightOK:
    equalityOperation(t0, t1, t0)
    loadisFromInstruction(1, t1)
    orq ValueFalse, t0
    storeq t0, [cfr, t1, 8]
    dispatch(4)

.slow:
    callSlowPath(slowPath)
    dispatch(4)
end

_llint_op_stricteq:
    strictEq(
        macro (left, right, result) cqeq left, right, result end,
        _llint_slow_path_stricteq)


_llint_op_nstricteq:
    strictEq(
        macro (left, right, result) cqneq left, right, result end,
        _llint_slow_path_nstricteq)


macro preOp(arithmeticOperation, slowPath)
    traceExecution()
    loadisFromInstruction(1, t0)
    loadq [cfr, t0, 8], t1
    bqb t1, tagTypeNumber, .slow
    arithmeticOperation(t1, .slow)
    orq tagTypeNumber, t1
    storeq t1, [cfr, t0, 8]
    dispatch(2)

.slow:
    callSlowPath(slowPath)
    dispatch(2)
end

_llint_op_inc:
    preOp(
        macro (value, slow) baddio 1, value, slow end,
        _llint_slow_path_pre_inc)


_llint_op_dec:
    preOp(
        macro (value, slow) bsubio 1, value, slow end,
        _llint_slow_path_pre_dec)


_llint_op_to_number:
    traceExecution()
    loadisFromInstruction(2, t0)
    loadisFromInstruction(1, t1)
    loadConstantOrVariable(t0, t2)
    bqaeq t2, tagTypeNumber, .opToNumberIsImmediate
    btqz t2, tagTypeNumber, .opToNumberSlow
.opToNumberIsImmediate:
    storeq t2, [cfr, t1, 8]
    dispatch(3)

.opToNumberSlow:
    callSlowPath(_llint_slow_path_to_number)
    dispatch(3)


_llint_op_negate:
    traceExecution()
    loadisFromInstruction(2, t0)
    loadisFromInstruction(1, t1)
    loadConstantOrVariable(t0, t2)
    bqb t2, tagTypeNumber, .opNegateNotInt
    btiz t2, 0x7fffffff, .opNegateSlow
    negi t2
    orq tagTypeNumber, t2
    storeq t2, [cfr, t1, 8]
    dispatch(3)
.opNegateNotInt:
    btqz t2, tagTypeNumber, .opNegateSlow
    xorq 0x8000000000000000, t2
    storeq t2, [cfr, t1, 8]
    dispatch(3)

.opNegateSlow:
    callSlowPath(_llint_slow_path_negate)
    dispatch(3)


macro binaryOpCustomStore(integerOperationAndStore, doubleOperation, slowPath)
    loadisFromInstruction(3, t0)
    loadisFromInstruction(2, t2)
    loadConstantOrVariable(t0, t1)
    loadConstantOrVariable(t2, t0)
    bqb t0, tagTypeNumber, .op1NotInt
    bqb t1, tagTypeNumber, .op2NotInt
    loadisFromInstruction(1, t2)
    integerOperationAndStore(t1, t0, .slow, t2)
    dispatch(5)

.op1NotInt:
    # First operand is definitely not an int, the second operand could be anything.
    btqz t0, tagTypeNumber, .slow
    bqaeq t1, tagTypeNumber, .op1NotIntOp2Int
    btqz t1, tagTypeNumber, .slow
    addq tagTypeNumber, t1
    fq2d t1, ft1
    jmp .op1NotIntReady
.op1NotIntOp2Int:
    ci2d t1, ft1
.op1NotIntReady:
    loadisFromInstruction(1, t2)
    addq tagTypeNumber, t0
    fq2d t0, ft0
    doubleOperation(ft1, ft0)
    fd2q ft0, t0
    subq tagTypeNumber, t0
    storeq t0, [cfr, t2, 8]
    dispatch(5)

.op2NotInt:
    # First operand is definitely an int, the second is definitely not.
    loadisFromInstruction(1, t2)
    btqz t1, tagTypeNumber, .slow
    ci2d t0, ft0
    addq tagTypeNumber, t1
    fq2d t1, ft1
    doubleOperation(ft1, ft0)
    fd2q ft0, t0
    subq tagTypeNumber, t0
    storeq t0, [cfr, t2, 8]
    dispatch(5)

.slow:
    callSlowPath(slowPath)
    dispatch(5)
end

macro binaryOp(integerOperation, doubleOperation, slowPath)
    binaryOpCustomStore(
        macro (left, right, slow, index)
            integerOperation(left, right, slow)
            orq tagTypeNumber, right
            storeq right, [cfr, index, 8]
        end,
        doubleOperation, slowPath)
end

_llint_op_add:
    traceExecution()
    binaryOp(
        macro (left, right, slow) baddio left, right, slow end,
        macro (left, right) addd left, right end,
        _llint_slow_path_add)


_llint_op_mul:
    traceExecution()
    binaryOpCustomStore(
        macro (left, right, slow, index)
            # Assume t3 is scratchable.
            move right, t3
            bmulio left, t3, slow
            btinz t3, .done
            bilt left, 0, slow
            bilt right, 0, slow
        .done:
            orq tagTypeNumber, t3
            storeq t3, [cfr, index, 8]
        end,
        macro (left, right) muld left, right end,
        _llint_slow_path_mul)


_llint_op_sub:
    traceExecution()
    binaryOp(
        macro (left, right, slow) bsubio left, right, slow end,
        macro (left, right) subd left, right end,
        _llint_slow_path_sub)


_llint_op_div:
    traceExecution()
    binaryOpCustomStore(
        macro (left, right, slow, index)
            # Assume t3 is scratchable.
            btiz left, slow
            bineq left, -1, .notNeg2TwoThe31DivByNeg1
            bieq right, -2147483648, .slow
        .notNeg2TwoThe31DivByNeg1:
            btinz right, .intOK
            bilt left, 0, slow
        .intOK:
            move left, t3
            move right, t0
            cdqi
            idivi t3
            btinz t1, slow
            orq tagTypeNumber, t0
            storeq t0, [cfr, index, 8]
        end,
        macro (left, right) divd left, right end,
        _llint_slow_path_div)


macro bitOp(operation, slowPath, advance)
    loadisFromInstruction(3, t0)
    loadisFromInstruction(2, t2)
    loadisFromInstruction(1, t3)
    loadConstantOrVariable(t0, t1)
    loadConstantOrVariable(t2, t0)
    bqb t0, tagTypeNumber, .slow
    bqb t1, tagTypeNumber, .slow
    operation(t1, t0, .slow)
    orq tagTypeNumber, t0
    storeq t0, [cfr, t3, 8]
    dispatch(advance)

.slow:
    callSlowPath(slowPath)
    dispatch(advance)
end

_llint_op_lshift:
    traceExecution()
    bitOp(
        macro (left, right, slow) lshifti left, right end,
        _llint_slow_path_lshift,
        4)


_llint_op_rshift:
    traceExecution()
    bitOp(
        macro (left, right, slow) rshifti left, right end,
        _llint_slow_path_rshift,
        4)


_llint_op_urshift:
    traceExecution()
    bitOp(
        macro (left, right, slow)
            urshifti left, right
            bilt right, 0, slow
        end,
        _llint_slow_path_urshift,
        4)


_llint_op_bitand:
    traceExecution()
    bitOp(
        macro (left, right, slow) andi left, right end,
        _llint_slow_path_bitand,
        5)


_llint_op_bitxor:
    traceExecution()
    bitOp(
        macro (left, right, slow) xori left, right end,
        _llint_slow_path_bitxor,
        5)


_llint_op_bitor:
    traceExecution()
    bitOp(
        macro (left, right, slow) ori left, right end,
        _llint_slow_path_bitor,
        5)


_llint_op_check_has_instance:
    traceExecution()
    loadisFromInstruction(3, t1)
    loadConstantOrVariableCell(t1, t0, .opCheckHasInstanceSlow)
    loadp JSCell::m_structure[t0], t0
    btbz Structure::m_typeInfo + TypeInfo::m_flags[t0], ImplementsDefaultHasInstance, .opCheckHasInstanceSlow
    dispatch(5)

.opCheckHasInstanceSlow:
    callSlowPath(_llint_slow_path_check_has_instance)
    dispatch(0)


_llint_op_instanceof:
    traceExecution()
    # Actually do the work.
    loadisFromInstruction(3, t0)
    loadisFromInstruction(1, t3)
    loadConstantOrVariableCell(t0, t1, .opInstanceofSlow)
    loadp JSCell::m_structure[t1], t2
    bbb Structure::m_typeInfo + TypeInfo::m_type[t2], ObjectType, .opInstanceofSlow
    loadisFromInstruction(2, t0)
    loadConstantOrVariableCell(t0, t2, .opInstanceofSlow)
    
    # Register state: t1 = prototype, t2 = value
    move 1, t0
.opInstanceofLoop:
    loadp JSCell::m_structure[t2], t2
    loadq Structure::m_prototype[t2], t2
    bqeq t2, t1, .opInstanceofDone
    btqz t2, tagMask, .opInstanceofLoop

    move 0, t0
.opInstanceofDone:
    orq ValueFalse, t0
    storeq t0, [cfr, t3, 8]
    dispatch(4)

.opInstanceofSlow:
    callSlowPath(_llint_slow_path_instanceof)
    dispatch(4)


_llint_op_is_undefined:
    traceExecution()
    loadisFromInstruction(2, t1)
    loadisFromInstruction(1, t2)
    loadConstantOrVariable(t1, t0)
    btqz t0, tagMask, .opIsUndefinedCell
    cqeq t0, ValueUndefined, t3
    orq ValueFalse, t3
    storeq t3, [cfr, t2, 8]
    dispatch(3)
.opIsUndefinedCell:
    loadp JSCell::m_structure[t0], t0
    btbnz Structure::m_typeInfo + TypeInfo::m_flags[t0], MasqueradesAsUndefined, .masqueradesAsUndefined
    move ValueFalse, t1
    storeq t1, [cfr, t2, 8]
    dispatch(3)
.masqueradesAsUndefined:
    loadp CodeBlock[cfr], t1
    loadp CodeBlock::m_globalObject[t1], t1
    cpeq Structure::m_globalObject[t0], t1, t3
    orq ValueFalse, t3
    storeq t3, [cfr, t2, 8]
    dispatch(3)


_llint_op_is_boolean:
    traceExecution()
    loadisFromInstruction(2, t1)
    loadisFromInstruction(1, t2)
    loadConstantOrVariable(t1, t0)
    xorq ValueFalse, t0
    tqz t0, ~1, t0
    orq ValueFalse, t0
    storeq t0, [cfr, t2, 8]
    dispatch(3)


_llint_op_is_number:
    traceExecution()
    loadisFromInstruction(2, t1)
    loadisFromInstruction(1, t2)
    loadConstantOrVariable(t1, t0)
    tqnz t0, tagTypeNumber, t1
    orq ValueFalse, t1
    storeq t1, [cfr, t2, 8]
    dispatch(3)


_llint_op_is_string:
    traceExecution()
    loadisFromInstruction(2, t1)
    loadisFromInstruction(1, t2)
    loadConstantOrVariable(t1, t0)
    btqnz t0, tagMask, .opIsStringNotCell
    loadp JSCell::m_structure[t0], t0
    cbeq Structure::m_typeInfo + TypeInfo::m_type[t0], StringType, t1
    orq ValueFalse, t1
    storeq t1, [cfr, t2, 8]
    dispatch(3)
.opIsStringNotCell:
    storeq ValueFalse, [cfr, t2, 8]
    dispatch(3)


macro loadPropertyAtVariableOffsetKnownNotInline(propertyOffsetAsPointer, objectAndStorage, value)
    assert(macro (ok) bigteq propertyOffsetAsPointer, firstOutOfLineOffset, ok end)
    negp propertyOffsetAsPointer
    loadp JSObject::m_butterfly[objectAndStorage], objectAndStorage
    loadq (firstOutOfLineOffset - 2) * 8[objectAndStorage, propertyOffsetAsPointer, 8], value
end

macro loadPropertyAtVariableOffset(propertyOffsetAsInt, objectAndStorage, value)
    bilt propertyOffsetAsInt, firstOutOfLineOffset, .isInline
    loadp JSObject::m_butterfly[objectAndStorage], objectAndStorage
    negi propertyOffsetAsInt
    sxi2q propertyOffsetAsInt, propertyOffsetAsInt
    jmp .ready
.isInline:
    addp sizeof JSObject - (firstOutOfLineOffset - 2) * 8, objectAndStorage
.ready:
    loadq (firstOutOfLineOffset - 2) * 8[objectAndStorage, propertyOffsetAsInt, 8], value
end

_llint_op_init_global_const:
    traceExecution()
    loadisFromInstruction(2, t1)
    loadpFromInstruction(1, t0)
    loadConstantOrVariable(t1, t2)
    writeBarrier(t2)
    storeq t2, [t0]
    dispatch(5)


_llint_op_init_global_const_check:
    traceExecution()
    loadpFromInstruction(3, t2)
    loadisFromInstruction(2, t1)
    loadpFromInstruction(1, t0)
    btbnz [t2], .opInitGlobalConstCheckSlow
    loadConstantOrVariable(t1, t2)
    writeBarrier(t2)
    storeq t2, [t0]
    dispatch(5)
.opInitGlobalConstCheckSlow:
    callSlowPath(_llint_slow_path_init_global_const_check)
    dispatch(5)

macro getById(getPropertyStorage)
    traceExecution()
    # We only do monomorphic get_by_id caching for now, and we do not modify the
    # opcode. We do, however, allow for the cache to change anytime if fails, since
    # ping-ponging is free. At best we get lucky and the get_by_id will continue
    # to take fast path on the new cache. At worst we take slow path, which is what
    # we would have been doing anyway.
    loadisFromInstruction(2, t0)
    loadpFromInstruction(4, t1)
    loadConstantOrVariableCell(t0, t3, .opGetByIdSlow)
    loadisFromInstruction(5, t2)
    getPropertyStorage(
        t3,
        t0,
        macro (propertyStorage, scratch)
            bpneq JSCell::m_structure[t3], t1, .opGetByIdSlow
            loadisFromInstruction(1, t1)
            loadq [propertyStorage, t2], scratch
            storeq scratch, [cfr, t1, 8]
            loadpFromInstruction(8, t1)
            valueProfile(scratch, t1)
            dispatch(9)
        end)
            
    .opGetByIdSlow:
        callSlowPath(_llint_slow_path_get_by_id)
        dispatch(9)
end

_llint_op_get_by_id:
    getById(withInlineStorage)


_llint_op_get_by_id_out_of_line:
    getById(withOutOfLineStorage)


_llint_op_get_array_length:
    traceExecution()
    loadisFromInstruction(2, t0)
    loadpFromInstruction(4, t1)
    loadConstantOrVariableCell(t0, t3, .opGetArrayLengthSlow)
    loadp JSCell::m_structure[t3], t2
    arrayProfile(t2, t1, t0)
    btiz t2, IsArray, .opGetArrayLengthSlow
    btiz t2, IndexingShapeMask, .opGetArrayLengthSlow
    loadisFromInstruction(1, t1)
    loadpFromInstruction(8, t2)
    loadp JSObject::m_butterfly[t3], t0
    loadi -sizeof IndexingHeader + IndexingHeader::u.lengths.publicLength[t0], t0
    bilt t0, 0, .opGetArrayLengthSlow
    orq tagTypeNumber, t0
    valueProfile(t0, t2)
    storeq t0, [cfr, t1, 8]
    dispatch(9)

.opGetArrayLengthSlow:
    callSlowPath(_llint_slow_path_get_by_id)
    dispatch(9)


_llint_op_get_arguments_length:
    traceExecution()
    loadisFromInstruction(2, t0)
    loadisFromInstruction(1, t1)
    btqnz [cfr, t0, 8], .opGetArgumentsLengthSlow
    loadi ArgumentCount + PayloadOffset[cfr], t2
    subi 1, t2
    orq tagTypeNumber, t2
    storeq t2, [cfr, t1, 8]
    dispatch(4)

.opGetArgumentsLengthSlow:
    callSlowPath(_llint_slow_path_get_arguments_length)
    dispatch(4)


macro putById(getPropertyStorage)
    traceExecution()
    loadisFromInstruction(1, t3)
    loadpFromInstruction(4, t1)
    loadConstantOrVariableCell(t3, t0, .opPutByIdSlow)
    loadisFromInstruction(3, t2)
    getPropertyStorage(
        t0,
        t3,
        macro (propertyStorage, scratch)
            bpneq JSCell::m_structure[t0], t1, .opPutByIdSlow
            loadisFromInstruction(5, t1)
            loadConstantOrVariable(t2, scratch)
            writeBarrier(t0)
            storeq scratch, [propertyStorage, t1]
            dispatch(9)
        end)
end

_llint_op_put_by_id:
    putById(withInlineStorage)

.opPutByIdSlow:
    callSlowPath(_llint_slow_path_put_by_id)
    dispatch(9)


_llint_op_put_by_id_out_of_line:
    putById(withOutOfLineStorage)


macro putByIdTransition(additionalChecks, getPropertyStorage)
    traceExecution()
    loadisFromInstruction(1, t3)
    loadpFromInstruction(4, t1)
    loadConstantOrVariableCell(t3, t0, .opPutByIdSlow)
    loadisFromInstruction(3, t2)
    bpneq JSCell::m_structure[t0], t1, .opPutByIdSlow
    additionalChecks(t1, t3)
    loadisFromInstruction(5, t1)
    getPropertyStorage(
        t0,
        t3,
        macro (propertyStorage, scratch)
            addp t1, propertyStorage, t3
            loadConstantOrVariable(t2, t1)
            writeBarrier(t1)
            storeq t1, [t3]
            loadpFromInstruction(6, t1)
            storep t1, JSCell::m_structure[t0]
            dispatch(9)
        end)
end

macro noAdditionalChecks(oldStructure, scratch)
end

macro structureChainChecks(oldStructure, scratch)
    const protoCell = oldStructure    # Reusing the oldStructure register for the proto
    loadpFromInstruction(7, scratch)
    assert(macro (ok) btpnz scratch, ok end)
    loadp StructureChain::m_vector[scratch], scratch
    assert(macro (ok) btpnz scratch, ok end)
    bqeq Structure::m_prototype[oldStructure], ValueNull, .done
.loop:
    loadq Structure::m_prototype[oldStructure], protoCell
    loadp JSCell::m_structure[protoCell], oldStructure
    bpneq oldStructure, [scratch], .opPutByIdSlow
    addp 8, scratch
    bqneq Structure::m_prototype[oldStructure], ValueNull, .loop
.done:
end

_llint_op_put_by_id_transition_direct:
    putByIdTransition(noAdditionalChecks, withInlineStorage)


_llint_op_put_by_id_transition_direct_out_of_line:
    putByIdTransition(noAdditionalChecks, withOutOfLineStorage)


_llint_op_put_by_id_transition_normal:
    putByIdTransition(structureChainChecks, withInlineStorage)


_llint_op_put_by_id_transition_normal_out_of_line:
    putByIdTransition(structureChainChecks, withOutOfLineStorage)


_llint_op_get_by_val:
    traceExecution()
    loadisFromInstruction(2, t2)
    loadConstantOrVariableCell(t2, t0, .opGetByValSlow)
    loadp JSCell::m_structure[t0], t2
    loadpFromInstruction(4, t3)
    arrayProfile(t2, t3, t1)
    loadisFromInstruction(3, t3)
    loadConstantOrVariableInt32(t3, t1, .opGetByValSlow)
    sxi2q t1, t1
    loadp JSObject::m_butterfly[t0], t3
    andi IndexingShapeMask, t2
    bieq t2, Int32Shape, .opGetByValIsContiguous
    bineq t2, ContiguousShape, .opGetByValNotContiguous
.opGetByValIsContiguous:

    biaeq t1, -sizeof IndexingHeader + IndexingHeader::u.lengths.publicLength[t3], .opGetByValOutOfBounds
    loadisFromInstruction(1, t0)
    loadq [t3, t1, 8], t2
    btqz t2, .opGetByValOutOfBounds
    jmp .opGetByValDone

.opGetByValNotContiguous:
    bineq t2, DoubleShape, .opGetByValNotDouble
    biaeq t1, -sizeof IndexingHeader + IndexingHeader::u.lengths.publicLength[t3], .opGetByValOutOfBounds
    loadis 8[PB, PC, 8], t0
    loadd [t3, t1, 8], ft0
    bdnequn ft0, ft0, .opGetByValOutOfBounds
    fd2q ft0, t2
    subq tagTypeNumber, t2
    jmp .opGetByValDone
    
.opGetByValNotDouble:
    subi ArrayStorageShape, t2
    bia t2, SlowPutArrayStorageShape - ArrayStorageShape, .opGetByValSlow
    biaeq t1, -sizeof IndexingHeader + IndexingHeader::u.lengths.vectorLength[t3], .opGetByValOutOfBounds
    loadisFromInstruction(1, t0)
    loadq ArrayStorage::m_vector[t3, t1, 8], t2
    btqz t2, .opGetByValOutOfBounds

.opGetByValDone:
    storeq t2, [cfr, t0, 8]
    loadpFromInstruction(5, t0)
    valueProfile(t2, t0)
    dispatch(6)

.opGetByValOutOfBounds:
    if VALUE_PROFILER
        loadpFromInstruction(4, t0)
        storeb 1, ArrayProfile::m_outOfBounds[t0]
    end
.opGetByValSlow:
    callSlowPath(_llint_slow_path_get_by_val)
    dispatch(6)


_llint_op_get_argument_by_val:
    # FIXME: At some point we should array profile this. Right now it isn't necessary
    # since the DFG will never turn a get_argument_by_val into a GetByVal.
    traceExecution()
    loadisFromInstruction(2, t0)
    loadisFromInstruction(3, t1)
    btqnz [cfr, t0, 8], .opGetArgumentByValSlow
    loadConstantOrVariableInt32(t1, t2, .opGetArgumentByValSlow)
    addi 1, t2
    loadi ArgumentCount + PayloadOffset[cfr], t1
    biaeq t2, t1, .opGetArgumentByValSlow
    negi t2
    sxi2q t2, t2
    loadisFromInstruction(1, t3)
    loadpFromInstruction(5, t1)
    loadq ThisArgumentOffset[cfr, t2, 8], t0
    storeq t0, [cfr, t3, 8]
    valueProfile(t0, t1)
    dispatch(6)

.opGetArgumentByValSlow:
    callSlowPath(_llint_slow_path_get_argument_by_val)
    dispatch(6)


_llint_op_get_by_pname:
    traceExecution()
    loadisFromInstruction(3, t1)
    loadConstantOrVariable(t1, t0)
    loadisFromInstruction(4, t1)
    assertNotConstant(t1)
    bqneq t0, [cfr, t1, 8], .opGetByPnameSlow
    loadisFromInstruction(2, t2)
    loadisFromInstruction(5, t3)
    loadConstantOrVariableCell(t2, t0, .opGetByPnameSlow)
    assertNotConstant(t3)
    loadq [cfr, t3, 8], t1
    loadp JSCell::m_structure[t0], t2
    bpneq t2, JSPropertyNameIterator::m_cachedStructure[t1], .opGetByPnameSlow
    loadisFromInstruction(6, t3)
    loadi PayloadOffset[cfr, t3, 8], t3
    subi 1, t3
    biaeq t3, JSPropertyNameIterator::m_numCacheableSlots[t1], .opGetByPnameSlow
    bilt t3, JSPropertyNameIterator::m_cachedStructureInlineCapacity[t1], .opGetByPnameInlineProperty
    addi firstOutOfLineOffset, t3
    subi JSPropertyNameIterator::m_cachedStructureInlineCapacity[t1], t3
.opGetByPnameInlineProperty:
    loadPropertyAtVariableOffset(t3, t0, t0)
    loadisFromInstruction(1, t1)
    storeq t0, [cfr, t1, 8]
    dispatch(7)

.opGetByPnameSlow:
    callSlowPath(_llint_slow_path_get_by_pname)
    dispatch(7)


macro contiguousPutByVal(storeCallback)
    biaeq t3, -sizeof IndexingHeader + IndexingHeader::u.lengths.publicLength[t0], .outOfBounds
.storeResult:
    loadisFromInstruction(3, t2)
    storeCallback(t2, t1, [t0, t3, 8])
    dispatch(5)

.outOfBounds:
    biaeq t3, -sizeof IndexingHeader + IndexingHeader::u.lengths.vectorLength[t0], .opPutByValOutOfBounds
    if VALUE_PROFILER
        loadp 32[PB, PC, 8], t2
        storeb 1, ArrayProfile::m_mayStoreToHole[t2]
    end
    addi 1, t3, t2
    storei t2, -sizeof IndexingHeader + IndexingHeader::u.lengths.publicLength[t0]
    jmp .storeResult
end

_llint_op_put_by_val:
    traceExecution()
    loadisFromInstruction(1, t0)
    loadConstantOrVariableCell(t0, t1, .opPutByValSlow)
    loadp JSCell::m_structure[t1], t2
    loadpFromInstruction(4, t3)
    arrayProfile(t2, t3, t0)
    loadisFromInstruction(2, t0)
    loadConstantOrVariableInt32(t0, t3, .opPutByValSlow)
    sxi2q t3, t3
    loadp JSObject::m_butterfly[t1], t0
    andi IndexingShapeMask, t2
    bineq t2, Int32Shape, .opPutByValNotInt32
    contiguousPutByVal(
        macro (operand, scratch, address)
            loadConstantOrVariable(operand, scratch)
            bpb scratch, tagTypeNumber, .opPutByValSlow
            storep scratch, address
        end)

.opPutByValNotInt32:
    bineq t2, DoubleShape, .opPutByValNotDouble
    contiguousPutByVal(
        macro (operand, scratch, address)
            loadConstantOrVariable(operand, scratch)
            bqb scratch, tagTypeNumber, .notInt
            ci2d scratch, ft0
            jmp .ready
        .notInt:
            addp tagTypeNumber, scratch
            fq2d scratch, ft0
            bdnequn ft0, ft0, .opPutByValSlow
        .ready:
            stored ft0, address
        end)

.opPutByValNotDouble:
    bineq t2, ContiguousShape, .opPutByValNotContiguous
    contiguousPutByVal(
        macro (operand, scratch, address)
            loadConstantOrVariable(operand, scratch)
            writeBarrier(scratch)
            storep scratch, address
        end)

.opPutByValNotContiguous:
    bineq t2, ArrayStorageShape, .opPutByValSlow
    biaeq t3, -sizeof IndexingHeader + IndexingHeader::u.lengths.vectorLength[t0], .opPutByValOutOfBounds
    btqz ArrayStorage::m_vector[t0, t3, 8], .opPutByValArrayStorageEmpty
.opPutByValArrayStorageStoreResult:
    loadisFromInstruction(3, t2)
    loadConstantOrVariable(t2, t1)
    writeBarrier(t1)
    storeq t1, ArrayStorage::m_vector[t0, t3, 8]
    dispatch(5)

.opPutByValArrayStorageEmpty:
    if VALUE_PROFILER
        loadpFromInstruction(4, t1)
        storeb 1, ArrayProfile::m_mayStoreToHole[t1]
    end
    addi 1, ArrayStorage::m_numValuesInVector[t0]
    bib t3, -sizeof IndexingHeader + IndexingHeader::u.lengths.publicLength[t0], .opPutByValArrayStorageStoreResult
    addi 1, t3, t1
    storei t1, -sizeof IndexingHeader + IndexingHeader::u.lengths.publicLength[t0]
    jmp .opPutByValArrayStorageStoreResult

.opPutByValOutOfBounds:
    if VALUE_PROFILER
        loadpFromInstruction(4, t0)
        storeb 1, ArrayProfile::m_outOfBounds[t0]
    end
.opPutByValSlow:
    callSlowPath(_llint_slow_path_put_by_val)
    dispatch(5)


_llint_op_jmp:
    traceExecution()
    dispatchIntIndirect(1)


macro jumpTrueOrFalse(conditionOp, slow)
    loadisFromInstruction(1, t1)
    loadConstantOrVariable(t1, t0)
    xorq ValueFalse, t0
    btqnz t0, -1, .slow
    conditionOp(t0, .target)
    dispatch(3)

.target:
    dispatchIntIndirect(2)

.slow:
    callSlowPath(slow)
    dispatch(0)
end


macro equalNull(cellHandler, immediateHandler)
    loadisFromInstruction(1, t0)
    assertNotConstant(t0)
    loadq [cfr, t0, 8], t0
    btqnz t0, tagMask, .immediate
    loadp JSCell::m_structure[t0], t2
    cellHandler(t2, Structure::m_typeInfo + TypeInfo::m_flags[t2], .target)
    dispatch(3)

.target:
    dispatchIntIndirect(2)

.immediate:
    andq ~TagBitUndefined, t0
    immediateHandler(t0, .target)
    dispatch(3)
end

_llint_op_jeq_null:
    traceExecution()
    equalNull(
        macro (structure, value, target) 
            btbz value, MasqueradesAsUndefined, .notMasqueradesAsUndefined
            loadp CodeBlock[cfr], t0
            loadp CodeBlock::m_globalObject[t0], t0
            bpeq Structure::m_globalObject[structure], t0, target
.notMasqueradesAsUndefined:
        end,
        macro (value, target) bqeq value, ValueNull, target end)


_llint_op_jneq_null:
    traceExecution()
    equalNull(
        macro (structure, value, target) 
            btbz value, MasqueradesAsUndefined, target
            loadp CodeBlock[cfr], t0
            loadp CodeBlock::m_globalObject[t0], t0
            bpneq Structure::m_globalObject[structure], t0, target
        end,
        macro (value, target) bqneq value, ValueNull, target end)


_llint_op_jneq_ptr:
    traceExecution()
    loadisFromInstruction(1, t0)
    loadisFromInstruction(2, t1)
    loadp CodeBlock[cfr], t2
    loadp CodeBlock::m_globalObject[t2], t2
    loadp JSGlobalObject::m_specialPointers[t2, t1, 8], t1
    bpneq t1, [cfr, t0, 8], .opJneqPtrTarget
    dispatch(4)

.opJneqPtrTarget:
    dispatchIntIndirect(3)


macro compare(integerCompare, doubleCompare, slowPath)
    loadisFromInstruction(1, t2)
    loadisFromInstruction(2, t3)
    loadConstantOrVariable(t2, t0)
    loadConstantOrVariable(t3, t1)
    bqb t0, tagTypeNumber, .op1NotInt
    bqb t1, tagTypeNumber, .op2NotInt
    integerCompare(t0, t1, .jumpTarget)
    dispatch(4)

.op1NotInt:
    btqz t0, tagTypeNumber, .slow
    bqb t1, tagTypeNumber, .op1NotIntOp2NotInt
    ci2d t1, ft1
    jmp .op1NotIntReady
.op1NotIntOp2NotInt:
    btqz t1, tagTypeNumber, .slow
    addq tagTypeNumber, t1
    fq2d t1, ft1
.op1NotIntReady:
    addq tagTypeNumber, t0
    fq2d t0, ft0
    doubleCompare(ft0, ft1, .jumpTarget)
    dispatch(4)

.op2NotInt:
    ci2d t0, ft0
    btqz t1, tagTypeNumber, .slow
    addq tagTypeNumber, t1
    fq2d t1, ft1
    doubleCompare(ft0, ft1, .jumpTarget)
    dispatch(4)

.jumpTarget:
    dispatchIntIndirect(3)

.slow:
    callSlowPath(slowPath)
    dispatch(0)
end


_llint_op_switch_imm:
    traceExecution()
    loadisFromInstruction(3, t2)
    loadisFromInstruction(1, t3)
    loadConstantOrVariable(t2, t1)
    loadp CodeBlock[cfr], t2
    loadp CodeBlock::m_rareData[t2], t2
    muli sizeof SimpleJumpTable, t3    # FIXME: would be nice to peephole this!
    loadp CodeBlock::RareData::m_immediateSwitchJumpTables + VectorBufferOffset[t2], t2
    addp t3, t2
    bqb t1, tagTypeNumber, .opSwitchImmNotInt
    subi SimpleJumpTable::min[t2], t1
    biaeq t1, SimpleJumpTable::branchOffsets + VectorSizeOffset[t2], .opSwitchImmFallThrough
    loadp SimpleJumpTable::branchOffsets + VectorBufferOffset[t2], t3
    loadis [t3, t1, 4], t1
    btiz t1, .opSwitchImmFallThrough
    dispatch(t1)

.opSwitchImmNotInt:
    btqnz t1, tagTypeNumber, .opSwitchImmSlow   # Go slow if it's a double.
.opSwitchImmFallThrough:
    dispatchIntIndirect(2)

.opSwitchImmSlow:
    callSlowPath(_llint_slow_path_switch_imm)
    dispatch(0)


_llint_op_switch_char:
    traceExecution()
    loadisFromInstruction(3, t2)
    loadisFromInstruction(1, t3)
    loadConstantOrVariable(t2, t1)
    loadp CodeBlock[cfr], t2
    loadp CodeBlock::m_rareData[t2], t2
    muli sizeof SimpleJumpTable, t3
    loadp CodeBlock::RareData::m_characterSwitchJumpTables + VectorBufferOffset[t2], t2
    addp t3, t2
    btqnz t1, tagMask, .opSwitchCharFallThrough
    loadp JSCell::m_structure[t1], t0
    bbneq Structure::m_typeInfo + TypeInfo::m_type[t0], StringType, .opSwitchCharFallThrough
    bineq JSString::m_length[t1], 1, .opSwitchCharFallThrough
    loadp JSString::m_value[t1], t0
    btpz  t0, .opSwitchOnRope
    loadp StringImpl::m_data8[t0], t1
    btinz StringImpl::m_hashAndFlags[t0], HashFlags8BitBuffer, .opSwitchChar8Bit
    loadh [t1], t0
    jmp .opSwitchCharReady
.opSwitchChar8Bit:
    loadb [t1], t0
.opSwitchCharReady:
    subi SimpleJumpTable::min[t2], t0
    biaeq t0, SimpleJumpTable::branchOffsets + VectorSizeOffset[t2], .opSwitchCharFallThrough
    loadp SimpleJumpTable::branchOffsets + VectorBufferOffset[t2], t2
    loadis [t2, t0, 4], t1
    btiz t1, .opSwitchCharFallThrough
    dispatch(t1)

.opSwitchCharFallThrough:
    dispatchIntIndirect(2)

.opSwitchOnRope:
    callSlowPath(_llint_slow_path_switch_char)
    dispatch(0)


_llint_op_new_func:
    traceExecution()
    loadisFromInstruction(3, t2)
    btiz t2, .opNewFuncUnchecked
    loadisFromInstruction(1, t1)
    btqnz [cfr, t1, 8], .opNewFuncDone
.opNewFuncUnchecked:
    callSlowPath(_llint_slow_path_new_func)
.opNewFuncDone:
    dispatch(4)


macro arrayProfileForCall()
    if VALUE_PROFILER
        loadisFromInstruction(3, t3)
        loadq ThisArgumentOffset[cfr, t3, 8], t0
        btqnz t0, tagMask, .done
        loadp JSCell::m_structure[t0], t0
        loadpFromInstruction(5, t1)
        storep t0, ArrayProfile::m_lastSeenStructure[t1]
    .done:
    end
end

macro doCall(slowPath)
    loadisFromInstruction(1, t0)
    loadpFromInstruction(4, t1)
    loadp LLIntCallLinkInfo::callee[t1], t2
    loadConstantOrVariable(t0, t3)
    bqneq t3, t2, .opCallSlow
    loadisFromInstruction(3, t3)
    addi 6, PC
    lshifti 3, t3
    addp cfr, t3
    loadp JSFunction::m_scope[t2], t0
    storeq t2, Callee[t3]
    storeq t0, ScopeChain[t3]
    loadisFromInstruction(-4, t2)
    storei PC, ArgumentCount + TagOffset[cfr]
    storeq cfr, CallerFrame[t3]
    storei t2, ArgumentCount + PayloadOffset[t3]
    move t3, cfr
    callTargetFunction(t1)

.opCallSlow:
    slowPathForCall(6, slowPath)
end


_llint_op_tear_off_activation:
    traceExecution()
    loadisFromInstruction(1, t0)
    btqz [cfr, t0, 8], .opTearOffActivationNotCreated
    callSlowPath(_llint_slow_path_tear_off_activation)
.opTearOffActivationNotCreated:
    dispatch(2)


_llint_op_tear_off_arguments:
    traceExecution()
    loadisFromInstruction(1, t0)
    subi 1, t0   # Get the unmodifiedArgumentsRegister
    btqz [cfr, t0, 8], .opTearOffArgumentsNotCreated
    callSlowPath(_llint_slow_path_tear_off_arguments)
.opTearOffArgumentsNotCreated:
    dispatch(3)


_llint_op_ret:
    traceExecution()
    checkSwitchToJITForEpilogue()
    loadisFromInstruction(1, t2)
    loadConstantOrVariable(t2, t0)
    doReturn()


_llint_op_call_put_result:
    loadisFromInstruction(1, t2)
    loadpFromInstruction(2, t3)
    storeq t0, [cfr, t2, 8]
    valueProfile(t0, t3)
    traceExecution()
    dispatch(3)


_llint_op_ret_object_or_this:
    traceExecution()
    checkSwitchToJITForEpilogue()
    loadisFromInstruction(1, t2)
    loadConstantOrVariable(t2, t0)
    btqnz t0, tagMask, .opRetObjectOrThisNotObject
    loadp JSCell::m_structure[t0], t2
    bbb Structure::m_typeInfo + TypeInfo::m_type[t2], ObjectType, .opRetObjectOrThisNotObject
    doReturn()

.opRetObjectOrThisNotObject:
    loadisFromInstruction(2, t2)
    loadConstantOrVariable(t2, t0)
    doReturn()


_llint_op_to_primitive:
    traceExecution()
    loadisFromInstruction(2, t2)
    loadisFromInstruction(1, t3)
    loadConstantOrVariable(t2, t0)
    btqnz t0, tagMask, .opToPrimitiveIsImm
    loadp JSCell::m_structure[t0], t2
    bbneq Structure::m_typeInfo + TypeInfo::m_type[t2], StringType, .opToPrimitiveSlowCase
.opToPrimitiveIsImm:
    storeq t0, [cfr, t3, 8]
    dispatch(3)

.opToPrimitiveSlowCase:
    callSlowPath(_llint_slow_path_to_primitive)
    dispatch(3)


_llint_op_next_pname:
    traceExecution()
    loadisFromInstruction(3, t1)
    loadisFromInstruction(4, t2)
    assertNotConstant(t1)
    assertNotConstant(t2)
    loadi PayloadOffset[cfr, t1, 8], t0
    bieq t0, PayloadOffset[cfr, t2, 8], .opNextPnameEnd
    loadisFromInstruction(5, t2)
    assertNotConstant(t2)
    loadp [cfr, t2, 8], t2
    loadp JSPropertyNameIterator::m_jsStrings[t2], t3
    loadq [t3, t0, 8], t3
    addi 1, t0
    storei t0, PayloadOffset[cfr, t1, 8]
    loadisFromInstruction(1, t1)
    storeq t3, [cfr, t1, 8]
    loadisFromInstruction(2, t3)
    assertNotConstant(t3)
    loadq [cfr, t3, 8], t3
    loadp JSCell::m_structure[t3], t1
    bpneq t1, JSPropertyNameIterator::m_cachedStructure[t2], .opNextPnameSlow
    loadp JSPropertyNameIterator::m_cachedPrototypeChain[t2], t0
    loadp StructureChain::m_vector[t0], t0
    btpz [t0], .opNextPnameTarget
.opNextPnameCheckPrototypeLoop:
    bqeq Structure::m_prototype[t1], ValueNull, .opNextPnameSlow
    loadq Structure::m_prototype[t1], t2
    loadp JSCell::m_structure[t2], t1
    bpneq t1, [t0], .opNextPnameSlow
    addp 8, t0
    btpnz [t0], .opNextPnameCheckPrototypeLoop
.opNextPnameTarget:
    dispatchIntIndirect(6)

.opNextPnameEnd:
    dispatch(7)

.opNextPnameSlow:
    callSlowPath(_llint_slow_path_next_pname) # This either keeps the PC where it was (causing us to loop) or sets it to target.
    dispatch(0)


_llint_op_catch:
    # This is where we end up from the JIT's throw trampoline (because the
    # machine code return address will be set to _llint_op_catch), and from
    # the interpreter's throw trampoline (see _llint_throw_trampoline).
    # The JIT throwing protocol calls for the cfr to be in t0. The throwing
    # code must have known that we were throwing to the interpreter, and have
    # set VM::targetInterpreterPCForThrow.
    move t0, cfr
    loadp CodeBlock[cfr], PB
    loadp CodeBlock::m_instructions[PB], PB
    loadp JITStackFrame::vm[sp], t3
    loadp VM::targetInterpreterPCForThrow[t3], PC
    subp PB, PC
    rshiftp 3, PC
    loadq VM::exception[t3], t0
    storeq 0, VM::exception[t3]
    loadisFromInstruction(1, t2)
    storeq t0, [cfr, t2, 8]
    traceExecution()
    dispatch(2)


_llint_op_end:
    traceExecution()
    checkSwitchToJITForEpilogue()
    loadisFromInstruction(1, t0)
    assertNotConstant(t0)
    loadq [cfr, t0, 8], t0
    doReturn()


_llint_throw_from_slow_path_trampoline:
    # When throwing from the interpreter (i.e. throwing from LLIntSlowPaths), so
    # the throw target is not necessarily interpreted code, we come to here.
    # This essentially emulates the JIT's throwing protocol.
    loadp JITStackFrame::vm[sp], t1
    loadp VM::callFrameForThrow[t1], t0
    jmp VM::targetMachinePCForThrow[t1]


_llint_throw_during_call_trampoline:
    preserveReturnAddressAfterCall(t2)
    loadp JITStackFrame::vm[sp], t1
    loadp VM::callFrameForThrow[t1], t0
    jmp VM::targetMachinePCForThrow[t1]

# Gives you the scope in t0, while allowing you to optionally perform additional checks on the
# scopes as they are traversed. scopeCheck() is called with two arguments: the register
# holding the scope, and a register that can be used for scratch. Note that this does not
# use t3, so you can hold stuff in t3 if need be.
macro getDeBruijnScope(deBruijinIndexOperand, scopeCheck)
    loadp ScopeChain[cfr], t0
    loadis deBruijinIndexOperand, t2

    btiz t2, .done

    loadp CodeBlock[cfr], t1
    bineq CodeBlock::m_codeType[t1], FunctionCode, .loop
    btbz CodeBlock::m_needsActivation[t1], .loop

    loadis CodeBlock::m_activationRegister[t1], t1

    # Need to conditionally skip over one scope.
    btpz [cfr, t1, 8], .noActivation
    scopeCheck(t0, t1)
    loadp JSScope::m_next[t0], t0
.noActivation:
    subi 1, t2

    btiz t2, .done
.loop:
    scopeCheck(t0, t1)
    loadp JSScope::m_next[t0], t0
    subi 1, t2
    btinz t2, .loop

.done:
end

_llint_op_get_scoped_var:
    traceExecution()
    # Operands are as follows:
    # pc[1]: Destination for the load
    # pc[2]: Index of register in the scope
    # 24[PB, PC, 8]  De Bruijin index.
    getDeBruijnScope(24[PB, PC, 8], macro (scope, scratch) end)
    loadisFromInstruction(1, t1)
    loadisFromInstruction(2, t2)

    loadp JSVariableObject::m_registers[t0], t0
    loadp [t0, t2, 8], t3
    storep t3, [cfr, t1, 8]
    loadp 32[PB, PC, 8], t1
    valueProfile(t3, t1)
    dispatch(5)


_llint_op_put_scoped_var:
    traceExecution()
    getDeBruijnScope(16[PB, PC, 8], macro (scope, scratch) end)
    loadis 24[PB, PC, 8], t1
    loadConstantOrVariable(t1, t3)
    loadis 8[PB, PC, 8], t1
    writeBarrier(t3)
    loadp JSVariableObject::m_registers[t0], t0
    storep t3, [t0, t1, 8]
    dispatch(4)

macro nativeCallTrampoline(executableOffsetToFunction)
    storep 0, CodeBlock[cfr]
    if X86_64
        loadp JITStackFrame::vm + 8[sp], t0
        storep cfr, VM::topCallFrame[t0]
        loadp CallerFrame[cfr], t0
        loadq ScopeChain[t0], t1
        storeq t1, ScopeChain[cfr]
        peek 0, t1
        storep t1, ReturnPC[cfr]
        move cfr, t5  # t5 = rdi
        subp 16 - 8, sp
        loadp Callee[cfr], t4 # t4 = rsi
        loadp JSFunction::m_executable[t4], t1
        move t0, cfr # Restore cfr to avoid loading from stack
        call executableOffsetToFunction[t1]
        addp 16 - 8, sp
        loadp JITStackFrame::vm + 8[sp], t3

    elsif C_LOOP
        loadp CallerFrame[cfr], t0
        loadp ScopeChain[t0], t1
        storep t1, ScopeChain[cfr]

        loadp JITStackFrame::vm[sp], t3
        storep cfr, VM::topCallFrame[t3]

        move t0, t2
        preserveReturnAddressAfterCall(t3)
        storep t3, ReturnPC[cfr]
        move cfr, t0
        loadp Callee[cfr], t1
        loadp JSFunction::m_executable[t1], t1
        move t2, cfr
        cloopCallNative executableOffsetToFunction[t1]

        restoreReturnAddressBeforeReturn(t3)
        loadp JITStackFrame::vm[sp], t3
    else
        error
    end

    btqnz VM::exception[t3], .exception
    ret
.exception:
    preserveReturnAddressAfterCall(t1)
    loadi ArgumentCount + TagOffset[cfr], PC
    loadp CodeBlock[cfr], PB
    loadp CodeBlock::m_instructions[PB], PB
    loadp JITStackFrame::vm[sp], t0
    storep cfr, VM::topCallFrame[t0]
    callSlowPath(_llint_throw_from_native_call)
    jmp _llint_throw_from_slow_path_trampoline
end

