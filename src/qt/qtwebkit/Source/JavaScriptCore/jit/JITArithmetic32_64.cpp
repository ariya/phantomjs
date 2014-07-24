/*
* Copyright (C) 2008 Apple Inc. All rights reserved.
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

#if ENABLE(JIT)
#if USE(JSVALUE32_64)
#include "JIT.h"

#include "CodeBlock.h"
#include "JITInlines.h"
#include "JITStubCall.h"
#include "JITStubs.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "Interpreter.h"
#include "Operations.h"
#include "ResultType.h"
#include "SamplingTool.h"

#ifndef NDEBUG
#include <stdio.h>
#endif

using namespace std;

namespace JSC {

void JIT::emit_op_negate(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned src = currentInstruction[2].u.operand;

    emitLoad(src, regT1, regT0);

    Jump srcNotInt = branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag));
    addSlowCase(branchTest32(Zero, regT0, TrustedImm32(0x7fffffff)));
    neg32(regT0);
    emitStoreInt32(dst, regT0, (dst == src));

    Jump end = jump();

    srcNotInt.link(this);
    addSlowCase(branch32(Above, regT1, TrustedImm32(JSValue::LowestTag)));

    xor32(TrustedImm32(1 << 31), regT1);
    store32(regT1, tagFor(dst));
    if (dst != src)
        store32(regT0, payloadFor(dst));

    end.link(this);
}

void JIT::emitSlow_op_negate(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;

    linkSlowCase(iter); // 0x7fffffff check
    linkSlowCase(iter); // double check

    JITStubCall stubCall(this, cti_op_negate);
    stubCall.addArgument(regT1, regT0);
    stubCall.call(dst);
}

void JIT::emit_compareAndJump(OpcodeID opcode, unsigned op1, unsigned op2, unsigned target, RelationalCondition condition)
{
    JumpList notInt32Op1;
    JumpList notInt32Op2;

    // Character less.
    if (isOperandConstantImmediateChar(op1)) {
        emitLoad(op2, regT1, regT0);
        addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::CellTag)));
        JumpList failures;
        emitLoadCharacterString(regT0, regT0, failures);
        addSlowCase(failures);
        addJump(branch32(commute(condition), regT0, Imm32(asString(getConstantOperand(op1))->tryGetValue()[0])), target);
        return;
    }
    if (isOperandConstantImmediateChar(op2)) {
        emitLoad(op1, regT1, regT0);
        addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::CellTag)));
        JumpList failures;
        emitLoadCharacterString(regT0, regT0, failures);
        addSlowCase(failures);
        addJump(branch32(condition, regT0, Imm32(asString(getConstantOperand(op2))->tryGetValue()[0])), target);
        return;
    } 
    if (isOperandConstantImmediateInt(op1)) {
        emitLoad(op2, regT3, regT2);
        notInt32Op2.append(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));
        addJump(branch32(commute(condition), regT2, Imm32(getConstantOperand(op1).asInt32())), target);
    } else if (isOperandConstantImmediateInt(op2)) {
        emitLoad(op1, regT1, regT0);
        notInt32Op1.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
        addJump(branch32(condition, regT0, Imm32(getConstantOperand(op2).asInt32())), target);
    } else {
        emitLoad2(op1, regT1, regT0, op2, regT3, regT2);
        notInt32Op1.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
        notInt32Op2.append(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));
        addJump(branch32(condition, regT0, regT2), target);
    }

    if (!supportsFloatingPoint()) {
        addSlowCase(notInt32Op1);
        addSlowCase(notInt32Op2);
        return;
    }
    Jump end = jump();

    // Double less.
    emitBinaryDoubleOp(opcode, target, op1, op2, OperandTypes(), notInt32Op1, notInt32Op2, !isOperandConstantImmediateInt(op1), isOperandConstantImmediateInt(op1) || !isOperandConstantImmediateInt(op2));
    end.link(this);
}

void JIT::emit_compareAndJumpSlow(unsigned op1, unsigned op2, unsigned target, DoubleCondition, int (JIT_STUB *stub)(STUB_ARGS_DECLARATION), bool invert, Vector<SlowCaseEntry>::iterator& iter)
{
    if (isOperandConstantImmediateChar(op1) || isOperandConstantImmediateChar(op2)) {
        linkSlowCase(iter);
        linkSlowCase(iter);
        linkSlowCase(iter);
        linkSlowCase(iter);
    } else {
        if (!supportsFloatingPoint()) {
            if (!isOperandConstantImmediateInt(op1) && !isOperandConstantImmediateInt(op2))
                linkSlowCase(iter); // int32 check
            linkSlowCase(iter); // int32 check
        } else {
            if (!isOperandConstantImmediateInt(op1)) {
                linkSlowCase(iter); // double check
                linkSlowCase(iter); // int32 check
            }
            if (isOperandConstantImmediateInt(op1) || !isOperandConstantImmediateInt(op2))
                linkSlowCase(iter); // double check
        }
    }
    JITStubCall stubCall(this, stub);
    stubCall.addArgument(op1);
    stubCall.addArgument(op2);
    stubCall.call();
    emitJumpSlowToHot(branchTest32(invert ? Zero : NonZero, regT0), target);
}

// LeftShift (<<)

void JIT::emit_op_lshift(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    if (isOperandConstantImmediateInt(op2)) {
        emitLoad(op1, regT1, regT0);
        addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
        lshift32(Imm32(getConstantOperand(op2).asInt32()), regT0);
        emitStoreAndMapInt32(dst, regT1, regT0, dst == op1, OPCODE_LENGTH(op_lshift));
        return;
    }

    emitLoad2(op1, regT1, regT0, op2, regT3, regT2);
    if (!isOperandConstantImmediateInt(op1))
        addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
    addSlowCase(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));
    lshift32(regT2, regT0);
    emitStoreAndMapInt32(dst, regT1, regT0, dst == op1 || dst == op2, OPCODE_LENGTH(op_lshift));
}

void JIT::emitSlow_op_lshift(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    if (!isOperandConstantImmediateInt(op1) && !isOperandConstantImmediateInt(op2))
        linkSlowCase(iter); // int32 check
    linkSlowCase(iter); // int32 check

    JITStubCall stubCall(this, cti_op_lshift);
    stubCall.addArgument(op1);
    stubCall.addArgument(op2);
    stubCall.call(dst);
}

// RightShift (>>) and UnsignedRightShift (>>>) helper

void JIT::emitRightShift(Instruction* currentInstruction, bool isUnsigned)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    // Slow case of rshift makes assumptions about what registers hold the
    // shift arguments, so any changes must be updated there as well.
    if (isOperandConstantImmediateInt(op2)) {
        emitLoad(op1, regT1, regT0);
        addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
        int shift = getConstantOperand(op2).asInt32() & 0x1f;
        if (shift) {
            if (isUnsigned)
                urshift32(Imm32(shift), regT0);
            else
                rshift32(Imm32(shift), regT0);
        } else if (isUnsigned) // signed right shift by zero is simply toInt conversion
            addSlowCase(branch32(LessThan, regT0, TrustedImm32(0)));
        emitStoreAndMapInt32(dst, regT1, regT0, dst == op1, OPCODE_LENGTH(op_rshift));
    } else {
        emitLoad2(op1, regT1, regT0, op2, regT3, regT2);
        if (!isOperandConstantImmediateInt(op1))
            addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
        addSlowCase(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));
        if (isUnsigned) {
            urshift32(regT2, regT0);
            addSlowCase(branch32(LessThan, regT0, TrustedImm32(0)));
        } else
            rshift32(regT2, regT0);
        emitStoreAndMapInt32(dst, regT1, regT0, dst == op1, OPCODE_LENGTH(op_rshift));
    }
}

void JIT::emitRightShiftSlowCase(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter, bool isUnsigned)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    if (isOperandConstantImmediateInt(op2)) {
        int shift = getConstantOperand(op2).asInt32() & 0x1f;
        // op1 = regT1:regT0
        linkSlowCase(iter); // int32 check
        if (supportsFloatingPointTruncate()) {
            JumpList failures;
            failures.append(branch32(AboveOrEqual, regT1, TrustedImm32(JSValue::LowestTag)));
            emitLoadDouble(op1, fpRegT0);
            failures.append(branchTruncateDoubleToInt32(fpRegT0, regT0));
            if (shift) {
                if (isUnsigned)
                    urshift32(Imm32(shift), regT0);
                else
                    rshift32(Imm32(shift), regT0);
            } else if (isUnsigned) // signed right shift by zero is simply toInt conversion
                failures.append(branch32(LessThan, regT0, TrustedImm32(0)));
            move(TrustedImm32(JSValue::Int32Tag), regT1);
            emitStoreInt32(dst, regT0, false);
            emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_rshift));
            failures.link(this);
        }
        if (isUnsigned && !shift)
            linkSlowCase(iter); // failed to box in hot path
    } else {
        // op1 = regT1:regT0
        // op2 = regT3:regT2
        if (!isOperandConstantImmediateInt(op1)) {
            linkSlowCase(iter); // int32 check -- op1 is not an int
            if (supportsFloatingPointTruncate()) {
                JumpList failures;
                failures.append(branch32(Above, regT1, TrustedImm32(JSValue::LowestTag))); // op1 is not a double
                emitLoadDouble(op1, fpRegT0);
                failures.append(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag))); // op2 is not an int
                failures.append(branchTruncateDoubleToInt32(fpRegT0, regT0));
                if (isUnsigned) {
                    urshift32(regT2, regT0);
                    failures.append(branch32(LessThan, regT0, TrustedImm32(0)));
                } else
                    rshift32(regT2, regT0);
                move(TrustedImm32(JSValue::Int32Tag), regT1);
                emitStoreInt32(dst, regT0, false);
                emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_rshift));
                failures.link(this);
            }
        }

        linkSlowCase(iter); // int32 check - op2 is not an int
        if (isUnsigned)
            linkSlowCase(iter); // Can't represent unsigned result as an immediate
    }

    JITStubCall stubCall(this, isUnsigned ? cti_op_urshift : cti_op_rshift);
    stubCall.addArgument(op1);
    stubCall.addArgument(op2);
    stubCall.call(dst);
}

// RightShift (>>)

void JIT::emit_op_rshift(Instruction* currentInstruction)
{
    emitRightShift(currentInstruction, false);
}

void JIT::emitSlow_op_rshift(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    emitRightShiftSlowCase(currentInstruction, iter, false);
}

// UnsignedRightShift (>>>)

void JIT::emit_op_urshift(Instruction* currentInstruction)
{
    emitRightShift(currentInstruction, true);
}

void JIT::emitSlow_op_urshift(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    emitRightShiftSlowCase(currentInstruction, iter, true);
}

// BitAnd (&)

void JIT::emit_op_bitand(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    unsigned op;
    int32_t constant;
    if (getOperandConstantImmediateInt(op1, op2, op, constant)) {
        emitLoad(op, regT1, regT0);
        addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
        and32(Imm32(constant), regT0);
        emitStoreAndMapInt32(dst, regT1, regT0, dst == op, OPCODE_LENGTH(op_bitand));
        return;
    }

    emitLoad2(op1, regT1, regT0, op2, regT3, regT2);
    addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
    addSlowCase(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));
    and32(regT2, regT0);
    emitStoreAndMapInt32(dst, regT1, regT0, (op1 == dst || op2 == dst), OPCODE_LENGTH(op_bitand));
}

void JIT::emitSlow_op_bitand(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    if (!isOperandConstantImmediateInt(op1) && !isOperandConstantImmediateInt(op2))
        linkSlowCase(iter); // int32 check
    linkSlowCase(iter); // int32 check

    JITStubCall stubCall(this, cti_op_bitand);
    stubCall.addArgument(op1);
    stubCall.addArgument(op2);
    stubCall.call(dst);
}

// BitOr (|)

void JIT::emit_op_bitor(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    unsigned op;
    int32_t constant;
    if (getOperandConstantImmediateInt(op1, op2, op, constant)) {
        emitLoad(op, regT1, regT0);
        addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
        or32(Imm32(constant), regT0);
        emitStoreAndMapInt32(dst, regT1, regT0, op == dst, OPCODE_LENGTH(op_bitor));
        return;
    }

    emitLoad2(op1, regT1, regT0, op2, regT3, regT2);
    addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
    addSlowCase(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));
    or32(regT2, regT0);
    emitStoreAndMapInt32(dst, regT1, regT0, (op1 == dst || op2 == dst), OPCODE_LENGTH(op_bitor));
}

void JIT::emitSlow_op_bitor(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    if (!isOperandConstantImmediateInt(op1) && !isOperandConstantImmediateInt(op2))
        linkSlowCase(iter); // int32 check
    linkSlowCase(iter); // int32 check

    JITStubCall stubCall(this, cti_op_bitor);
    stubCall.addArgument(op1);
    stubCall.addArgument(op2);
    stubCall.call(dst);
}

// BitXor (^)

void JIT::emit_op_bitxor(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    unsigned op;
    int32_t constant;
    if (getOperandConstantImmediateInt(op1, op2, op, constant)) {
        emitLoad(op, regT1, regT0);
        addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
        xor32(Imm32(constant), regT0);
        emitStoreAndMapInt32(dst, regT1, regT0, op == dst, OPCODE_LENGTH(op_bitxor));
        return;
    }

    emitLoad2(op1, regT1, regT0, op2, regT3, regT2);
    addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
    addSlowCase(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));
    xor32(regT2, regT0);
    emitStoreAndMapInt32(dst, regT1, regT0, (op1 == dst || op2 == dst), OPCODE_LENGTH(op_bitxor));
}

void JIT::emitSlow_op_bitxor(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    if (!isOperandConstantImmediateInt(op1) && !isOperandConstantImmediateInt(op2))
        linkSlowCase(iter); // int32 check
    linkSlowCase(iter); // int32 check

    JITStubCall stubCall(this, cti_op_bitxor);
    stubCall.addArgument(op1);
    stubCall.addArgument(op2);
    stubCall.call(dst);
}

void JIT::emit_op_inc(Instruction* currentInstruction)
{
    unsigned srcDst = currentInstruction[1].u.operand;

    emitLoad(srcDst, regT1, regT0);

    addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
    addSlowCase(branchAdd32(Overflow, TrustedImm32(1), regT0));
    emitStoreAndMapInt32(srcDst, regT1, regT0, true, OPCODE_LENGTH(op_inc));
}

void JIT::emitSlow_op_inc(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned srcDst = currentInstruction[1].u.operand;

    linkSlowCase(iter); // int32 check
    linkSlowCase(iter); // overflow check

    JITStubCall stubCall(this, cti_op_inc);
    stubCall.addArgument(srcDst);
    stubCall.call(srcDst);
}

void JIT::emit_op_dec(Instruction* currentInstruction)
{
    unsigned srcDst = currentInstruction[1].u.operand;

    emitLoad(srcDst, regT1, regT0);

    addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
    addSlowCase(branchSub32(Overflow, TrustedImm32(1), regT0));
    emitStoreAndMapInt32(srcDst, regT1, regT0, true, OPCODE_LENGTH(op_dec));
}

void JIT::emitSlow_op_dec(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned srcDst = currentInstruction[1].u.operand;

    linkSlowCase(iter); // int32 check
    linkSlowCase(iter); // overflow check

    JITStubCall stubCall(this, cti_op_dec);
    stubCall.addArgument(srcDst);
    stubCall.call(srcDst);
}

// Addition (+)

void JIT::emit_op_add(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

    if (!types.first().mightBeNumber() || !types.second().mightBeNumber()) {
        addSlowCase();
        JITStubCall stubCall(this, cti_op_add);
        stubCall.addArgument(op1);
        stubCall.addArgument(op2);
        stubCall.call(dst);
        return;
    }

    JumpList notInt32Op1;
    JumpList notInt32Op2;

    unsigned op;
    int32_t constant;
    if (getOperandConstantImmediateInt(op1, op2, op, constant)) {
        emitAdd32Constant(dst, op, constant, op == op1 ? types.first() : types.second());
        return;
    }

    emitLoad2(op1, regT1, regT0, op2, regT3, regT2);
    notInt32Op1.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
    notInt32Op2.append(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));

    // Int32 case.
    addSlowCase(branchAdd32(Overflow, regT2, regT0));
    emitStoreInt32(dst, regT0, (op1 == dst || op2 == dst));

    if (!supportsFloatingPoint()) {
        addSlowCase(notInt32Op1);
        addSlowCase(notInt32Op2);
        return;
    }
    Jump end = jump();

    // Double case.
    emitBinaryDoubleOp(op_add, dst, op1, op2, types, notInt32Op1, notInt32Op2);
    end.link(this);
}

void JIT::emitAdd32Constant(unsigned dst, unsigned op, int32_t constant, ResultType opType)
{
    // Int32 case.
    emitLoad(op, regT1, regT2);
    Jump notInt32 = branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag));
    addSlowCase(branchAdd32(Overflow, regT2, Imm32(constant), regT0));
    emitStoreInt32(dst, regT0, (op == dst));

    // Double case.
    if (!supportsFloatingPoint()) {
        addSlowCase(notInt32);
        return;
    }
    Jump end = jump();

    notInt32.link(this);
    if (!opType.definitelyIsNumber())
        addSlowCase(branch32(Above, regT1, TrustedImm32(JSValue::LowestTag)));
    move(Imm32(constant), regT2);
    convertInt32ToDouble(regT2, fpRegT0);
    emitLoadDouble(op, fpRegT1);
    addDouble(fpRegT1, fpRegT0);
    emitStoreDouble(dst, fpRegT0);

    end.link(this);
}

void JIT::emitSlow_op_add(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

    if (!types.first().mightBeNumber() || !types.second().mightBeNumber()) {
        linkDummySlowCase(iter);
        return;
    }

    unsigned op;
    int32_t constant;
    if (getOperandConstantImmediateInt(op1, op2, op, constant)) {
        linkSlowCase(iter); // overflow check

        if (!supportsFloatingPoint())
            linkSlowCase(iter); // non-sse case
        else {
            ResultType opType = op == op1 ? types.first() : types.second();
            if (!opType.definitelyIsNumber())
                linkSlowCase(iter); // double check
        }
    } else {
        linkSlowCase(iter); // overflow check

        if (!supportsFloatingPoint()) {
            linkSlowCase(iter); // int32 check
            linkSlowCase(iter); // int32 check
        } else {
            if (!types.first().definitelyIsNumber())
                linkSlowCase(iter); // double check

            if (!types.second().definitelyIsNumber()) {
                linkSlowCase(iter); // int32 check
                linkSlowCase(iter); // double check
            }
        }
    }

    JITStubCall stubCall(this, cti_op_add);
    stubCall.addArgument(op1);
    stubCall.addArgument(op2);
    stubCall.call(dst);
}

// Subtraction (-)

void JIT::emit_op_sub(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

    JumpList notInt32Op1;
    JumpList notInt32Op2;

    if (isOperandConstantImmediateInt(op2)) {
        emitSub32Constant(dst, op1, getConstantOperand(op2).asInt32(), types.first());
        return;
    }

    emitLoad2(op1, regT1, regT0, op2, regT3, regT2);
    notInt32Op1.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
    notInt32Op2.append(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));

    // Int32 case.
    addSlowCase(branchSub32(Overflow, regT2, regT0));
    emitStoreInt32(dst, regT0, (op1 == dst || op2 == dst));

    if (!supportsFloatingPoint()) {
        addSlowCase(notInt32Op1);
        addSlowCase(notInt32Op2);
        return;
    }
    Jump end = jump();

    // Double case.
    emitBinaryDoubleOp(op_sub, dst, op1, op2, types, notInt32Op1, notInt32Op2);
    end.link(this);
}

void JIT::emitSub32Constant(unsigned dst, unsigned op, int32_t constant, ResultType opType)
{
    // Int32 case.
    emitLoad(op, regT1, regT0);
    Jump notInt32 = branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag));
#if ENABLE(JIT_CONSTANT_BLINDING)
    addSlowCase(branchSub32(Overflow, regT0, Imm32(constant), regT2, regT3));
#else
    addSlowCase(branchSub32(Overflow, regT0, Imm32(constant), regT2));
#endif
    
    emitStoreInt32(dst, regT2, (op == dst));

    // Double case.
    if (!supportsFloatingPoint()) {
        addSlowCase(notInt32);
        return;
    }
    Jump end = jump();

    notInt32.link(this);
    if (!opType.definitelyIsNumber())
        addSlowCase(branch32(Above, regT1, TrustedImm32(JSValue::LowestTag)));
    move(Imm32(constant), regT2);
    convertInt32ToDouble(regT2, fpRegT0);
    emitLoadDouble(op, fpRegT1);
    subDouble(fpRegT0, fpRegT1);
    emitStoreDouble(dst, fpRegT1);

    end.link(this);
}

void JIT::emitSlow_op_sub(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

    if (isOperandConstantImmediateInt(op2)) {
        linkSlowCase(iter); // overflow check

        if (!supportsFloatingPoint() || !types.first().definitelyIsNumber())
            linkSlowCase(iter); // int32 or double check
    } else {
        linkSlowCase(iter); // overflow check

        if (!supportsFloatingPoint()) {
            linkSlowCase(iter); // int32 check
            linkSlowCase(iter); // int32 check
        } else {
            if (!types.first().definitelyIsNumber())
                linkSlowCase(iter); // double check

            if (!types.second().definitelyIsNumber()) {
                linkSlowCase(iter); // int32 check
                linkSlowCase(iter); // double check
            }
        }
    }

    JITStubCall stubCall(this, cti_op_sub);
    stubCall.addArgument(op1);
    stubCall.addArgument(op2);
    stubCall.call(dst);
}

void JIT::emitBinaryDoubleOp(OpcodeID opcodeID, unsigned dst, unsigned op1, unsigned op2, OperandTypes types, JumpList& notInt32Op1, JumpList& notInt32Op2, bool op1IsInRegisters, bool op2IsInRegisters)
{
    JumpList end;

    if (!notInt32Op1.empty()) {
        // Double case 1: Op1 is not int32; Op2 is unknown.
        notInt32Op1.link(this);

        ASSERT(op1IsInRegisters);

        // Verify Op1 is double.
        if (!types.first().definitelyIsNumber())
            addSlowCase(branch32(Above, regT1, TrustedImm32(JSValue::LowestTag)));

        if (!op2IsInRegisters)
            emitLoad(op2, regT3, regT2);

        Jump doubleOp2 = branch32(Below, regT3, TrustedImm32(JSValue::LowestTag));

        if (!types.second().definitelyIsNumber())
            addSlowCase(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));

        convertInt32ToDouble(regT2, fpRegT0);
        Jump doTheMath = jump();

        // Load Op2 as double into double register.
        doubleOp2.link(this);
        emitLoadDouble(op2, fpRegT0);

        // Do the math.
        doTheMath.link(this);
        switch (opcodeID) {
            case op_mul:
                emitLoadDouble(op1, fpRegT2);
                mulDouble(fpRegT2, fpRegT0);
                emitStoreDouble(dst, fpRegT0);
                break;
            case op_add:
                emitLoadDouble(op1, fpRegT2);
                addDouble(fpRegT2, fpRegT0);
                emitStoreDouble(dst, fpRegT0);
                break;
            case op_sub:
                emitLoadDouble(op1, fpRegT1);
                subDouble(fpRegT0, fpRegT1);
                emitStoreDouble(dst, fpRegT1);
                break;
            case op_div: {
                emitLoadDouble(op1, fpRegT1);
                divDouble(fpRegT0, fpRegT1);

#if ENABLE(VALUE_PROFILER)
                // Is the result actually an integer? The DFG JIT would really like to know. If it's
                // not an integer, we increment a count. If this together with the slow case counter
                // are below threshold then the DFG JIT will compile this division with a specualtion
                // that the remainder is zero.
                
                // As well, there are cases where a double result here would cause an important field
                // in the heap to sometimes have doubles in it, resulting in double predictions getting
                // propagated to a use site where it might cause damage (such as the index to an array
                // access). So if we are DFG compiling anything in the program, we want this code to
                // ensure that it produces integers whenever possible.
                
                // FIXME: This will fail to convert to integer if the result is zero. We should
                // distinguish between positive zero and negative zero here.
                
                JumpList notInteger;
                branchConvertDoubleToInt32(fpRegT1, regT2, notInteger, fpRegT0);
                // If we've got an integer, we might as well make that the result of the division.
                emitStoreInt32(dst, regT2);
                Jump isInteger = jump();
                notInteger.link(this);
                add32(TrustedImm32(1), AbsoluteAddress(&m_codeBlock->specialFastCaseProfileForBytecodeOffset(m_bytecodeOffset)->m_counter));
                emitStoreDouble(dst, fpRegT1);
                isInteger.link(this);
#else
                emitStoreDouble(dst, fpRegT1);
#endif
                break;
            }
            case op_jless:
                emitLoadDouble(op1, fpRegT2);
                addJump(branchDouble(DoubleLessThan, fpRegT2, fpRegT0), dst);
                break;
            case op_jlesseq:
                emitLoadDouble(op1, fpRegT2);
                addJump(branchDouble(DoubleLessThanOrEqual, fpRegT2, fpRegT0), dst);
                break;
            case op_jgreater:
                emitLoadDouble(op1, fpRegT2);
                addJump(branchDouble(DoubleGreaterThan, fpRegT2, fpRegT0), dst);
                break;
            case op_jgreatereq:
                emitLoadDouble(op1, fpRegT2);
                addJump(branchDouble(DoubleGreaterThanOrEqual, fpRegT2, fpRegT0), dst);
                break;
            case op_jnless:
                emitLoadDouble(op1, fpRegT2);
                addJump(branchDouble(DoubleLessThanOrEqualOrUnordered, fpRegT0, fpRegT2), dst);
                break;
            case op_jnlesseq:
                emitLoadDouble(op1, fpRegT2);
                addJump(branchDouble(DoubleLessThanOrUnordered, fpRegT0, fpRegT2), dst);
                break;
            case op_jngreater:
                emitLoadDouble(op1, fpRegT2);
                addJump(branchDouble(DoubleGreaterThanOrEqualOrUnordered, fpRegT0, fpRegT2), dst);
                break;
            case op_jngreatereq:
                emitLoadDouble(op1, fpRegT2);
                addJump(branchDouble(DoubleGreaterThanOrUnordered, fpRegT0, fpRegT2), dst);
                break;
            default:
                RELEASE_ASSERT_NOT_REACHED();
        }

        if (!notInt32Op2.empty())
            end.append(jump());
    }

    if (!notInt32Op2.empty()) {
        // Double case 2: Op1 is int32; Op2 is not int32.
        notInt32Op2.link(this);

        ASSERT(op2IsInRegisters);

        if (!op1IsInRegisters)
            emitLoadPayload(op1, regT0);

        convertInt32ToDouble(regT0, fpRegT0);

        // Verify op2 is double.
        if (!types.second().definitelyIsNumber())
            addSlowCase(branch32(Above, regT3, TrustedImm32(JSValue::LowestTag)));

        // Do the math.
        switch (opcodeID) {
            case op_mul:
                emitLoadDouble(op2, fpRegT2);
                mulDouble(fpRegT2, fpRegT0);
                emitStoreDouble(dst, fpRegT0);
                break;
            case op_add:
                emitLoadDouble(op2, fpRegT2);
                addDouble(fpRegT2, fpRegT0);
                emitStoreDouble(dst, fpRegT0);
                break;
            case op_sub:
                emitLoadDouble(op2, fpRegT2);
                subDouble(fpRegT2, fpRegT0);
                emitStoreDouble(dst, fpRegT0);
                break;
            case op_div: {
                emitLoadDouble(op2, fpRegT2);
                divDouble(fpRegT2, fpRegT0);
#if ENABLE(VALUE_PROFILER)
                // Is the result actually an integer? The DFG JIT would really like to know. If it's
                // not an integer, we increment a count. If this together with the slow case counter
                // are below threshold then the DFG JIT will compile this division with a specualtion
                // that the remainder is zero.
                
                // As well, there are cases where a double result here would cause an important field
                // in the heap to sometimes have doubles in it, resulting in double predictions getting
                // propagated to a use site where it might cause damage (such as the index to an array
                // access). So if we are DFG compiling anything in the program, we want this code to
                // ensure that it produces integers whenever possible.
                
                // FIXME: This will fail to convert to integer if the result is zero. We should
                // distinguish between positive zero and negative zero here.
                
                JumpList notInteger;
                branchConvertDoubleToInt32(fpRegT0, regT2, notInteger, fpRegT1);
                // If we've got an integer, we might as well make that the result of the division.
                emitStoreInt32(dst, regT2);
                Jump isInteger = jump();
                notInteger.link(this);
                add32(TrustedImm32(1), AbsoluteAddress(&m_codeBlock->specialFastCaseProfileForBytecodeOffset(m_bytecodeOffset)->m_counter));
                emitStoreDouble(dst, fpRegT0);
                isInteger.link(this);
#else
                emitStoreDouble(dst, fpRegT0);
#endif
                break;
            }
            case op_jless:
                emitLoadDouble(op2, fpRegT1);
                addJump(branchDouble(DoubleLessThan, fpRegT0, fpRegT1), dst);
                break;
            case op_jlesseq:
                emitLoadDouble(op2, fpRegT1);
                addJump(branchDouble(DoubleLessThanOrEqual, fpRegT0, fpRegT1), dst);
                break;
            case op_jgreater:
                emitLoadDouble(op2, fpRegT1);
                addJump(branchDouble(DoubleGreaterThan, fpRegT0, fpRegT1), dst);
                break;
            case op_jgreatereq:
                emitLoadDouble(op2, fpRegT1);
                addJump(branchDouble(DoubleGreaterThanOrEqual, fpRegT0, fpRegT1), dst);
                break;
            case op_jnless:
                emitLoadDouble(op2, fpRegT1);
                addJump(branchDouble(DoubleLessThanOrEqualOrUnordered, fpRegT1, fpRegT0), dst);
                break;
            case op_jnlesseq:
                emitLoadDouble(op2, fpRegT1);
                addJump(branchDouble(DoubleLessThanOrUnordered, fpRegT1, fpRegT0), dst);
                break;
            case op_jngreater:
                emitLoadDouble(op2, fpRegT1);
                addJump(branchDouble(DoubleGreaterThanOrEqualOrUnordered, fpRegT1, fpRegT0), dst);
                break;
            case op_jngreatereq:
                emitLoadDouble(op2, fpRegT1);
                addJump(branchDouble(DoubleGreaterThanOrUnordered, fpRegT1, fpRegT0), dst);
                break;
            default:
                RELEASE_ASSERT_NOT_REACHED();
        }
    }

    end.link(this);
}

// Multiplication (*)

void JIT::emit_op_mul(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

#if ENABLE(VALUE_PROFILER)
    m_codeBlock->addSpecialFastCaseProfile(m_bytecodeOffset);
#endif

    JumpList notInt32Op1;
    JumpList notInt32Op2;

    emitLoad2(op1, regT1, regT0, op2, regT3, regT2);
    notInt32Op1.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
    notInt32Op2.append(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));

    // Int32 case.
    move(regT0, regT3);
    addSlowCase(branchMul32(Overflow, regT2, regT0));
    addSlowCase(branchTest32(Zero, regT0));
    emitStoreInt32(dst, regT0, (op1 == dst || op2 == dst));

    if (!supportsFloatingPoint()) {
        addSlowCase(notInt32Op1);
        addSlowCase(notInt32Op2);
        return;
    }
    Jump end = jump();

    // Double case.
    emitBinaryDoubleOp(op_mul, dst, op1, op2, types, notInt32Op1, notInt32Op2);
    end.link(this);
}

void JIT::emitSlow_op_mul(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

    Jump overflow = getSlowCase(iter); // overflow check
    linkSlowCase(iter); // zero result check

    Jump negZero = branchOr32(Signed, regT2, regT3);
    emitStoreInt32(dst, TrustedImm32(0), (op1 == dst || op2 == dst));

    emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_mul));

    negZero.link(this);
#if ENABLE(VALUE_PROFILER)
    // We only get here if we have a genuine negative zero. Record this,
    // so that the speculative JIT knows that we failed speculation
    // because of a negative zero.
    add32(TrustedImm32(1), AbsoluteAddress(&m_codeBlock->specialFastCaseProfileForBytecodeOffset(m_bytecodeOffset)->m_counter));
#endif
    overflow.link(this);

    if (!supportsFloatingPoint()) {
        linkSlowCase(iter); // int32 check
        linkSlowCase(iter); // int32 check
    }

    if (supportsFloatingPoint()) {
        if (!types.first().definitelyIsNumber())
            linkSlowCase(iter); // double check

        if (!types.second().definitelyIsNumber()) {
            linkSlowCase(iter); // int32 check
            linkSlowCase(iter); // double check
        }
    }

    Label jitStubCall(this);
    JITStubCall stubCall(this, cti_op_mul);
    stubCall.addArgument(op1);
    stubCall.addArgument(op2);
    stubCall.call(dst);
}

// Division (/)

void JIT::emit_op_div(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

#if ENABLE(VALUE_PROFILER)
    m_codeBlock->addSpecialFastCaseProfile(m_bytecodeOffset);
#endif

    if (!supportsFloatingPoint()) {
        addSlowCase(jump());
        return;
    }

    // Int32 divide.
    JumpList notInt32Op1;
    JumpList notInt32Op2;

    JumpList end;

    emitLoad2(op1, regT1, regT0, op2, regT3, regT2);

    notInt32Op1.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
    notInt32Op2.append(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));

    convertInt32ToDouble(regT0, fpRegT0);
    convertInt32ToDouble(regT2, fpRegT1);
    divDouble(fpRegT1, fpRegT0);
#if ENABLE(VALUE_PROFILER)
    // Is the result actually an integer? The DFG JIT would really like to know. If it's
    // not an integer, we increment a count. If this together with the slow case counter
    // are below threshold then the DFG JIT will compile this division with a specualtion
    // that the remainder is zero.
    
    // As well, there are cases where a double result here would cause an important field
    // in the heap to sometimes have doubles in it, resulting in double predictions getting
    // propagated to a use site where it might cause damage (such as the index to an array
    // access). So if we are DFG compiling anything in the program, we want this code to
    // ensure that it produces integers whenever possible.
    
    // FIXME: This will fail to convert to integer if the result is zero. We should
    // distinguish between positive zero and negative zero here.
    
    JumpList notInteger;
    branchConvertDoubleToInt32(fpRegT0, regT2, notInteger, fpRegT1);
    // If we've got an integer, we might as well make that the result of the division.
    emitStoreInt32(dst, regT2);
    end.append(jump());
    notInteger.link(this);
    add32(TrustedImm32(1), AbsoluteAddress(&m_codeBlock->specialFastCaseProfileForBytecodeOffset(m_bytecodeOffset)->m_counter));
    emitStoreDouble(dst, fpRegT0);
#else
    emitStoreDouble(dst, fpRegT0);
#endif
    end.append(jump());

    // Double divide.
    emitBinaryDoubleOp(op_div, dst, op1, op2, types, notInt32Op1, notInt32Op2);
    end.link(this);
}

void JIT::emitSlow_op_div(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

    if (!supportsFloatingPoint())
        linkSlowCase(iter);
    else {
        if (!types.first().definitelyIsNumber())
            linkSlowCase(iter); // double check

        if (!types.second().definitelyIsNumber()) {
            linkSlowCase(iter); // int32 check
            linkSlowCase(iter); // double check
        }
    }

    JITStubCall stubCall(this, cti_op_div);
    stubCall.addArgument(op1);
    stubCall.addArgument(op2);
    stubCall.call(dst);
}

// Mod (%)

/* ------------------------------ BEGIN: OP_MOD ------------------------------ */

