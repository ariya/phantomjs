/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "JSCellInlines.h"
#include "PreciseJumpTargets.h"

namespace JSC {

static void addSimpleSwitchTargets(SimpleJumpTable& jumpTable, unsigned bytecodeOffset, Vector<unsigned, 32>& out)
{
    for (unsigned i = jumpTable.branchOffsets.size(); i--;)
        out.append(bytecodeOffset + jumpTable.branchOffsets[i]);
}

void computePreciseJumpTargets(CodeBlock* codeBlock, Vector<unsigned, 32>& out)
{
    ASSERT(out.isEmpty());
    
    // We will derive a superset of the jump targets that the code block thinks it has.
    // So, if the code block claims there are none, then we are done.
    if (!codeBlock->numberOfJumpTargets())
        return;
    
    for (unsigned i = codeBlock->numberOfExceptionHandlers(); i--;)
        out.append(codeBlock->exceptionHandler(i).target);
    
    Interpreter* interpreter = codeBlock->vm()->interpreter;
    Instruction* instructionsBegin = codeBlock->instructions().begin();
    unsigned instructionCount = codeBlock->instructions().size();
    for (unsigned bytecodeOffset = 0; bytecodeOffset < instructionCount;) {
        OpcodeID opcodeID = interpreter->getOpcodeID(instructionsBegin[bytecodeOffset].u.opcode);
        Instruction* current = instructionsBegin + bytecodeOffset;
        switch (opcodeID) {
        case op_jmp:
            out.append(bytecodeOffset + current[1].u.operand);
            break;
        case op_jtrue:
        case op_jfalse:
        case op_jeq_null:
        case op_jneq_null:
            out.append(bytecodeOffset + current[2].u.operand);
            break;
        case op_jneq_ptr:
        case op_jless:
        case op_jlesseq:
        case op_jgreater:
        case op_jgreatereq:
        case op_jnless:
        case op_jnlesseq:
        case op_jngreater:
        case op_jngreatereq:
            out.append(bytecodeOffset + current[3].u.operand);
            break;
        case op_switch_imm:
            addSimpleSwitchTargets(codeBlock->immediateSwitchJumpTable(current[1].u.operand), bytecodeOffset, out);
            out.append(bytecodeOffset + current[2].u.operand);
            break;
        case op_switch_char:
            addSimpleSwitchTargets(codeBlock->characterSwitchJumpTable(current[1].u.operand), bytecodeOffset, out);
            out.append(bytecodeOffset + current[2].u.operand);
            break;
        case op_switch_string: {
            StringJumpTable& table = codeBlock->stringSwitchJumpTable(current[1].u.operand);
            StringJumpTable::StringOffsetTable::iterator iter = table.offsetTable.begin();
            StringJumpTable::StringOffsetTable::iterator end = table.offsetTable.end();
            for (; iter != end; ++iter)
                out.append(bytecodeOffset + iter->value.branchOffset);
            out.append(bytecodeOffset + current[2].u.operand);
            break;
        }
        case op_get_pnames:
            out.append(bytecodeOffset + current[5].u.operand);
            break;
        case op_next_pname:
            out.append(bytecodeOffset + current[6].u.operand);
            break;
        case op_check_has_instance:
            out.append(bytecodeOffset + current[4].u.operand);
            break;
        case op_loop_hint:
            out.append(bytecodeOffset);
            break;
        default:
            break;
        }
        bytecodeOffset += opcodeLengths[opcodeID];
    }
    
    std::sort(out.begin(), out.end());
    
    // We will have duplicates, and we must remove them.
    unsigned toIndex = 0;
    unsigned fromIndex = 0;
    unsigned lastValue = UINT_MAX;
    while (fromIndex < out.size()) {
        unsigned value = out[fromIndex++];
        if (value == lastValue)
            continue;
        out[toIndex++] = value;
        lastValue = value;
    }
    out.resize(toIndex);
}

} // namespace JSC

