/*
 * Copyright (C) 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CodeBlock.h"

#include "BytecodeGenerator.h"
#include "Debugger.h"
#include "Interpreter.h"
#include "JIT.h"
#include "JSActivation.h"
#include "JSFunction.h"
#include "JSStaticScopeObject.h"
#include "JSValue.h"
#include "UStringConcatenate.h"
#include <stdio.h>
#include <wtf/StringExtras.h>

#define DUMP_CODE_BLOCK_STATISTICS 0

namespace JSC {

#if !defined(NDEBUG) || ENABLE(OPCODE_SAMPLING)

static UString escapeQuotes(const UString& str)
{
    UString result = str;
    size_t pos = 0;
    while ((pos = result.find('\"', pos)) != notFound) {
        result = makeUString(result.substringSharingImpl(0, pos), "\"\\\"\"", result.substringSharingImpl(pos + 1));
        pos += 4;
    }
    return result;
}

static UString valueToSourceString(ExecState* exec, JSValue val)
{
    if (!val)
        return "0";

    if (val.isString())
        return makeUString("\"", escapeQuotes(val.toString(exec)), "\"");

    return val.toString(exec);
}

static CString constantName(ExecState* exec, int k, JSValue value)
{
    return makeUString(valueToSourceString(exec, value), "(@k", UString::number(k - FirstConstantRegisterIndex), ")").utf8();
}

static CString idName(int id0, const Identifier& ident)
{
    return makeUString(ident.ustring(), "(@id", UString::number(id0), ")").utf8();
}

CString CodeBlock::registerName(ExecState* exec, int r) const
{
    if (r == missingThisObjectMarker())
        return "<null>";

    if (isConstantRegisterIndex(r))
        return constantName(exec, r, getConstant(r));

    return makeUString("r", UString::number(r)).utf8();
}

static UString regexpToSourceString(RegExp* regExp)
{
    char postfix[5] = { '/', 0, 0, 0, 0 };
    int index = 1;
    if (regExp->global())
        postfix[index++] = 'g';
    if (regExp->ignoreCase())
        postfix[index++] = 'i';
    if (regExp->multiline())
        postfix[index] = 'm';

    return makeUString("/", regExp->pattern(), postfix);
}

static CString regexpName(int re, RegExp* regexp)
{
    return makeUString(regexpToSourceString(regexp), "(@re", UString::number(re), ")").utf8();
}

static UString pointerToSourceString(void* p)
{
    char buffer[2 + 2 * sizeof(void*) + 1]; // 0x [two characters per byte] \0
    snprintf(buffer, sizeof(buffer), "%p", p);
    return buffer;
}

NEVER_INLINE static const char* debugHookName(int debugHookID)
{
    switch (static_cast<DebugHookID>(debugHookID)) {
        case DidEnterCallFrame:
            return "didEnterCallFrame";
        case WillLeaveCallFrame:
            return "willLeaveCallFrame";
        case WillExecuteStatement:
            return "willExecuteStatement";
        case WillExecuteProgram:
            return "willExecuteProgram";
        case DidExecuteProgram:
            return "didExecuteProgram";
        case DidReachBreakpoint:
            return "didReachBreakpoint";
    }

    ASSERT_NOT_REACHED();
    return "";
}

void CodeBlock::printUnaryOp(ExecState* exec, int location, Vector<Instruction>::const_iterator& it, const char* op) const
{
    int r0 = (++it)->u.operand;
    int r1 = (++it)->u.operand;

    printf("[%4d] %s\t\t %s, %s\n", location, op, registerName(exec, r0).data(), registerName(exec, r1).data());
}

void CodeBlock::printBinaryOp(ExecState* exec, int location, Vector<Instruction>::const_iterator& it, const char* op) const
{
    int r0 = (++it)->u.operand;
    int r1 = (++it)->u.operand;
    int r2 = (++it)->u.operand;
    printf("[%4d] %s\t\t %s, %s, %s\n", location, op, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data());
}

void CodeBlock::printConditionalJump(ExecState* exec, const Vector<Instruction>::const_iterator&, Vector<Instruction>::const_iterator& it, int location, const char* op) const
{
    int r0 = (++it)->u.operand;
    int offset = (++it)->u.operand;
    printf("[%4d] %s\t\t %s, %d(->%d)\n", location, op, registerName(exec, r0).data(), offset, location + offset);
}

void CodeBlock::printGetByIdOp(ExecState* exec, int location, Vector<Instruction>::const_iterator& it, const char* op) const
{
    int r0 = (++it)->u.operand;
    int r1 = (++it)->u.operand;
    int id0 = (++it)->u.operand;
    printf("[%4d] %s\t %s, %s, %s\n", location, op, registerName(exec, r0).data(), registerName(exec, r1).data(), idName(id0, m_identifiers[id0]).data());
    it += 4;
}

void CodeBlock::printPutByIdOp(ExecState* exec, int location, Vector<Instruction>::const_iterator& it, const char* op) const
{
    int r0 = (++it)->u.operand;
    int id0 = (++it)->u.operand;
    int r1 = (++it)->u.operand;
    printf("[%4d] %s\t %s, %s, %s\n", location, op, registerName(exec, r0).data(), idName(id0, m_identifiers[id0]).data(), registerName(exec, r1).data());
    it += 5;
}

#if ENABLE(JIT)
static bool isGlobalResolve(OpcodeID opcodeID)
{
    return opcodeID == op_resolve_global || opcodeID == op_resolve_global_dynamic;
}

static bool isPropertyAccess(OpcodeID opcodeID)
{
    switch (opcodeID) {
        case op_get_by_id_self:
        case op_get_by_id_proto:
        case op_get_by_id_chain:
        case op_get_by_id_self_list:
        case op_get_by_id_proto_list:
        case op_put_by_id_transition:
        case op_put_by_id_replace:
        case op_get_by_id:
        case op_put_by_id:
        case op_get_by_id_generic:
        case op_put_by_id_generic:
        case op_get_array_length:
        case op_get_string_length:
            return true;
        default:
            return false;
    }
}

static unsigned instructionOffsetForNth(ExecState* exec, const Vector<Instruction>& instructions, int nth, bool (*predicate)(OpcodeID))
{
    size_t i = 0;
    while (i < instructions.size()) {
        OpcodeID currentOpcode = exec->interpreter()->getOpcodeID(instructions[i].u.opcode);
        if (predicate(currentOpcode)) {
            if (!--nth)
                return i;
        }
        i += opcodeLengths[currentOpcode];
    }

    ASSERT_NOT_REACHED();
    return 0;
}

static void printGlobalResolveInfo(const GlobalResolveInfo& resolveInfo, unsigned instructionOffset)
{
    printf("  [%4d] %s: %s\n", instructionOffset, "resolve_global", pointerToSourceString(resolveInfo.structure).utf8().data());
}

static void printStructureStubInfo(const StructureStubInfo& stubInfo, unsigned instructionOffset)
{
    switch (stubInfo.accessType) {
    case access_get_by_id_self:
        printf("  [%4d] %s: %s\n", instructionOffset, "get_by_id_self", pointerToSourceString(stubInfo.u.getByIdSelf.baseObjectStructure).utf8().data());
        return;
    case access_get_by_id_proto:
        printf("  [%4d] %s: %s, %s\n", instructionOffset, "get_by_id_proto", pointerToSourceString(stubInfo.u.getByIdProto.baseObjectStructure).utf8().data(), pointerToSourceString(stubInfo.u.getByIdProto.prototypeStructure).utf8().data());
        return;
    case access_get_by_id_chain:
        printf("  [%4d] %s: %s, %s\n", instructionOffset, "get_by_id_chain", pointerToSourceString(stubInfo.u.getByIdChain.baseObjectStructure).utf8().data(), pointerToSourceString(stubInfo.u.getByIdChain.chain).utf8().data());
        return;
    case access_get_by_id_self_list:
        printf("  [%4d] %s: %s (%d)\n", instructionOffset, "op_get_by_id_self_list", pointerToSourceString(stubInfo.u.getByIdSelfList.structureList).utf8().data(), stubInfo.u.getByIdSelfList.listSize);
        return;
    case access_get_by_id_proto_list:
        printf("  [%4d] %s: %s (%d)\n", instructionOffset, "op_get_by_id_proto_list", pointerToSourceString(stubInfo.u.getByIdProtoList.structureList).utf8().data(), stubInfo.u.getByIdProtoList.listSize);
        return;
    case access_put_by_id_transition:
        printf("  [%4d] %s: %s, %s, %s\n", instructionOffset, "put_by_id_transition", pointerToSourceString(stubInfo.u.putByIdTransition.previousStructure).utf8().data(), pointerToSourceString(stubInfo.u.putByIdTransition.structure).utf8().data(), pointerToSourceString(stubInfo.u.putByIdTransition.chain).utf8().data());
        return;
    case access_put_by_id_replace:
        printf("  [%4d] %s: %s\n", instructionOffset, "put_by_id_replace", pointerToSourceString(stubInfo.u.putByIdReplace.baseObjectStructure).utf8().data());
        return;
    case access_get_by_id:
        printf("  [%4d] %s\n", instructionOffset, "get_by_id");
        return;
    case access_put_by_id:
        printf("  [%4d] %s\n", instructionOffset, "put_by_id");
        return;
    case access_get_by_id_generic:
        printf("  [%4d] %s\n", instructionOffset, "op_get_by_id_generic");
        return;
    case access_put_by_id_generic:
        printf("  [%4d] %s\n", instructionOffset, "op_put_by_id_generic");
        return;
    case access_get_array_length:
        printf("  [%4d] %s\n", instructionOffset, "op_get_array_length");
        return;
    case access_get_string_length:
        printf("  [%4d] %s\n", instructionOffset, "op_get_string_length");
        return;
    default:
        ASSERT_NOT_REACHED();
    }
}
#endif

void CodeBlock::printStructure(const char* name, const Instruction* vPC, int operand) const
{
    unsigned instructionOffset = vPC - m_instructions.begin();
    printf("  [%4d] %s: %s\n", instructionOffset, name, pointerToSourceString(vPC[operand].u.structure).utf8().data());
}

void CodeBlock::printStructures(const Instruction* vPC) const
{
    Interpreter* interpreter = m_globalData->interpreter;
    unsigned instructionOffset = vPC - m_instructions.begin();

    if (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id)) {
        printStructure("get_by_id", vPC, 4);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_self)) {
        printStructure("get_by_id_self", vPC, 4);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_proto)) {
        printf("  [%4d] %s: %s, %s\n", instructionOffset, "get_by_id_proto", pointerToSourceString(vPC[4].u.structure).utf8().data(), pointerToSourceString(vPC[5].u.structure).utf8().data());
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id_transition)) {
        printf("  [%4d] %s: %s, %s, %s\n", instructionOffset, "put_by_id_transition", pointerToSourceString(vPC[4].u.structure).utf8().data(), pointerToSourceString(vPC[5].u.structure).utf8().data(), pointerToSourceString(vPC[6].u.structureChain).utf8().data());
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_chain)) {
        printf("  [%4d] %s: %s, %s\n", instructionOffset, "get_by_id_chain", pointerToSourceString(vPC[4].u.structure).utf8().data(), pointerToSourceString(vPC[5].u.structureChain).utf8().data());
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id)) {
        printStructure("put_by_id", vPC, 4);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id_replace)) {
        printStructure("put_by_id_replace", vPC, 4);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_resolve_global)) {
        printStructure("resolve_global", vPC, 4);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_resolve_global_dynamic)) {
        printStructure("resolve_global_dynamic", vPC, 4);
        return;
    }

    // These m_instructions doesn't ref Structures.
    ASSERT(vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_generic) || vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id_generic) || vPC[0].u.opcode == interpreter->getOpcode(op_call) || vPC[0].u.opcode == interpreter->getOpcode(op_call_eval) || vPC[0].u.opcode == interpreter->getOpcode(op_construct));
}

void CodeBlock::dump(ExecState* exec) const
{
    if (m_instructions.isEmpty()) {
        printf("No instructions available.\n");
        return;
    }

    size_t instructionCount = 0;

    for (size_t i = 0; i < m_instructions.size(); i += opcodeLengths[exec->interpreter()->getOpcodeID(m_instructions[i].u.opcode)])
        ++instructionCount;

    printf("%lu m_instructions; %lu bytes at %p; %d parameter(s); %d callee register(s)\n\n",
        static_cast<unsigned long>(instructionCount),
        static_cast<unsigned long>(m_instructions.size() * sizeof(Instruction)),
        this, m_numParameters, m_numCalleeRegisters);

    Vector<Instruction>::const_iterator begin = m_instructions.begin();
    Vector<Instruction>::const_iterator end = m_instructions.end();
    for (Vector<Instruction>::const_iterator it = begin; it != end; ++it)
        dump(exec, begin, it);

    if (!m_identifiers.isEmpty()) {
        printf("\nIdentifiers:\n");
        size_t i = 0;
        do {
            printf("  id%u = %s\n", static_cast<unsigned>(i), m_identifiers[i].ustring().utf8().data());
            ++i;
        } while (i != m_identifiers.size());
    }

    if (!m_constantRegisters.isEmpty()) {
        printf("\nConstants:\n");
        unsigned registerIndex = m_numVars;
        size_t i = 0;
        do {
            printf("   k%u = %s\n", registerIndex, valueToSourceString(exec, m_constantRegisters[i].get()).utf8().data());
            ++i;
            ++registerIndex;
        } while (i < m_constantRegisters.size());
    }

    if (m_rareData && !m_rareData->m_regexps.isEmpty()) {
        printf("\nm_regexps:\n");
        size_t i = 0;
        do {
            printf("  re%u = %s\n", static_cast<unsigned>(i), regexpToSourceString(m_rareData->m_regexps[i].get()).utf8().data());
            ++i;
        } while (i < m_rareData->m_regexps.size());
    }

#if ENABLE(JIT)
    if (!m_globalResolveInfos.isEmpty() || !m_structureStubInfos.isEmpty())
        printf("\nStructures:\n");

    if (!m_globalResolveInfos.isEmpty()) {
        size_t i = 0;
        do {
             printGlobalResolveInfo(m_globalResolveInfos[i], instructionOffsetForNth(exec, m_instructions, i + 1, isGlobalResolve));
             ++i;
        } while (i < m_globalResolveInfos.size());
    }
    if (!m_structureStubInfos.isEmpty()) {
        size_t i = 0;
        do {
            printStructureStubInfo(m_structureStubInfos[i], instructionOffsetForNth(exec, m_instructions, i + 1, isPropertyAccess));
             ++i;
        } while (i < m_structureStubInfos.size());
    }
#endif
#if ENABLE(INTERPRETER)
    if (!m_globalResolveInstructions.isEmpty() || !m_propertyAccessInstructions.isEmpty())
        printf("\nStructures:\n");

    if (!m_globalResolveInstructions.isEmpty()) {
        size_t i = 0;
        do {
             printStructures(&m_instructions[m_globalResolveInstructions[i]]);
             ++i;
        } while (i < m_globalResolveInstructions.size());
    }
    if (!m_propertyAccessInstructions.isEmpty()) {
        size_t i = 0;
        do {
            printStructures(&m_instructions[m_propertyAccessInstructions[i]]);
             ++i;
        } while (i < m_propertyAccessInstructions.size());
    }
#endif

    if (m_rareData && !m_rareData->m_exceptionHandlers.isEmpty()) {
        printf("\nException Handlers:\n");
        unsigned i = 0;
        do {
            printf("\t %d: { start: [%4d] end: [%4d] target: [%4d] }\n", i + 1, m_rareData->m_exceptionHandlers[i].start, m_rareData->m_exceptionHandlers[i].end, m_rareData->m_exceptionHandlers[i].target);
            ++i;
        } while (i < m_rareData->m_exceptionHandlers.size());
    }
    
    if (m_rareData && !m_rareData->m_immediateSwitchJumpTables.isEmpty()) {
        printf("Immediate Switch Jump Tables:\n");
        unsigned i = 0;
        do {
            printf("  %1d = {\n", i);
            int entry = 0;
            Vector<int32_t>::const_iterator end = m_rareData->m_immediateSwitchJumpTables[i].branchOffsets.end();
            for (Vector<int32_t>::const_iterator iter = m_rareData->m_immediateSwitchJumpTables[i].branchOffsets.begin(); iter != end; ++iter, ++entry) {
                if (!*iter)
                    continue;
                printf("\t\t%4d => %04d\n", entry + m_rareData->m_immediateSwitchJumpTables[i].min, *iter);
            }
            printf("      }\n");
            ++i;
        } while (i < m_rareData->m_immediateSwitchJumpTables.size());
    }
    
    if (m_rareData && !m_rareData->m_characterSwitchJumpTables.isEmpty()) {
        printf("\nCharacter Switch Jump Tables:\n");
        unsigned i = 0;
        do {
            printf("  %1d = {\n", i);
            int entry = 0;
            Vector<int32_t>::const_iterator end = m_rareData->m_characterSwitchJumpTables[i].branchOffsets.end();
            for (Vector<int32_t>::const_iterator iter = m_rareData->m_characterSwitchJumpTables[i].branchOffsets.begin(); iter != end; ++iter, ++entry) {
                if (!*iter)
                    continue;
                ASSERT(!((i + m_rareData->m_characterSwitchJumpTables[i].min) & ~0xFFFF));
                UChar ch = static_cast<UChar>(entry + m_rareData->m_characterSwitchJumpTables[i].min);
                printf("\t\t\"%s\" => %04d\n", UString(&ch, 1).utf8().data(), *iter);
        }
            printf("      }\n");
            ++i;
        } while (i < m_rareData->m_characterSwitchJumpTables.size());
    }
    
    if (m_rareData && !m_rareData->m_stringSwitchJumpTables.isEmpty()) {
        printf("\nString Switch Jump Tables:\n");
        unsigned i = 0;
        do {
            printf("  %1d = {\n", i);
            StringJumpTable::StringOffsetTable::const_iterator end = m_rareData->m_stringSwitchJumpTables[i].offsetTable.end();
            for (StringJumpTable::StringOffsetTable::const_iterator iter = m_rareData->m_stringSwitchJumpTables[i].offsetTable.begin(); iter != end; ++iter)
                printf("\t\t\"%s\" => %04d\n", UString(iter->first).utf8().data(), iter->second.branchOffset);
            printf("      }\n");
            ++i;
        } while (i < m_rareData->m_stringSwitchJumpTables.size());
    }

    printf("\n");
}

void CodeBlock::dump(ExecState* exec, const Vector<Instruction>::const_iterator& begin, Vector<Instruction>::const_iterator& it) const
{
    int location = it - begin;
    switch (exec->interpreter()->getOpcodeID(it->u.opcode)) {
        case op_enter: {
            printf("[%4d] enter\n", location);
            break;
        }
        case op_create_activation: {
            int r0 = (++it)->u.operand;
            printf("[%4d] create_activation %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_create_arguments: {
            int r0 = (++it)->u.operand;
            printf("[%4d] create_arguments\t %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_init_lazy_reg: {
            int r0 = (++it)->u.operand;
            printf("[%4d] init_lazy_reg\t %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_get_callee: {
            int r0 = (++it)->u.operand;
            printf("[%4d] op_get_callee %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_create_this: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            printf("[%4d] create_this %s %s\n", location, registerName(exec, r0).data(), registerName(exec, r1).data());
            break;
        }
        case op_convert_this: {
            int r0 = (++it)->u.operand;
            printf("[%4d] convert_this %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_convert_this_strict: {
            int r0 = (++it)->u.operand;
            printf("[%4d] convert_this_strict %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_new_object: {
            int r0 = (++it)->u.operand;
            printf("[%4d] new_object\t %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_new_array: {
            int dst = (++it)->u.operand;
            int argv = (++it)->u.operand;
            int argc = (++it)->u.operand;
            printf("[%4d] new_array\t %s, %s, %d\n", location, registerName(exec, dst).data(), registerName(exec, argv).data(), argc);
            break;
        }
        case op_new_regexp: {
            int r0 = (++it)->u.operand;
            int re0 = (++it)->u.operand;
            printf("[%4d] new_regexp\t %s, %s\n", location, registerName(exec, r0).data(), regexpName(re0, regexp(re0)).data());
            break;
        }
        case op_mov: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            printf("[%4d] mov\t\t %s, %s\n", location, registerName(exec, r0).data(), registerName(exec, r1).data());
            break;
        }
        case op_not: {
            printUnaryOp(exec, location, it, "not");
            break;
        }
        case op_eq: {
            printBinaryOp(exec, location, it, "eq");
            break;
        }
        case op_eq_null: {
            printUnaryOp(exec, location, it, "eq_null");
            break;
        }
        case op_neq: {
            printBinaryOp(exec, location, it, "neq");
            break;
        }
        case op_neq_null: {
            printUnaryOp(exec, location, it, "neq_null");
            break;
        }
        case op_stricteq: {
            printBinaryOp(exec, location, it, "stricteq");
            break;
        }
        case op_nstricteq: {
            printBinaryOp(exec, location, it, "nstricteq");
            break;
        }
        case op_less: {
            printBinaryOp(exec, location, it, "less");
            break;
        }
        case op_lesseq: {
            printBinaryOp(exec, location, it, "lesseq");
            break;
        }
        case op_pre_inc: {
            int r0 = (++it)->u.operand;
            printf("[%4d] pre_inc\t\t %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_pre_dec: {
            int r0 = (++it)->u.operand;
            printf("[%4d] pre_dec\t\t %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_post_inc: {
            printUnaryOp(exec, location, it, "post_inc");
            break;
        }
        case op_post_dec: {
            printUnaryOp(exec, location, it, "post_dec");
            break;
        }
        case op_to_jsnumber: {
            printUnaryOp(exec, location, it, "to_jsnumber");
            break;
        }
        case op_negate: {
            printUnaryOp(exec, location, it, "negate");
            break;
        }
        case op_add: {
            printBinaryOp(exec, location, it, "add");
            ++it;
            break;
        }
        case op_mul: {
            printBinaryOp(exec, location, it, "mul");
            ++it;
            break;
        }
        case op_div: {
            printBinaryOp(exec, location, it, "div");
            ++it;
            break;
        }
        case op_mod: {
            printBinaryOp(exec, location, it, "mod");
            break;
        }
        case op_sub: {
            printBinaryOp(exec, location, it, "sub");
            ++it;
            break;
        }
        case op_lshift: {
            printBinaryOp(exec, location, it, "lshift");
            break;            
        }
        case op_rshift: {
            printBinaryOp(exec, location, it, "rshift");
            break;
        }
        case op_urshift: {
            printBinaryOp(exec, location, it, "urshift");
            break;
        }
        case op_bitand: {
            printBinaryOp(exec, location, it, "bitand");
            ++it;
            break;
        }
        case op_bitxor: {
            printBinaryOp(exec, location, it, "bitxor");
            ++it;
            break;
        }
        case op_bitor: {
            printBinaryOp(exec, location, it, "bitor");
            ++it;
            break;
        }
        case op_bitnot: {
            printUnaryOp(exec, location, it, "bitnot");
            break;
        }
        case op_check_has_instance: {
            int base = (++it)->u.operand;
            printf("[%4d] check_has_instance\t\t %s\n", location, registerName(exec, base).data());
            break;
        }
        case op_instanceof: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int r2 = (++it)->u.operand;
            int r3 = (++it)->u.operand;
            printf("[%4d] instanceof\t\t %s, %s, %s, %s\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data(), registerName(exec, r3).data());
            break;
        }
        case op_typeof: {
            printUnaryOp(exec, location, it, "typeof");
            break;
        }
        case op_is_undefined: {
            printUnaryOp(exec, location, it, "is_undefined");
            break;
        }
        case op_is_boolean: {
            printUnaryOp(exec, location, it, "is_boolean");
            break;
        }
        case op_is_number: {
            printUnaryOp(exec, location, it, "is_number");
            break;
        }
        case op_is_string: {
            printUnaryOp(exec, location, it, "is_string");
            break;
        }
        case op_is_object: {
            printUnaryOp(exec, location, it, "is_object");
            break;
        }
        case op_is_function: {
            printUnaryOp(exec, location, it, "is_function");
            break;
        }
        case op_in: {
            printBinaryOp(exec, location, it, "in");
            break;
        }
        case op_resolve: {
            int r0 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            printf("[%4d] resolve\t\t %s, %s\n", location, registerName(exec, r0).data(), idName(id0, m_identifiers[id0]).data());
            break;
        }
        case op_resolve_skip: {
            int r0 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            int skipLevels = (++it)->u.operand;
            printf("[%4d] resolve_skip\t %s, %s, %d\n", location, registerName(exec, r0).data(), idName(id0, m_identifiers[id0]).data(), skipLevels);
            break;
        }
        case op_resolve_global: {
            int r0 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            printf("[%4d] resolve_global\t %s, %s\n", location, registerName(exec, r0).data(), idName(id0, m_identifiers[id0]).data());
            it += 2;
            break;
        }
        case op_resolve_global_dynamic: {
            int r0 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            JSValue scope = JSValue((++it)->u.jsCell.get());
            ++it;
            int depth = (++it)->u.operand;
            printf("[%4d] resolve_global_dynamic\t %s, %s, %s, %d\n", location, registerName(exec, r0).data(), valueToSourceString(exec, scope).utf8().data(), idName(id0, m_identifiers[id0]).data(), depth);
            break;
        }
        case op_get_scoped_var: {
            int r0 = (++it)->u.operand;
            int index = (++it)->u.operand;
            int skipLevels = (++it)->u.operand;
            printf("[%4d] get_scoped_var\t %s, %d, %d\n", location, registerName(exec, r0).data(), index, skipLevels);
            break;
        }
        case op_put_scoped_var: {
            int index = (++it)->u.operand;
            int skipLevels = (++it)->u.operand;
            int r0 = (++it)->u.operand;
            printf("[%4d] put_scoped_var\t %d, %d, %s\n", location, index, skipLevels, registerName(exec, r0).data());
            break;
        }
        case op_get_global_var: {
            int r0 = (++it)->u.operand;
            int index = (++it)->u.operand;
            printf("[%4d] get_global_var\t %s, %d\n", location, registerName(exec, r0).data(), index);
            break;
        }
        case op_put_global_var: {
            int index = (++it)->u.operand;
            int r0 = (++it)->u.operand;
            printf("[%4d] put_global_var\t %d, %s\n", location, index, registerName(exec, r0).data());
            break;
        }
        case op_resolve_base: {
            int r0 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            int isStrict = (++it)->u.operand;
            printf("[%4d] resolve_base%s\t %s, %s\n", location, isStrict ? "_strict" : "", registerName(exec, r0).data(), idName(id0, m_identifiers[id0]).data());
            break;
        }
        case op_ensure_property_exists: {
            int r0 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            printf("[%4d] ensure_property_exists\t %s, %s\n", location, registerName(exec, r0).data(), idName(id0, m_identifiers[id0]).data());
            break;
        }
        case op_resolve_with_base: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            printf("[%4d] resolve_with_base %s, %s, %s\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), idName(id0, m_identifiers[id0]).data());
            break;
        }
        case op_get_by_id: {
            printGetByIdOp(exec, location, it, "get_by_id");
            break;
        }
        case op_get_by_id_self: {
            printGetByIdOp(exec, location, it, "get_by_id_self");
            break;
        }
        case op_get_by_id_self_list: {
            printGetByIdOp(exec, location, it, "get_by_id_self_list");
            break;
        }
        case op_get_by_id_proto: {
            printGetByIdOp(exec, location, it, "get_by_id_proto");
            break;
        }
        case op_get_by_id_proto_list: {
            printGetByIdOp(exec, location, it, "op_get_by_id_proto_list");
            break;
        }
        case op_get_by_id_chain: {
            printGetByIdOp(exec, location, it, "get_by_id_chain");
            break;
        }
        case op_get_by_id_getter_self: {
            printGetByIdOp(exec, location, it, "get_by_id_getter_self");
            break;
        }
        case op_get_by_id_getter_self_list: {
            printGetByIdOp(exec, location, it, "get_by_id_getter_self_list");
            break;
        }
        case op_get_by_id_getter_proto: {
            printGetByIdOp(exec, location, it, "get_by_id_getter_proto");
            break;
        }
        case op_get_by_id_getter_proto_list: {
            printGetByIdOp(exec, location, it, "get_by_id_getter_proto_list");
            break;
        }
        case op_get_by_id_getter_chain: {
            printGetByIdOp(exec, location, it, "get_by_id_getter_chain");
            break;
        }
        case op_get_by_id_custom_self: {
            printGetByIdOp(exec, location, it, "get_by_id_custom_self");
            break;
        }
        case op_get_by_id_custom_self_list: {
            printGetByIdOp(exec, location, it, "get_by_id_custom_self_list");
            break;
        }
        case op_get_by_id_custom_proto: {
            printGetByIdOp(exec, location, it, "get_by_id_custom_proto");
            break;
        }
        case op_get_by_id_custom_proto_list: {
            printGetByIdOp(exec, location, it, "get_by_id_custom_proto_list");
            break;
        }
        case op_get_by_id_custom_chain: {
            printGetByIdOp(exec, location, it, "get_by_id_custom_chain");
            break;
        }
        case op_get_by_id_generic: {
            printGetByIdOp(exec, location, it, "get_by_id_generic");
            break;
        }
        case op_get_array_length: {
            printGetByIdOp(exec, location, it, "get_array_length");
            break;
        }
        case op_get_string_length: {
            printGetByIdOp(exec, location, it, "get_string_length");
            break;
        }
        case op_get_arguments_length: {
            printUnaryOp(exec, location, it, "get_arguments_length");
            it++;
            break;
        }
        case op_put_by_id: {
            printPutByIdOp(exec, location, it, "put_by_id");
            break;
        }
        case op_put_by_id_replace: {
            printPutByIdOp(exec, location, it, "put_by_id_replace");
            break;
        }
        case op_put_by_id_transition: {
            printPutByIdOp(exec, location, it, "put_by_id_transition");
            break;
        }
        case op_put_by_id_generic: {
            printPutByIdOp(exec, location, it, "put_by_id_generic");
            break;
        }
        case op_put_getter: {
            int r0 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            printf("[%4d] put_getter\t %s, %s, %s\n", location, registerName(exec, r0).data(), idName(id0, m_identifiers[id0]).data(), registerName(exec, r1).data());
            break;
        }
        case op_put_setter: {
            int r0 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            printf("[%4d] put_setter\t %s, %s, %s\n", location, registerName(exec, r0).data(), idName(id0, m_identifiers[id0]).data(), registerName(exec, r1).data());
            break;
        }
        case op_method_check: {
            printf("[%4d] method_check\n", location);
            break;
        }
        case op_del_by_id: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            printf("[%4d] del_by_id\t %s, %s, %s\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), idName(id0, m_identifiers[id0]).data());
            break;
        }
        case op_get_by_val: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int r2 = (++it)->u.operand;
            printf("[%4d] get_by_val\t %s, %s, %s\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data());
            break;
        }
        case op_get_argument_by_val: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int r2 = (++it)->u.operand;
            printf("[%4d] get_argument_by_val\t %s, %s, %s\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data());
            break;
        }
        case op_get_by_pname: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int r2 = (++it)->u.operand;
            int r3 = (++it)->u.operand;
            int r4 = (++it)->u.operand;
            int r5 = (++it)->u.operand;
            printf("[%4d] get_by_pname\t %s, %s, %s, %s, %s, %s\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data(), registerName(exec, r3).data(), registerName(exec, r4).data(), registerName(exec, r5).data());
            break;
        }
        case op_put_by_val: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int r2 = (++it)->u.operand;
            printf("[%4d] put_by_val\t %s, %s, %s\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data());
            break;
        }
        case op_del_by_val: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int r2 = (++it)->u.operand;
            printf("[%4d] del_by_val\t %s, %s, %s\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data());
            break;
        }
        case op_put_by_index: {
            int r0 = (++it)->u.operand;
            unsigned n0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            printf("[%4d] put_by_index\t %s, %u, %s\n", location, registerName(exec, r0).data(), n0, registerName(exec, r1).data());
            break;
        }
        case op_jmp: {
            int offset = (++it)->u.operand;
            printf("[%4d] jmp\t\t %d(->%d)\n", location, offset, location + offset);
            break;
        }
        case op_loop: {
            int offset = (++it)->u.operand;
            printf("[%4d] loop\t\t %d(->%d)\n", location, offset, location + offset);
            break;
        }
        case op_jtrue: {
            printConditionalJump(exec, begin, it, location, "jtrue");
            break;
        }
        case op_loop_if_true: {
            printConditionalJump(exec, begin, it, location, "loop_if_true");
            break;
        }
        case op_loop_if_false: {
            printConditionalJump(exec, begin, it, location, "loop_if_false");
            break;
        }
        case op_jfalse: {
            printConditionalJump(exec, begin, it, location, "jfalse");
            break;
        }
        case op_jeq_null: {
            printConditionalJump(exec, begin, it, location, "jeq_null");
            break;
        }
        case op_jneq_null: {
            printConditionalJump(exec, begin, it, location, "jneq_null");
            break;
        }
        case op_jneq_ptr: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            printf("[%4d] jneq_ptr\t\t %s, %s, %d(->%d)\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_jnless: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            printf("[%4d] jnless\t\t %s, %s, %d(->%d)\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_jnlesseq: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            printf("[%4d] jnlesseq\t\t %s, %s, %d(->%d)\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_loop_if_less: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            printf("[%4d] loop_if_less\t %s, %s, %d(->%d)\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_jless: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            printf("[%4d] jless\t\t %s, %s, %d(->%d)\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_jlesseq: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            printf("[%4d] jlesseq\t\t %s, %s, %d(->%d)\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_loop_if_lesseq: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            printf("[%4d] loop_if_lesseq\t %s, %s, %d(->%d)\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_switch_imm: {
            int tableIndex = (++it)->u.operand;
            int defaultTarget = (++it)->u.operand;
            int scrutineeRegister = (++it)->u.operand;
            printf("[%4d] switch_imm\t %d, %d(->%d), %s\n", location, tableIndex, defaultTarget, location + defaultTarget, registerName(exec, scrutineeRegister).data());
            break;
        }
        case op_switch_char: {
            int tableIndex = (++it)->u.operand;
            int defaultTarget = (++it)->u.operand;
            int scrutineeRegister = (++it)->u.operand;
            printf("[%4d] switch_char\t %d, %d(->%d), %s\n", location, tableIndex, defaultTarget, location + defaultTarget, registerName(exec, scrutineeRegister).data());
            break;
        }
        case op_switch_string: {
            int tableIndex = (++it)->u.operand;
            int defaultTarget = (++it)->u.operand;
            int scrutineeRegister = (++it)->u.operand;
            printf("[%4d] switch_string\t %d, %d(->%d), %s\n", location, tableIndex, defaultTarget, location + defaultTarget, registerName(exec, scrutineeRegister).data());
            break;
        }
        case op_new_func: {
            int r0 = (++it)->u.operand;
            int f0 = (++it)->u.operand;
            int shouldCheck = (++it)->u.operand;
            printf("[%4d] new_func\t\t %s, f%d, %s\n", location, registerName(exec, r0).data(), f0, shouldCheck ? "<Checked>" : "<Unchecked>");
            break;
        }
        case op_new_func_exp: {
            int r0 = (++it)->u.operand;
            int f0 = (++it)->u.operand;
            printf("[%4d] new_func_exp\t %s, f%d\n", location, registerName(exec, r0).data(), f0);
            break;
        }
        case op_call: {
            int func = (++it)->u.operand;
            int argCount = (++it)->u.operand;
            int registerOffset = (++it)->u.operand;
            printf("[%4d] call\t\t %s, %d, %d\n", location, registerName(exec, func).data(), argCount, registerOffset);
            break;
        }
        case op_call_eval: {
            int func = (++it)->u.operand;
            int argCount = (++it)->u.operand;
            int registerOffset = (++it)->u.operand;
            printf("[%4d] call_eval\t %s, %d, %d\n", location, registerName(exec, func).data(), argCount, registerOffset);
            break;
        }
        case op_call_varargs: {
            int func = (++it)->u.operand;
            int argCount = (++it)->u.operand;
            int registerOffset = (++it)->u.operand;
            printf("[%4d] call_varargs\t %s, %s, %d\n", location, registerName(exec, func).data(), registerName(exec, argCount).data(), registerOffset);
            break;
        }
        case op_load_varargs: {
            printUnaryOp(exec, location, it, "load_varargs");
            break;
        }
        case op_tear_off_activation: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            printf("[%4d] tear_off_activation\t %s, %s\n", location, registerName(exec, r0).data(), registerName(exec, r1).data());
            break;
        }
        case op_tear_off_arguments: {
            int r0 = (++it)->u.operand;
            printf("[%4d] tear_off_arguments\t %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_ret: {
            int r0 = (++it)->u.operand;
            printf("[%4d] ret\t\t %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_call_put_result: {
            int r0 = (++it)->u.operand;
            printf("[%4d] op_call_put_result\t\t %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_ret_object_or_this: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            printf("[%4d] constructor_ret\t\t %s %s\n", location, registerName(exec, r0).data(), registerName(exec, r1).data());
            break;
        }
        case op_construct: {
            int func = (++it)->u.operand;
            int argCount = (++it)->u.operand;
            int registerOffset = (++it)->u.operand;
            printf("[%4d] construct\t %s, %d, %d\n", location, registerName(exec, func).data(), argCount, registerOffset);
            break;
        }
        case op_strcat: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int count = (++it)->u.operand;
            printf("[%4d] strcat\t\t %s, %s, %d\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), count);
            break;
        }
        case op_to_primitive: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            printf("[%4d] to_primitive\t %s, %s\n", location, registerName(exec, r0).data(), registerName(exec, r1).data());
            break;
        }
        case op_get_pnames: {
            int r0 = it[1].u.operand;
            int r1 = it[2].u.operand;
            int r2 = it[3].u.operand;
            int r3 = it[4].u.operand;
            int offset = it[5].u.operand;
            printf("[%4d] get_pnames\t %s, %s, %s, %s, %d(->%d)\n", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data(), registerName(exec, r3).data(), offset, location + offset);
            it += OPCODE_LENGTH(op_get_pnames) - 1;
            break;
        }
        case op_next_pname: {
            int dest = it[1].u.operand;
            int base = it[2].u.operand;
            int i = it[3].u.operand;
            int size = it[4].u.operand;
            int iter = it[5].u.operand;
            int offset = it[6].u.operand;
            printf("[%4d] next_pname\t %s, %s, %s, %s, %s, %d(->%d)\n", location, registerName(exec, dest).data(), registerName(exec, base).data(), registerName(exec, i).data(), registerName(exec, size).data(), registerName(exec, iter).data(), offset, location + offset);
            it += OPCODE_LENGTH(op_next_pname) - 1;
            break;
        }
        case op_push_scope: {
            int r0 = (++it)->u.operand;
            printf("[%4d] push_scope\t %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_pop_scope: {
            printf("[%4d] pop_scope\n", location);
            break;
        }
        case op_push_new_scope: {
            int r0 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            printf("[%4d] push_new_scope \t%s, %s, %s\n", location, registerName(exec, r0).data(), idName(id0, m_identifiers[id0]).data(), registerName(exec, r1).data());
            break;
        }
        case op_jmp_scopes: {
            int scopeDelta = (++it)->u.operand;
            int offset = (++it)->u.operand;
            printf("[%4d] jmp_scopes\t^%d, %d(->%d)\n", location, scopeDelta, offset, location + offset);
            break;
        }
        case op_catch: {
            int r0 = (++it)->u.operand;
            printf("[%4d] catch\t\t %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_throw: {
            int r0 = (++it)->u.operand;
            printf("[%4d] throw\t\t %s\n", location, registerName(exec, r0).data());
            break;
        }
        case op_throw_reference_error: {
            int k0 = (++it)->u.operand;
            printf("[%4d] throw_reference_error\t %s\n", location, constantName(exec, k0, getConstant(k0)).data());
            break;
        }
        case op_jsr: {
            int retAddrDst = (++it)->u.operand;
            int offset = (++it)->u.operand;
            printf("[%4d] jsr\t\t %s, %d(->%d)\n", location, registerName(exec, retAddrDst).data(), offset, location + offset);
            break;
        }
        case op_sret: {
            int retAddrSrc = (++it)->u.operand;
            printf("[%4d] sret\t\t %s\n", location, registerName(exec, retAddrSrc).data());
            break;
        }
        case op_debug: {
            int debugHookID = (++it)->u.operand;
            int firstLine = (++it)->u.operand;
            int lastLine = (++it)->u.operand;
            printf("[%4d] debug\t\t %s, %d, %d\n", location, debugHookName(debugHookID), firstLine, lastLine);
            break;
        }
        case op_profile_will_call: {
            int function = (++it)->u.operand;
            printf("[%4d] profile_will_call %s\n", location, registerName(exec, function).data());
            break;
        }
        case op_profile_did_call: {
            int function = (++it)->u.operand;
            printf("[%4d] profile_did_call\t %s\n", location, registerName(exec, function).data());
            break;
        }
        case op_end: {
            int r0 = (++it)->u.operand;
            printf("[%4d] end\t\t %s\n", location, registerName(exec, r0).data());
            break;
        }
    }
}

#endif // !defined(NDEBUG) || ENABLE(OPCODE_SAMPLING)

#if DUMP_CODE_BLOCK_STATISTICS
static HashSet<CodeBlock*> liveCodeBlockSet;
#endif

#define FOR_EACH_MEMBER_VECTOR(macro) \
    macro(instructions) \
    macro(globalResolveInfos) \
    macro(structureStubInfos) \
    macro(callLinkInfos) \
    macro(linkedCallerList) \
    macro(identifiers) \
    macro(functionExpressions) \
    macro(constantRegisters)

#define FOR_EACH_MEMBER_VECTOR_RARE_DATA(macro) \
    macro(regexps) \
    macro(functions) \
    macro(exceptionHandlers) \
    macro(immediateSwitchJumpTables) \
    macro(characterSwitchJumpTables) \
    macro(stringSwitchJumpTables) \
    macro(evalCodeCache) \
    macro(expressionInfo) \
    macro(lineInfo) \
    macro(callReturnIndexVector)

template<typename T>
static size_t sizeInBytes(const Vector<T>& vector)
{
    return vector.capacity() * sizeof(T);
}

void CodeBlock::dumpStatistics()
{
#if DUMP_CODE_BLOCK_STATISTICS
    #define DEFINE_VARS(name) size_t name##IsNotEmpty = 0; size_t name##TotalSize = 0;
        FOR_EACH_MEMBER_VECTOR(DEFINE_VARS)
        FOR_EACH_MEMBER_VECTOR_RARE_DATA(DEFINE_VARS)
    #undef DEFINE_VARS

    // Non-vector data members
    size_t evalCodeCacheIsNotEmpty = 0;

    size_t symbolTableIsNotEmpty = 0;
    size_t symbolTableTotalSize = 0;

    size_t hasRareData = 0;

    size_t isFunctionCode = 0;
    size_t isGlobalCode = 0;
    size_t isEvalCode = 0;

    HashSet<CodeBlock*>::const_iterator end = liveCodeBlockSet.end();
    for (HashSet<CodeBlock*>::const_iterator it = liveCodeBlockSet.begin(); it != end; ++it) {
        CodeBlock* codeBlock = *it;

        #define GET_STATS(name) if (!codeBlock->m_##name.isEmpty()) { name##IsNotEmpty++; name##TotalSize += sizeInBytes(codeBlock->m_##name); }
            FOR_EACH_MEMBER_VECTOR(GET_STATS)
        #undef GET_STATS

        if (!codeBlock->m_symbolTable.isEmpty()) {
            symbolTableIsNotEmpty++;
            symbolTableTotalSize += (codeBlock->m_symbolTable.capacity() * (sizeof(SymbolTable::KeyType) + sizeof(SymbolTable::MappedType)));
        }

        if (codeBlock->m_rareData) {
            hasRareData++;
            #define GET_STATS(name) if (!codeBlock->m_rareData->m_##name.isEmpty()) { name##IsNotEmpty++; name##TotalSize += sizeInBytes(codeBlock->m_rareData->m_##name); }
                FOR_EACH_MEMBER_VECTOR_RARE_DATA(GET_STATS)
            #undef GET_STATS

            if (!codeBlock->m_rareData->m_evalCodeCache.isEmpty())
                evalCodeCacheIsNotEmpty++;
        }

        switch (codeBlock->codeType()) {
            case FunctionCode:
                ++isFunctionCode;
                break;
            case GlobalCode:
                ++isGlobalCode;
                break;
            case EvalCode:
                ++isEvalCode;
                break;
        }
    }

    size_t totalSize = 0;

    #define GET_TOTAL_SIZE(name) totalSize += name##TotalSize;
        FOR_EACH_MEMBER_VECTOR(GET_TOTAL_SIZE)
        FOR_EACH_MEMBER_VECTOR_RARE_DATA(GET_TOTAL_SIZE)
    #undef GET_TOTAL_SIZE

    totalSize += symbolTableTotalSize;
    totalSize += (liveCodeBlockSet.size() * sizeof(CodeBlock));

    printf("Number of live CodeBlocks: %d\n", liveCodeBlockSet.size());
    printf("Size of a single CodeBlock [sizeof(CodeBlock)]: %zu\n", sizeof(CodeBlock));
    printf("Size of all CodeBlocks: %zu\n", totalSize);
    printf("Average size of a CodeBlock: %zu\n", totalSize / liveCodeBlockSet.size());

    printf("Number of FunctionCode CodeBlocks: %zu (%.3f%%)\n", isFunctionCode, static_cast<double>(isFunctionCode) * 100.0 / liveCodeBlockSet.size());
    printf("Number of GlobalCode CodeBlocks: %zu (%.3f%%)\n", isGlobalCode, static_cast<double>(isGlobalCode) * 100.0 / liveCodeBlockSet.size());
    printf("Number of EvalCode CodeBlocks: %zu (%.3f%%)\n", isEvalCode, static_cast<double>(isEvalCode) * 100.0 / liveCodeBlockSet.size());

    printf("Number of CodeBlocks with rare data: %zu (%.3f%%)\n", hasRareData, static_cast<double>(hasRareData) * 100.0 / liveCodeBlockSet.size());

    #define PRINT_STATS(name) printf("Number of CodeBlocks with " #name ": %zu\n", name##IsNotEmpty); printf("Size of all " #name ": %zu\n", name##TotalSize); 
        FOR_EACH_MEMBER_VECTOR(PRINT_STATS)
        FOR_EACH_MEMBER_VECTOR_RARE_DATA(PRINT_STATS)
    #undef PRINT_STATS

    printf("Number of CodeBlocks with evalCodeCache: %zu\n", evalCodeCacheIsNotEmpty);
    printf("Number of CodeBlocks with symbolTable: %zu\n", symbolTableIsNotEmpty);

    printf("Size of all symbolTables: %zu\n", symbolTableTotalSize);

#else
    printf("Dumping CodeBlock statistics is not enabled.\n");
#endif
}

CodeBlock::CodeBlock(ScriptExecutable* ownerExecutable, CodeType codeType, JSGlobalObject *globalObject, PassRefPtr<SourceProvider> sourceProvider, unsigned sourceOffset, SymbolTable* symTab, bool isConstructor)
    : m_globalObject(globalObject->globalData(), ownerExecutable, globalObject)
    , m_heap(&m_globalObject->globalData().heap)
    , m_numCalleeRegisters(0)
    , m_numVars(0)
    , m_numParameters(0)
    , m_isConstructor(isConstructor)
    , m_ownerExecutable(globalObject->globalData(), ownerExecutable, ownerExecutable)
    , m_globalData(0)
#ifndef NDEBUG
    , m_instructionCount(0)
#endif
    , m_argumentsRegister(-1)
    , m_needsFullScopeChain(ownerExecutable->needsActivation())
    , m_usesEval(ownerExecutable->usesEval())
    , m_isNumericCompareFunction(false)
    , m_isStrictMode(ownerExecutable->isStrictMode())
    , m_codeType(codeType)
    , m_source(sourceProvider)
    , m_sourceOffset(sourceOffset)
    , m_symbolTable(symTab)
{
    ASSERT(m_source);

#if DUMP_CODE_BLOCK_STATISTICS
    liveCodeBlockSet.add(this);
#endif
}

CodeBlock::~CodeBlock()
{
#if ENABLE(JIT)
    for (size_t size = m_structureStubInfos.size(), i = 0; i < size; ++i)
        m_structureStubInfos[i].deref();
#endif // ENABLE(JIT)

#if DUMP_CODE_BLOCK_STATISTICS
    liveCodeBlockSet.remove(this);
#endif
}

void CodeBlock::visitStructures(SlotVisitor& visitor, Instruction* vPC) const
{
    Interpreter* interpreter = m_globalData->interpreter;

    if (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_self) || vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_getter_self) || vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_custom_self)) {
        visitor.append(&vPC[4].u.structure);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_proto) || vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_getter_proto) || vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_custom_proto)) {
        visitor.append(&vPC[4].u.structure);
        visitor.append(&vPC[5].u.structure);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_chain) || vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_getter_chain) || vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_custom_chain)) {
        visitor.append(&vPC[4].u.structure);
        visitor.append(&vPC[5].u.structureChain);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id_transition)) {
        visitor.append(&vPC[4].u.structure);
        visitor.append(&vPC[5].u.structure);
        visitor.append(&vPC[6].u.structureChain);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id_replace)) {
        visitor.append(&vPC[4].u.structure);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_resolve_global) || vPC[0].u.opcode == interpreter->getOpcode(op_resolve_global_dynamic)) {
        if (vPC[3].u.structure)
            visitor.append(&vPC[3].u.structure);
        return;
    }
    if ((vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_proto_list))
        || (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_self_list))
        || (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_getter_proto_list))
        || (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_getter_self_list))
        || (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_custom_proto_list))
        || (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_custom_self_list))) {
        PolymorphicAccessStructureList* polymorphicStructures = vPC[4].u.polymorphicStructures;
        polymorphicStructures->visitAggregate(visitor, vPC[5].u.operand);
        delete polymorphicStructures;
        return;
    }

    // These instructions don't ref their Structures.
    ASSERT(vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id) || vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id) || vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_generic) || vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id_generic) || vPC[0].u.opcode == interpreter->getOpcode(op_get_array_length) || vPC[0].u.opcode == interpreter->getOpcode(op_get_string_length));
}

void EvalCodeCache::visitAggregate(SlotVisitor& visitor)
{
    EvalCacheMap::iterator end = m_cacheMap.end();
    for (EvalCacheMap::iterator ptr = m_cacheMap.begin(); ptr != end; ++ptr)
        visitor.append(&ptr->second);
}

void CodeBlock::visitAggregate(SlotVisitor& visitor)
{
    visitor.append(&m_globalObject);
    visitor.append(&m_ownerExecutable);
    if (m_rareData)
        m_rareData->m_evalCodeCache.visitAggregate(visitor);
    visitor.appendValues(m_constantRegisters.data(), m_constantRegisters.size());
    for (size_t i = 0; i < m_functionExprs.size(); ++i)
        visitor.append(&m_functionExprs[i]);
    for (size_t i = 0; i < m_functionDecls.size(); ++i)
        visitor.append(&m_functionDecls[i]);
#if ENABLE(JIT_OPTIMIZE_CALL)
    for (unsigned i = 0; i < numberOfCallLinkInfos(); ++i)
        if (callLinkInfo(i).isLinked())
            visitor.append(&callLinkInfo(i).callee);
#endif
#if ENABLE(INTERPRETER)
    for (size_t size = m_propertyAccessInstructions.size(), i = 0; i < size; ++i)
        visitStructures(visitor, &m_instructions[m_propertyAccessInstructions[i]]);
#endif
#if ENABLE(JIT)
    for (size_t size = m_globalResolveInfos.size(), i = 0; i < size; ++i) {
        if (m_globalResolveInfos[i].structure)
            visitor.append(&m_globalResolveInfos[i].structure);
    }

    for (size_t size = m_structureStubInfos.size(), i = 0; i < size; ++i)
        m_structureStubInfos[i].visitAggregate(visitor);

    for (size_t size = m_methodCallLinkInfos.size(), i = 0; i < size; ++i) {
        if (m_methodCallLinkInfos[i].cachedStructure) {
            // Both members must be filled at the same time
            visitor.append(&m_methodCallLinkInfos[i].cachedStructure);
            ASSERT(!!m_methodCallLinkInfos[i].cachedPrototypeStructure);
            visitor.append(&m_methodCallLinkInfos[i].cachedPrototypeStructure);
        }
    }
#endif
}

HandlerInfo* CodeBlock::handlerForBytecodeOffset(unsigned bytecodeOffset)
{
    ASSERT(bytecodeOffset < m_instructionCount);

    if (!m_rareData)
        return 0;
    
    Vector<HandlerInfo>& exceptionHandlers = m_rareData->m_exceptionHandlers;
    for (size_t i = 0; i < exceptionHandlers.size(); ++i) {
        // Handlers are ordered innermost first, so the first handler we encounter
        // that contains the source address is the correct handler to use.
        if (exceptionHandlers[i].start <= bytecodeOffset && exceptionHandlers[i].end >= bytecodeOffset)
            return &exceptionHandlers[i];
    }

    return 0;
}

int CodeBlock::lineNumberForBytecodeOffset(unsigned bytecodeOffset)
{
    ASSERT(bytecodeOffset < m_instructionCount);

    if (!m_rareData)
        return m_ownerExecutable->source().firstLine();

    Vector<LineInfo>& lineInfo = m_rareData->m_lineInfo;

    int low = 0;
    int high = lineInfo.size();
    while (low < high) {
        int mid = low + (high - low) / 2;
        if (lineInfo[mid].instructionOffset <= bytecodeOffset)
            low = mid + 1;
        else
            high = mid;
    }

    if (!low)
        return m_ownerExecutable->source().firstLine();
    return lineInfo[low - 1].lineNumber;
}

void CodeBlock::expressionRangeForBytecodeOffset(unsigned bytecodeOffset, int& divot, int& startOffset, int& endOffset)
{
    ASSERT(bytecodeOffset < m_instructionCount);

    if (!m_rareData) {
        startOffset = 0;
        endOffset = 0;
        divot = 0;
        return;
    }

    Vector<ExpressionRangeInfo>& expressionInfo = m_rareData->m_expressionInfo;

    int low = 0;
    int high = expressionInfo.size();
    while (low < high) {
        int mid = low + (high - low) / 2;
        if (expressionInfo[mid].instructionOffset <= bytecodeOffset)
            low = mid + 1;
        else
            high = mid;
    }

    ASSERT(low);
    if (!low) {
        startOffset = 0;
        endOffset = 0;
        divot = 0;
        return;
    }

    startOffset = expressionInfo[low - 1].startOffset;
    endOffset = expressionInfo[low - 1].endOffset;
    divot = expressionInfo[low - 1].divotPoint + m_sourceOffset;
    return;
}

#if ENABLE(INTERPRETER)
bool CodeBlock::hasGlobalResolveInstructionAtBytecodeOffset(unsigned bytecodeOffset)
{
    if (m_globalResolveInstructions.isEmpty())
        return false;

    int low = 0;
    int high = m_globalResolveInstructions.size();
    while (low < high) {
        int mid = low + (high - low) / 2;
        if (m_globalResolveInstructions[mid] <= bytecodeOffset)
            low = mid + 1;
        else
            high = mid;
    }

    if (!low || m_globalResolveInstructions[low - 1] != bytecodeOffset)
        return false;
    return true;
}
#endif
#if ENABLE(JIT)
bool CodeBlock::hasGlobalResolveInfoAtBytecodeOffset(unsigned bytecodeOffset)
{
    if (m_globalResolveInfos.isEmpty())
        return false;

    int low = 0;
    int high = m_globalResolveInfos.size();
    while (low < high) {
        int mid = low + (high - low) / 2;
        if (m_globalResolveInfos[mid].bytecodeOffset <= bytecodeOffset)
            low = mid + 1;
        else
            high = mid;
    }

    if (!low || m_globalResolveInfos[low - 1].bytecodeOffset != bytecodeOffset)
        return false;
    return true;
}
#endif

void CodeBlock::shrinkToFit()
{
    m_instructions.shrinkToFit();

#if ENABLE(INTERPRETER)
    m_propertyAccessInstructions.shrinkToFit();
    m_globalResolveInstructions.shrinkToFit();
#endif
#if ENABLE(JIT)
    m_structureStubInfos.shrinkToFit();
    m_globalResolveInfos.shrinkToFit();
    m_callLinkInfos.shrinkToFit();
#endif

    m_identifiers.shrinkToFit();
    m_functionDecls.shrinkToFit();
    m_functionExprs.shrinkToFit();
    m_constantRegisters.shrinkToFit();

    if (m_rareData) {
        m_rareData->m_exceptionHandlers.shrinkToFit();
        m_rareData->m_regexps.shrinkToFit();
        m_rareData->m_immediateSwitchJumpTables.shrinkToFit();
        m_rareData->m_characterSwitchJumpTables.shrinkToFit();
        m_rareData->m_stringSwitchJumpTables.shrinkToFit();
        m_rareData->m_expressionInfo.shrinkToFit();
        m_rareData->m_lineInfo.shrinkToFit();
    }
}

void CodeBlock::createActivation(CallFrame* callFrame)
{
    ASSERT(codeType() == FunctionCode);
    ASSERT(needsFullScopeChain());
    ASSERT(!callFrame->uncheckedR(activationRegister()).jsValue());
    JSActivation* activation = new (callFrame) JSActivation(callFrame, static_cast<FunctionExecutable*>(ownerExecutable()));
    callFrame->uncheckedR(activationRegister()) = JSValue(activation);
    callFrame->setScopeChain(callFrame->scopeChain()->push(activation));
}

} // namespace JSC