void JIT::emit_op_mod(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

#if CPU(X86) || CPU(X86_64)
    // Make sure registers are correct for x86 IDIV instructions.
    ASSERT(regT0 == X86Registers::eax);
    ASSERT(regT1 == X86Registers::edx);
    ASSERT(regT2 == X86Registers::ecx);
    ASSERT(regT3 == X86Registers::ebx);

    emitLoad2(op1, regT0, regT3, op2, regT1, regT2);
    addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
    addSlowCase(branch32(NotEqual, regT0, TrustedImm32(JSValue::Int32Tag)));

    move(regT3, regT0);
    addSlowCase(branchTest32(Zero, regT2));
    Jump denominatorNotNeg1 = branch32(NotEqual, regT2, TrustedImm32(-1));
    addSlowCase(branch32(Equal, regT0, TrustedImm32(-2147483647-1)));
    denominatorNotNeg1.link(this);
    m_assembler.cdq();
    m_assembler.idivl_r(regT2);
    Jump numeratorPositive = branch32(GreaterThanOrEqual, regT3, TrustedImm32(0));
    addSlowCase(branchTest32(Zero, regT1));
    numeratorPositive.link(this);
    emitStoreInt32(dst, regT1, (op1 == dst || op2 == dst));
#else
    JITStubCall stubCall(this, cti_op_mod);
    stubCall.addArgument(op1);
    stubCall.addArgument(op2);
    stubCall.call(dst);
#endif
}

void JIT::emitSlow_op_mod(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
#if CPU(X86) || CPU(X86_64)
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    linkSlowCase(iter);
    linkSlowCase(iter);
    linkSlowCase(iter);
    linkSlowCase(iter);
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_mod);
    stubCall.addArgument(op1);
    stubCall.addArgument(op2);
    stubCall.call(result);
#else
    UNUSED_PARAM(currentInstruction);
    UNUSED_PARAM(iter);
    // We would have really useful assertions here if it wasn't for the compiler's
    // insistence on attribute noreturn.
    // RELEASE_ASSERT_NOT_REACHED();
#endif
}

/* ------------------------------ END: OP_MOD ------------------------------ */

} // namespace JSC

#endif // USE(JSVALUE32_64)
#endif // ENABLE(JIT)
