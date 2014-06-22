/*
 * Copyright (C) 2008, 2009, 2010, 2012, 2013 Apple Inc. All rights reserved.
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
#include "CallLinkStatus.h"
#include "DFGCapabilities.h"
#include "DFGCommon.h"
#include "DFGNode.h"
#include "DFGRepatch.h"
#include "Debugger.h"
#include "Interpreter.h"
#include "JIT.h"
#include "JITStubs.h"
#include "JSActivation.h"
#include "JSCJSValue.h"
#include "JSFunction.h"
#include "JSNameScope.h"
#include "LowLevelInterpreter.h"
#include "Operations.h"
#include "ReduceWhitespace.h"
#include "RepatchBuffer.h"
#include "SlotVisitorInlines.h"
#include <stdio.h>
#include <wtf/CommaPrinter.h>
#include <wtf/StringExtras.h>
#include <wtf/StringPrintStream.h>

#if ENABLE(DFG_JIT)
#include "DFGOperations.h"
#endif

#define DUMP_CODE_BLOCK_STATISTICS 0

namespace JSC {

#if ENABLE(DFG_JIT)
using namespace DFG;
#endif

String CodeBlock::inferredName() const
{
    switch (codeType()) {
    case GlobalCode:
        return "<global>";
    case EvalCode:
        return "<eval>";
    case FunctionCode:
        return jsCast<FunctionExecutable*>(ownerExecutable())->inferredName().string();
    default:
        CRASH();
        return String();
    }
}

CodeBlockHash CodeBlock::hash() const
{
    return CodeBlockHash(ownerExecutable()->source(), specializationKind());
}

String CodeBlock::sourceCodeForTools() const
{
    if (codeType() != FunctionCode)
        return ownerExecutable()->source().toString();
    
    SourceProvider* provider = source();
    FunctionExecutable* executable = jsCast<FunctionExecutable*>(ownerExecutable());
    UnlinkedFunctionExecutable* unlinked = executable->unlinkedExecutable();
    unsigned unlinkedStartOffset = unlinked->startOffset();
    unsigned linkedStartOffset = executable->source().startOffset();
    int delta = linkedStartOffset - unlinkedStartOffset;
    StringBuilder builder;
    builder.append("function ");
    builder.append(provider->getRange(
        delta + unlinked->functionStartOffset(),
        delta + unlinked->startOffset() + unlinked->sourceLength()));
    return builder.toString();
}

String CodeBlock::sourceCodeOnOneLine() const
{
    return reduceWhitespace(sourceCodeForTools());
}

void CodeBlock::dumpAssumingJITType(PrintStream& out, JITCode::JITType jitType) const
{
    out.print(inferredName(), "#", hash(), ":[", RawPointer(this), "->", RawPointer(ownerExecutable()), ", ", jitType, codeType());
    if (codeType() == FunctionCode)
        out.print(specializationKind());
    if (ownerExecutable()->neverInline())
        out.print(" (NeverInline)");
    out.print("]");
}

void CodeBlock::dump(PrintStream& out) const
{
    dumpAssumingJITType(out, getJITType());
}

static String escapeQuotes(const String& str)
{
    String result = str;
    size_t pos = 0;
    while ((pos = result.find('\"', pos)) != notFound) {
        result = makeString(result.substringSharingImpl(0, pos), "\"\\\"\"", result.substringSharingImpl(pos + 1));
        pos += 4;
    }
    return result;
}

static String valueToSourceString(ExecState* exec, JSValue val)
{
    if (!val)
        return ASCIILiteral("0");

    if (val.isString())
        return makeString("\"", escapeQuotes(val.toString(exec)->value(exec)), "\"");

    return toString(val);
}

static CString constantName(ExecState* exec, int k, JSValue value)
{
    return makeString(valueToSourceString(exec, value), "(@k", String::number(k - FirstConstantRegisterIndex), ")").utf8();
}

static CString idName(int id0, const Identifier& ident)
{
    return makeString(ident.string(), "(@id", String::number(id0), ")").utf8();
}

CString CodeBlock::registerName(ExecState* exec, int r) const
{
    if (r == missingThisObjectMarker())
        return "<null>";

    if (isConstantRegisterIndex(r))
        return constantName(exec, r, getConstant(r));

    return makeString("r", String::number(r)).utf8();
}

static String regexpToSourceString(RegExp* regExp)
{
    char postfix[5] = { '/', 0, 0, 0, 0 };
    int index = 1;
    if (regExp->global())
        postfix[index++] = 'g';
    if (regExp->ignoreCase())
        postfix[index++] = 'i';
    if (regExp->multiline())
        postfix[index] = 'm';

    return makeString("/", regExp->pattern(), postfix);
}

static CString regexpName(int re, RegExp* regexp)
{
    return makeString(regexpToSourceString(regexp), "(@re", String::number(re), ")").utf8();
}

static String pointerToSourceString(void* p)
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

    RELEASE_ASSERT_NOT_REACHED();
    return "";
}

void CodeBlock::printUnaryOp(PrintStream& out, ExecState* exec, int location, const Instruction*& it, const char* op)
{
    int r0 = (++it)->u.operand;
    int r1 = (++it)->u.operand;

    out.printf("[%4d] %s\t\t %s, %s", location, op, registerName(exec, r0).data(), registerName(exec, r1).data());
}

void CodeBlock::printBinaryOp(PrintStream& out, ExecState* exec, int location, const Instruction*& it, const char* op)
{
    int r0 = (++it)->u.operand;
    int r1 = (++it)->u.operand;
    int r2 = (++it)->u.operand;
    out.printf("[%4d] %s\t\t %s, %s, %s", location, op, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data());
}

void CodeBlock::printConditionalJump(PrintStream& out, ExecState* exec, const Instruction*, const Instruction*& it, int location, const char* op)
{
    int r0 = (++it)->u.operand;
    int offset = (++it)->u.operand;
    out.printf("[%4d] %s\t\t %s, %d(->%d)", location, op, registerName(exec, r0).data(), offset, location + offset);
}

void CodeBlock::printGetByIdOp(PrintStream& out, ExecState* exec, int location, const Instruction*& it)
{
    const char* op;
    switch (exec->interpreter()->getOpcodeID(it->u.opcode)) {
    case op_get_by_id:
        op = "get_by_id";
        break;
    case op_get_by_id_out_of_line:
        op = "get_by_id_out_of_line";
        break;
    case op_get_by_id_self:
        op = "get_by_id_self";
        break;
    case op_get_by_id_proto:
        op = "get_by_id_proto";
        break;
    case op_get_by_id_chain:
        op = "get_by_id_chain";
        break;
    case op_get_by_id_getter_self:
        op = "get_by_id_getter_self";
        break;
    case op_get_by_id_getter_proto:
        op = "get_by_id_getter_proto";
        break;
    case op_get_by_id_getter_chain:
        op = "get_by_id_getter_chain";
        break;
    case op_get_by_id_custom_self:
        op = "get_by_id_custom_self";
        break;
    case op_get_by_id_custom_proto:
        op = "get_by_id_custom_proto";
        break;
    case op_get_by_id_custom_chain:
        op = "get_by_id_custom_chain";
        break;
    case op_get_by_id_generic:
        op = "get_by_id_generic";
        break;
    case op_get_array_length:
        op = "array_length";
        break;
    case op_get_string_length:
        op = "string_length";
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        op = 0;
    }
    int r0 = (++it)->u.operand;
    int r1 = (++it)->u.operand;
    int id0 = (++it)->u.operand;
    out.printf("[%4d] %s\t %s, %s, %s", location, op, registerName(exec, r0).data(), registerName(exec, r1).data(), idName(id0, m_identifiers[id0]).data());
    it += 4; // Increment up to the value profiler.
}

#if ENABLE(JIT) || ENABLE(LLINT) // unused in some configurations
static void dumpStructure(PrintStream& out, const char* name, ExecState* exec, Structure* structure, Identifier& ident)
{
    if (!structure)
        return;
    
    out.printf("%s = %p", name, structure);
    
    PropertyOffset offset = structure->get(exec->vm(), ident);
    if (offset != invalidOffset)
        out.printf(" (offset = %d)", offset);
}
#endif

#if ENABLE(JIT) // unused when not ENABLE(JIT), leading to silly warnings
static void dumpChain(PrintStream& out, ExecState* exec, StructureChain* chain, Identifier& ident)
{
    out.printf("chain = %p: [", chain);
    bool first = true;
    for (WriteBarrier<Structure>* currentStructure = chain->head();
         *currentStructure;
         ++currentStructure) {
        if (first)
            first = false;
        else
            out.printf(", ");
        dumpStructure(out, "struct", exec, currentStructure->get(), ident);
    }
    out.printf("]");
}
#endif

void CodeBlock::printGetByIdCacheStatus(PrintStream& out, ExecState* exec, int location)
{
    Instruction* instruction = instructions().begin() + location;

    Identifier& ident = identifier(instruction[3].u.operand);
    
    UNUSED_PARAM(ident); // tell the compiler to shut up in certain platform configurations.
    
#if ENABLE(LLINT)
    if (exec->interpreter()->getOpcodeID(instruction[0].u.opcode) == op_get_array_length)
        out.printf(" llint(array_length)");
    else if (Structure* structure = instruction[4].u.structure.get()) {
        out.printf(" llint(");
        dumpStructure(out, "struct", exec, structure, ident);
        out.printf(")");
    }
#endif

#if ENABLE(JIT)
    if (numberOfStructureStubInfos()) {
        StructureStubInfo& stubInfo = getStubInfo(location);
        if (stubInfo.seen) {
            out.printf(" jit(");
            
            Structure* baseStructure = 0;
            Structure* prototypeStructure = 0;
            StructureChain* chain = 0;
            PolymorphicAccessStructureList* structureList = 0;
            int listSize = 0;
            
            switch (stubInfo.accessType) {
            case access_get_by_id_self:
                out.printf("self");
                baseStructure = stubInfo.u.getByIdSelf.baseObjectStructure.get();
                break;
            case access_get_by_id_proto:
                out.printf("proto");
                baseStructure = stubInfo.u.getByIdProto.baseObjectStructure.get();
                prototypeStructure = stubInfo.u.getByIdProto.prototypeStructure.get();
                break;
            case access_get_by_id_chain:
                out.printf("chain");
                baseStructure = stubInfo.u.getByIdChain.baseObjectStructure.get();
                chain = stubInfo.u.getByIdChain.chain.get();
                break;
            case access_get_by_id_self_list:
                out.printf("self_list");
                structureList = stubInfo.u.getByIdSelfList.structureList;
                listSize = stubInfo.u.getByIdSelfList.listSize;
                break;
            case access_get_by_id_proto_list:
                out.printf("proto_list");
                structureList = stubInfo.u.getByIdProtoList.structureList;
                listSize = stubInfo.u.getByIdProtoList.listSize;
                break;
            case access_unset:
                out.printf("unset");
                break;
            case access_get_by_id_generic:
                out.printf("generic");
                break;
            case access_get_array_length:
                out.printf("array_length");
                break;
            case access_get_string_length:
                out.printf("string_length");
                break;
            default:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
            
            if (baseStructure) {
                out.printf(", ");
                dumpStructure(out, "struct", exec, baseStructure, ident);
            }
            
            if (prototypeStructure) {
                out.printf(", ");
                dumpStructure(out, "prototypeStruct", exec, baseStructure, ident);
            }
            
            if (chain) {
                out.printf(", ");
                dumpChain(out, exec, chain, ident);
            }
            
            if (structureList) {
                out.printf(", list = %p: [", structureList);
                for (int i = 0; i < listSize; ++i) {
                    if (i)
                        out.printf(", ");
                    out.printf("(");
                    dumpStructure(out, "base", exec, structureList->list[i].base.get(), ident);
                    if (structureList->list[i].isChain) {
                        if (structureList->list[i].u.chain.get()) {
                            out.printf(", ");
                            dumpChain(out, exec, structureList->list[i].u.chain.get(), ident);
                        }
                    } else {
                        if (structureList->list[i].u.proto.get()) {
                            out.printf(", ");
                            dumpStructure(out, "proto", exec, structureList->list[i].u.proto.get(), ident);
                        }
                    }
                    out.printf(")");
                }
                out.printf("]");
            }
            out.printf(")");
        }
    }
#endif
}

void CodeBlock::printCallOp(PrintStream& out, ExecState* exec, int location, const Instruction*& it, const char* op, CacheDumpMode cacheDumpMode)
{
    int func = (++it)->u.operand;
    int argCount = (++it)->u.operand;
    int registerOffset = (++it)->u.operand;
    out.printf("[%4d] %s\t %s, %d, %d", location, op, registerName(exec, func).data(), argCount, registerOffset);
    if (cacheDumpMode == DumpCaches) {
#if ENABLE(LLINT)
        LLIntCallLinkInfo* callLinkInfo = it[1].u.callLinkInfo;
        if (callLinkInfo->lastSeenCallee) {
            out.printf(
                " llint(%p, exec %p)",
                callLinkInfo->lastSeenCallee.get(),
                callLinkInfo->lastSeenCallee->executable());
        }
#endif
#if ENABLE(JIT)
        if (numberOfCallLinkInfos()) {
            JSFunction* target = getCallLinkInfo(location).lastSeenCallee.get();
            if (target)
                out.printf(" jit(%p, exec %p)", target, target->executable());
        }
#endif
        out.print(" status(", CallLinkStatus::computeFor(this, location), ")");
    }
    it += 2;
}

void CodeBlock::printPutByIdOp(PrintStream& out, ExecState* exec, int location, const Instruction*& it, const char* op)
{
    int r0 = (++it)->u.operand;
    int id0 = (++it)->u.operand;
    int r1 = (++it)->u.operand;
    out.printf("[%4d] %s\t %s, %s, %s", location, op, registerName(exec, r0).data(), idName(id0, m_identifiers[id0]).data(), registerName(exec, r1).data());
    it += 5;
}

void CodeBlock::printStructure(PrintStream& out, const char* name, const Instruction* vPC, int operand)
{
    unsigned instructionOffset = vPC - instructions().begin();
    out.printf("  [%4d] %s: %s\n", instructionOffset, name, pointerToSourceString(vPC[operand].u.structure).utf8().data());
}

void CodeBlock::printStructures(PrintStream& out, const Instruction* vPC)
{
    Interpreter* interpreter = m_vm->interpreter;
    unsigned instructionOffset = vPC - instructions().begin();

    if (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id)) {
        printStructure(out, "get_by_id", vPC, 4);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_self)) {
        printStructure(out, "get_by_id_self", vPC, 4);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_proto)) {
        out.printf("  [%4d] %s: %s, %s\n", instructionOffset, "get_by_id_proto", pointerToSourceString(vPC[4].u.structure).utf8().data(), pointerToSourceString(vPC[5].u.structure).utf8().data());
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id_transition)) {
        out.printf("  [%4d] %s: %s, %s, %s\n", instructionOffset, "put_by_id_transition", pointerToSourceString(vPC[4].u.structure).utf8().data(), pointerToSourceString(vPC[5].u.structure).utf8().data(), pointerToSourceString(vPC[6].u.structureChain).utf8().data());
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_chain)) {
        out.printf("  [%4d] %s: %s, %s\n", instructionOffset, "get_by_id_chain", pointerToSourceString(vPC[4].u.structure).utf8().data(), pointerToSourceString(vPC[5].u.structureChain).utf8().data());
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id)) {
        printStructure(out, "put_by_id", vPC, 4);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id_replace)) {
        printStructure(out, "put_by_id_replace", vPC, 4);
        return;
    }

    // These m_instructions doesn't ref Structures.
    ASSERT(vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_generic) || vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id_generic) || vPC[0].u.opcode == interpreter->getOpcode(op_call) || vPC[0].u.opcode == interpreter->getOpcode(op_call_eval) || vPC[0].u.opcode == interpreter->getOpcode(op_construct));
}

void CodeBlock::dumpBytecode(PrintStream& out)
{
    // We only use the ExecState* for things that don't actually lead to JS execution,
    // like converting a JSString to a String. Hence the globalExec is appropriate.
    ExecState* exec = m_globalObject->globalExec();
    
    size_t instructionCount = 0;

    for (size_t i = 0; i < instructions().size(); i += opcodeLengths[exec->interpreter()->getOpcodeID(instructions()[i].u.opcode)])
        ++instructionCount;

    out.print(*this);
    out.printf(
        ": %lu m_instructions; %lu bytes; %d parameter(s); %d callee register(s); %d variable(s)",
        static_cast<unsigned long>(instructions().size()),
        static_cast<unsigned long>(instructions().size() * sizeof(Instruction)),
        m_numParameters, m_numCalleeRegisters, m_numVars);
    if (symbolTable() && symbolTable()->captureCount()) {
        out.printf(
            "; %d captured var(s) (from r%d to r%d, inclusive)",
            symbolTable()->captureCount(), symbolTable()->captureStart(), symbolTable()->captureEnd() - 1);
    }
    if (usesArguments()) {
        out.printf(
            "; uses arguments, in r%d, r%d",
            argumentsRegister(),
            unmodifiedArgumentsRegister(argumentsRegister()));
    }
    if (needsFullScopeChain() && codeType() == FunctionCode)
        out.printf("; activation in r%d", activationRegister());
    out.print("\n\nSource: ", sourceCodeOnOneLine(), "\n\n");

    const Instruction* begin = instructions().begin();
    const Instruction* end = instructions().end();
    for (const Instruction* it = begin; it != end; ++it)
        dumpBytecode(out, exec, begin, it);

    if (!m_identifiers.isEmpty()) {
        out.printf("\nIdentifiers:\n");
        size_t i = 0;
        do {
            out.printf("  id%u = %s\n", static_cast<unsigned>(i), m_identifiers[i].string().utf8().data());
            ++i;
        } while (i != m_identifiers.size());
    }

    if (!m_constantRegisters.isEmpty()) {
        out.printf("\nConstants:\n");
        size_t i = 0;
        do {
            out.printf("   k%u = %s\n", static_cast<unsigned>(i), valueToSourceString(exec, m_constantRegisters[i].get()).utf8().data());
            ++i;
        } while (i < m_constantRegisters.size());
    }

    if (size_t count = m_unlinkedCode->numberOfRegExps()) {
        out.printf("\nm_regexps:\n");
        size_t i = 0;
        do {
            out.printf("  re%u = %s\n", static_cast<unsigned>(i), regexpToSourceString(m_unlinkedCode->regexp(i)).utf8().data());
            ++i;
        } while (i < count);
    }

#if ENABLE(JIT)
    if (!m_structureStubInfos.isEmpty())
        out.printf("\nStructures:\n");
#endif

    if (m_rareData && !m_rareData->m_exceptionHandlers.isEmpty()) {
        out.printf("\nException Handlers:\n");
        unsigned i = 0;
        do {
            out.printf("\t %d: { start: [%4d] end: [%4d] target: [%4d] depth: [%4d] }\n", i + 1, m_rareData->m_exceptionHandlers[i].start, m_rareData->m_exceptionHandlers[i].end, m_rareData->m_exceptionHandlers[i].target, m_rareData->m_exceptionHandlers[i].scopeDepth);
            ++i;
        } while (i < m_rareData->m_exceptionHandlers.size());
    }
    
    if (m_rareData && !m_rareData->m_immediateSwitchJumpTables.isEmpty()) {
        out.printf("Immediate Switch Jump Tables:\n");
        unsigned i = 0;
        do {
            out.printf("  %1d = {\n", i);
            int entry = 0;
            Vector<int32_t>::const_iterator end = m_rareData->m_immediateSwitchJumpTables[i].branchOffsets.end();
            for (Vector<int32_t>::const_iterator iter = m_rareData->m_immediateSwitchJumpTables[i].branchOffsets.begin(); iter != end; ++iter, ++entry) {
                if (!*iter)
                    continue;
                out.printf("\t\t%4d => %04d\n", entry + m_rareData->m_immediateSwitchJumpTables[i].min, *iter);
            }
            out.printf("      }\n");
            ++i;
        } while (i < m_rareData->m_immediateSwitchJumpTables.size());
    }
    
    if (m_rareData && !m_rareData->m_characterSwitchJumpTables.isEmpty()) {
        out.printf("\nCharacter Switch Jump Tables:\n");
        unsigned i = 0;
        do {
            out.printf("  %1d = {\n", i);
            int entry = 0;
            Vector<int32_t>::const_iterator end = m_rareData->m_characterSwitchJumpTables[i].branchOffsets.end();
            for (Vector<int32_t>::const_iterator iter = m_rareData->m_characterSwitchJumpTables[i].branchOffsets.begin(); iter != end; ++iter, ++entry) {
                if (!*iter)
                    continue;
                ASSERT(!((i + m_rareData->m_characterSwitchJumpTables[i].min) & ~0xFFFF));
                UChar ch = static_cast<UChar>(entry + m_rareData->m_characterSwitchJumpTables[i].min);
                out.printf("\t\t\"%s\" => %04d\n", String(&ch, 1).utf8().data(), *iter);
            }
            out.printf("      }\n");
            ++i;
        } while (i < m_rareData->m_characterSwitchJumpTables.size());
    }
    
    if (m_rareData && !m_rareData->m_stringSwitchJumpTables.isEmpty()) {
        out.printf("\nString Switch Jump Tables:\n");
        unsigned i = 0;
        do {
            out.printf("  %1d = {\n", i);
            StringJumpTable::StringOffsetTable::const_iterator end = m_rareData->m_stringSwitchJumpTables[i].offsetTable.end();
            for (StringJumpTable::StringOffsetTable::const_iterator iter = m_rareData->m_stringSwitchJumpTables[i].offsetTable.begin(); iter != end; ++iter)
                out.printf("\t\t\"%s\" => %04d\n", String(iter->key).utf8().data(), iter->value.branchOffset);
            out.printf("      }\n");
            ++i;
        } while (i < m_rareData->m_stringSwitchJumpTables.size());
    }

    out.printf("\n");
}

void CodeBlock::beginDumpProfiling(PrintStream& out, bool& hasPrintedProfiling)
{
    if (hasPrintedProfiling) {
        out.print("; ");
        return;
    }
    
    out.print("    ");
    hasPrintedProfiling = true;
}

void CodeBlock::dumpValueProfiling(PrintStream& out, const Instruction*& it, bool& hasPrintedProfiling)
{
    ++it;
#if ENABLE(VALUE_PROFILER)
    CString description = it->u.profile->briefDescription();
    if (!description.length())
        return;
    beginDumpProfiling(out, hasPrintedProfiling);
    out.print(description);
#else
    UNUSED_PARAM(out);
    UNUSED_PARAM(hasPrintedProfiling);
#endif
}

void CodeBlock::dumpArrayProfiling(PrintStream& out, const Instruction*& it, bool& hasPrintedProfiling)
{
    ++it;
#if ENABLE(VALUE_PROFILER)
    CString description = it->u.arrayProfile->briefDescription(this);
    if (!description.length())
        return;
    beginDumpProfiling(out, hasPrintedProfiling);
    out.print(description);
#else
    UNUSED_PARAM(out);
    UNUSED_PARAM(hasPrintedProfiling);
#endif
}

#if ENABLE(VALUE_PROFILER)
void CodeBlock::dumpRareCaseProfile(PrintStream& out, const char* name, RareCaseProfile* profile, bool& hasPrintedProfiling)
{
    if (!profile || !profile->m_counter)
        return;

    beginDumpProfiling(out, hasPrintedProfiling);
    out.print(name, profile->m_counter);
}
#endif

void CodeBlock::dumpBytecode(PrintStream& out, ExecState* exec, const Instruction* begin, const Instruction*& it)
{
    int location = it - begin;
    bool hasPrintedProfiling = false;
    switch (exec->interpreter()->getOpcodeID(it->u.opcode)) {
        case op_enter: {
            out.printf("[%4d] enter", location);
            break;
        }
        case op_create_activation: {
            int r0 = (++it)->u.operand;
            out.printf("[%4d] create_activation %s", location, registerName(exec, r0).data());
            break;
        }
        case op_create_arguments: {
            int r0 = (++it)->u.operand;
            out.printf("[%4d] create_arguments\t %s", location, registerName(exec, r0).data());
            break;
        }
        case op_init_lazy_reg: {
            int r0 = (++it)->u.operand;
            out.printf("[%4d] init_lazy_reg\t %s", location, registerName(exec, r0).data());
            break;
        }
        case op_get_callee: {
            int r0 = (++it)->u.operand;
            out.printf("[%4d] op_get_callee %s\n", location, registerName(exec, r0).data());
            ++it;
            break;
        }
        case op_create_this: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            unsigned inferredInlineCapacity = (++it)->u.operand;
            out.printf("[%4d] create_this %s, %s, %u", location, registerName(exec, r0).data(), registerName(exec, r1).data(), inferredInlineCapacity);
            break;
        }
        case op_convert_this: {
            int r0 = (++it)->u.operand;
            out.printf("[%4d] convert_this\t %s", location, registerName(exec, r0).data());
            ++it; // Skip value profile.
            break;
        }
        case op_new_object: {
            int r0 = (++it)->u.operand;
            unsigned inferredInlineCapacity = (++it)->u.operand;
            out.printf("[%4d] new_object\t %s, %u", location, registerName(exec, r0).data(), inferredInlineCapacity);
            ++it; // Skip object allocation profile.
            break;
        }
        case op_new_array: {
            int dst = (++it)->u.operand;
            int argv = (++it)->u.operand;
            int argc = (++it)->u.operand;
            out.printf("[%4d] new_array\t %s, %s, %d", location, registerName(exec, dst).data(), registerName(exec, argv).data(), argc);
            ++it; // Skip array allocation profile.
            break;
        }
        case op_new_array_with_size: {
            int dst = (++it)->u.operand;
            int length = (++it)->u.operand;
            out.printf("[%4d] new_array_with_size\t %s, %s", location, registerName(exec, dst).data(), registerName(exec, length).data());
            ++it; // Skip array allocation profile.
            break;
        }
        case op_new_array_buffer: {
            int dst = (++it)->u.operand;
            int argv = (++it)->u.operand;
            int argc = (++it)->u.operand;
            out.printf("[%4d] new_array_buffer\t %s, %d, %d", location, registerName(exec, dst).data(), argv, argc);
            ++it; // Skip array allocation profile.
            break;
        }
        case op_new_regexp: {
            int r0 = (++it)->u.operand;
            int re0 = (++it)->u.operand;
            out.printf("[%4d] new_regexp\t %s, ", location, registerName(exec, r0).data());
            if (r0 >=0 && r0 < (int)m_unlinkedCode->numberOfRegExps())
                out.printf("%s", regexpName(re0, regexp(re0)).data());
            else
                out.printf("bad_regexp(%d)", re0);
            break;
        }
        case op_mov: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            out.printf("[%4d] mov\t\t %s, %s", location, registerName(exec, r0).data(), registerName(exec, r1).data());
            break;
        }
        case op_not: {
            printUnaryOp(out, exec, location, it, "not");
            break;
        }
        case op_eq: {
            printBinaryOp(out, exec, location, it, "eq");
            break;
        }
        case op_eq_null: {
            printUnaryOp(out, exec, location, it, "eq_null");
            break;
        }
        case op_neq: {
            printBinaryOp(out, exec, location, it, "neq");
            break;
        }
        case op_neq_null: {
            printUnaryOp(out, exec, location, it, "neq_null");
            break;
        }
        case op_stricteq: {
            printBinaryOp(out, exec, location, it, "stricteq");
            break;
        }
        case op_nstricteq: {
            printBinaryOp(out, exec, location, it, "nstricteq");
            break;
        }
        case op_less: {
            printBinaryOp(out, exec, location, it, "less");
            break;
        }
        case op_lesseq: {
            printBinaryOp(out, exec, location, it, "lesseq");
            break;
        }
        case op_greater: {
            printBinaryOp(out, exec, location, it, "greater");
            break;
        }
        case op_greatereq: {
            printBinaryOp(out, exec, location, it, "greatereq");
            break;
        }
        case op_inc: {
            int r0 = (++it)->u.operand;
            out.printf("[%4d] pre_inc\t\t %s", location, registerName(exec, r0).data());
            break;
        }
        case op_dec: {
            int r0 = (++it)->u.operand;
            out.printf("[%4d] pre_dec\t\t %s", location, registerName(exec, r0).data());
            break;
        }
        case op_to_number: {
            printUnaryOp(out, exec, location, it, "to_number");
            break;
        }
        case op_negate: {
            printUnaryOp(out, exec, location, it, "negate");
            break;
        }
        case op_add: {
            printBinaryOp(out, exec, location, it, "add");
            ++it;
            break;
        }
        case op_mul: {
            printBinaryOp(out, exec, location, it, "mul");
            ++it;
            break;
        }
        case op_div: {
            printBinaryOp(out, exec, location, it, "div");
            ++it;
            break;
        }
        case op_mod: {
            printBinaryOp(out, exec, location, it, "mod");
            break;
        }
        case op_sub: {
            printBinaryOp(out, exec, location, it, "sub");
            ++it;
            break;
        }
        case op_lshift: {
            printBinaryOp(out, exec, location, it, "lshift");
            break;            
        }
        case op_rshift: {
            printBinaryOp(out, exec, location, it, "rshift");
            break;
        }
        case op_urshift: {
            printBinaryOp(out, exec, location, it, "urshift");
            break;
        }
        case op_bitand: {
            printBinaryOp(out, exec, location, it, "bitand");
            ++it;
            break;
        }
        case op_bitxor: {
            printBinaryOp(out, exec, location, it, "bitxor");
            ++it;
            break;
        }
        case op_bitor: {
            printBinaryOp(out, exec, location, it, "bitor");
            ++it;
            break;
        }
        case op_check_has_instance: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int r2 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            out.printf("[%4d] check_has_instance\t\t %s, %s, %s, %d(->%d)", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data(), offset, location + offset);
            break;
        }
        case op_instanceof: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int r2 = (++it)->u.operand;
            out.printf("[%4d] instanceof\t\t %s, %s, %s", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data());
            break;
        }
        case op_typeof: {
            printUnaryOp(out, exec, location, it, "typeof");
            break;
        }
        case op_is_undefined: {
            printUnaryOp(out, exec, location, it, "is_undefined");
            break;
        }
        case op_is_boolean: {
            printUnaryOp(out, exec, location, it, "is_boolean");
            break;
        }
        case op_is_number: {
            printUnaryOp(out, exec, location, it, "is_number");
            break;
        }
        case op_is_string: {
            printUnaryOp(out, exec, location, it, "is_string");
            break;
        }
        case op_is_object: {
            printUnaryOp(out, exec, location, it, "is_object");
            break;
        }
        case op_is_function: {
            printUnaryOp(out, exec, location, it, "is_function");
            break;
        }
        case op_in: {
            printBinaryOp(out, exec, location, it, "in");
            break;
        }
        case op_put_to_base_variable:
        case op_put_to_base: {
            int base = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            int value = (++it)->u.operand;
            int resolveInfo = (++it)->u.operand;
            out.printf("[%4d] put_to_base\t %s, %s, %s, %d", location, registerName(exec, base).data(), idName(id0, m_identifiers[id0]).data(), registerName(exec, value).data(), resolveInfo);
            break;
        }
        case op_resolve:
        case op_resolve_global_property:
        case op_resolve_global_var:
        case op_resolve_scoped_var:
        case op_resolve_scoped_var_on_top_scope:
        case op_resolve_scoped_var_with_top_scope_check: {
            int r0 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            int resolveInfo = (++it)->u.operand;
            out.printf("[%4d] resolve\t\t %s, %s, %d", location, registerName(exec, r0).data(), idName(id0, m_identifiers[id0]).data(), resolveInfo);
            dumpValueProfiling(out, it, hasPrintedProfiling);
            break;
        }
        case op_get_scoped_var: {
            int r0 = (++it)->u.operand;
            int index = (++it)->u.operand;
            int skipLevels = (++it)->u.operand;
            out.printf("[%4d] get_scoped_var\t %s, %d, %d", location, registerName(exec, r0).data(), index, skipLevels);
            dumpValueProfiling(out, it, hasPrintedProfiling);
            break;
        }
        case op_put_scoped_var: {
            int index = (++it)->u.operand;
            int skipLevels = (++it)->u.operand;
            int r0 = (++it)->u.operand;
            out.printf("[%4d] put_scoped_var\t %d, %d, %s", location, index, skipLevels, registerName(exec, r0).data());
            break;
        }
        case op_init_global_const_nop: {
            out.printf("[%4d] init_global_const_nop\t", location);
            it++;
            it++;
            it++;
            it++;
            break;
        }
        case op_init_global_const: {
            WriteBarrier<Unknown>* registerPointer = (++it)->u.registerPointer;
            int r0 = (++it)->u.operand;
            out.printf("[%4d] init_global_const\t g%d(%p), %s", location, m_globalObject->findRegisterIndex(registerPointer), registerPointer, registerName(exec, r0).data());
            it++;
            it++;
            break;
        }
        case op_init_global_const_check: {
            WriteBarrier<Unknown>* registerPointer = (++it)->u.registerPointer;
            int r0 = (++it)->u.operand;
            out.printf("[%4d] init_global_const_check\t g%d(%p), %s", location, m_globalObject->findRegisterIndex(registerPointer), registerPointer, registerName(exec, r0).data());
            it++;
            it++;
            break;
        }
        case op_resolve_base_to_global:
        case op_resolve_base_to_global_dynamic:
        case op_resolve_base_to_scope:
        case op_resolve_base_to_scope_with_top_scope_check:
        case op_resolve_base: {
            int r0 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            int isStrict = (++it)->u.operand;
            int resolveInfo = (++it)->u.operand;
            int putToBaseInfo = (++it)->u.operand;
            out.printf("[%4d] resolve_base%s\t %s, %s, %d, %d", location, isStrict ? "_strict" : "", registerName(exec, r0).data(), idName(id0, m_identifiers[id0]).data(), resolveInfo, putToBaseInfo);
            dumpValueProfiling(out, it, hasPrintedProfiling);
            break;
        }
        case op_resolve_with_base: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            int resolveInfo = (++it)->u.operand;
            int putToBaseInfo = (++it)->u.operand;
            out.printf("[%4d] resolve_with_base %s, %s, %s, %d, %d", location, registerName(exec, r0).data(), registerName(exec, r1).data(), idName(id0, m_identifiers[id0]).data(), resolveInfo, putToBaseInfo);
            dumpValueProfiling(out, it, hasPrintedProfiling);
            break;
        }
        case op_resolve_with_this: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            int resolveInfo = (++it)->u.operand;
            out.printf("[%4d] resolve_with_this %s, %s, %s, %d", location, registerName(exec, r0).data(), registerName(exec, r1).data(), idName(id0, m_identifiers[id0]).data(), resolveInfo);
            dumpValueProfiling(out, it, hasPrintedProfiling);
            break;
        }
        case op_get_by_id:
        case op_get_by_id_out_of_line:
        case op_get_by_id_self:
        case op_get_by_id_proto:
        case op_get_by_id_chain:
        case op_get_by_id_getter_self:
        case op_get_by_id_getter_proto:
        case op_get_by_id_getter_chain:
        case op_get_by_id_custom_self:
        case op_get_by_id_custom_proto:
        case op_get_by_id_custom_chain:
        case op_get_by_id_generic:
        case op_get_array_length:
        case op_get_string_length: {
            printGetByIdOp(out, exec, location, it);
            printGetByIdCacheStatus(out, exec, location);
            dumpValueProfiling(out, it, hasPrintedProfiling);
            break;
        }
        case op_get_arguments_length: {
            printUnaryOp(out, exec, location, it, "get_arguments_length");
            it++;
            break;
        }
        case op_put_by_id: {
            printPutByIdOp(out, exec, location, it, "put_by_id");
            break;
        }
        case op_put_by_id_out_of_line: {
            printPutByIdOp(out, exec, location, it, "put_by_id_out_of_line");
            break;
        }
        case op_put_by_id_replace: {
            printPutByIdOp(out, exec, location, it, "put_by_id_replace");
            break;
        }
        case op_put_by_id_transition: {
            printPutByIdOp(out, exec, location, it, "put_by_id_transition");
            break;
        }
        case op_put_by_id_transition_direct: {
            printPutByIdOp(out, exec, location, it, "put_by_id_transition_direct");
            break;
        }
        case op_put_by_id_transition_direct_out_of_line: {
            printPutByIdOp(out, exec, location, it, "put_by_id_transition_direct_out_of_line");
            break;
        }
        case op_put_by_id_transition_normal: {
            printPutByIdOp(out, exec, location, it, "put_by_id_transition_normal");
            break;
        }
        case op_put_by_id_transition_normal_out_of_line: {
            printPutByIdOp(out, exec, location, it, "put_by_id_transition_normal_out_of_line");
            break;
        }
        case op_put_by_id_generic: {
            printPutByIdOp(out, exec, location, it, "put_by_id_generic");
            break;
        }
        case op_put_getter_setter: {
            int r0 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int r2 = (++it)->u.operand;
            out.printf("[%4d] put_getter_setter\t %s, %s, %s, %s", location, registerName(exec, r0).data(), idName(id0, m_identifiers[id0]).data(), registerName(exec, r1).data(), registerName(exec, r2).data());
            break;
        }
        case op_del_by_id: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int id0 = (++it)->u.operand;
            out.printf("[%4d] del_by_id\t %s, %s, %s", location, registerName(exec, r0).data(), registerName(exec, r1).data(), idName(id0, m_identifiers[id0]).data());
            break;
        }
        case op_get_by_val: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int r2 = (++it)->u.operand;
            out.printf("[%4d] get_by_val\t %s, %s, %s", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data());
            dumpArrayProfiling(out, it, hasPrintedProfiling);
            dumpValueProfiling(out, it, hasPrintedProfiling);
            break;
        }
        case op_get_argument_by_val: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int r2 = (++it)->u.operand;
            out.printf("[%4d] get_argument_by_val\t %s, %s, %s", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data());
            ++it;
            dumpValueProfiling(out, it, hasPrintedProfiling);
            break;
        }
        case op_get_by_pname: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int r2 = (++it)->u.operand;
            int r3 = (++it)->u.operand;
            int r4 = (++it)->u.operand;
            int r5 = (++it)->u.operand;
            out.printf("[%4d] get_by_pname\t %s, %s, %s, %s, %s, %s", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data(), registerName(exec, r3).data(), registerName(exec, r4).data(), registerName(exec, r5).data());
            break;
        }
        case op_put_by_val: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int r2 = (++it)->u.operand;
            out.printf("[%4d] put_by_val\t %s, %s, %s", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data());
            dumpArrayProfiling(out, it, hasPrintedProfiling);
            break;
        }
        case op_del_by_val: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int r2 = (++it)->u.operand;
            out.printf("[%4d] del_by_val\t %s, %s, %s", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data());
            break;
        }
        case op_put_by_index: {
            int r0 = (++it)->u.operand;
            unsigned n0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            out.printf("[%4d] put_by_index\t %s, %u, %s", location, registerName(exec, r0).data(), n0, registerName(exec, r1).data());
            break;
        }
        case op_jmp: {
            int offset = (++it)->u.operand;
            out.printf("[%4d] jmp\t\t %d(->%d)", location, offset, location + offset);
            break;
        }
        case op_jtrue: {
            printConditionalJump(out, exec, begin, it, location, "jtrue");
            break;
        }
        case op_jfalse: {
            printConditionalJump(out, exec, begin, it, location, "jfalse");
            break;
        }
        case op_jeq_null: {
            printConditionalJump(out, exec, begin, it, location, "jeq_null");
            break;
        }
        case op_jneq_null: {
            printConditionalJump(out, exec, begin, it, location, "jneq_null");
            break;
        }
        case op_jneq_ptr: {
            int r0 = (++it)->u.operand;
            Special::Pointer pointer = (++it)->u.specialPointer;
            int offset = (++it)->u.operand;
            out.printf("[%4d] jneq_ptr\t\t %s, %d (%p), %d(->%d)", location, registerName(exec, r0).data(), pointer, m_globalObject->actualPointerFor(pointer), offset, location + offset);
            break;
        }
        case op_jless: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            out.printf("[%4d] jless\t\t %s, %s, %d(->%d)", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_jlesseq: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            out.printf("[%4d] jlesseq\t\t %s, %s, %d(->%d)", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_jgreater: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            out.printf("[%4d] jgreater\t\t %s, %s, %d(->%d)", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_jgreatereq: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            out.printf("[%4d] jgreatereq\t\t %s, %s, %d(->%d)", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_jnless: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            out.printf("[%4d] jnless\t\t %s, %s, %d(->%d)", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_jnlesseq: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            out.printf("[%4d] jnlesseq\t\t %s, %s, %d(->%d)", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_jngreater: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            out.printf("[%4d] jngreater\t\t %s, %s, %d(->%d)", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_jngreatereq: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int offset = (++it)->u.operand;
            out.printf("[%4d] jngreatereq\t\t %s, %s, %d(->%d)", location, registerName(exec, r0).data(), registerName(exec, r1).data(), offset, location + offset);
            break;
        }
        case op_loop_hint: {
            out.printf("[%4d] loop_hint", location);
            break;
        }
        case op_switch_imm: {
            int tableIndex = (++it)->u.operand;
            int defaultTarget = (++it)->u.operand;
            int scrutineeRegister = (++it)->u.operand;
            out.printf("[%4d] switch_imm\t %d, %d(->%d), %s", location, tableIndex, defaultTarget, location + defaultTarget, registerName(exec, scrutineeRegister).data());
            break;
        }
        case op_switch_char: {
            int tableIndex = (++it)->u.operand;
            int defaultTarget = (++it)->u.operand;
            int scrutineeRegister = (++it)->u.operand;
            out.printf("[%4d] switch_char\t %d, %d(->%d), %s", location, tableIndex, defaultTarget, location + defaultTarget, registerName(exec, scrutineeRegister).data());
            break;
        }
        case op_switch_string: {
            int tableIndex = (++it)->u.operand;
            int defaultTarget = (++it)->u.operand;
            int scrutineeRegister = (++it)->u.operand;
            out.printf("[%4d] switch_string\t %d, %d(->%d), %s", location, tableIndex, defaultTarget, location + defaultTarget, registerName(exec, scrutineeRegister).data());
            break;
        }
        case op_new_func: {
            int r0 = (++it)->u.operand;
            int f0 = (++it)->u.operand;
            int shouldCheck = (++it)->u.operand;
            out.printf("[%4d] new_func\t\t %s, f%d, %s", location, registerName(exec, r0).data(), f0, shouldCheck ? "<Checked>" : "<Unchecked>");
            break;
        }
        case op_new_func_exp: {
            int r0 = (++it)->u.operand;
            int f0 = (++it)->u.operand;
            out.printf("[%4d] new_func_exp\t %s, f%d", location, registerName(exec, r0).data(), f0);
            break;
        }
        case op_call: {
            printCallOp(out, exec, location, it, "call", DumpCaches);
            break;
        }
        case op_call_eval: {
            printCallOp(out, exec, location, it, "call_eval", DontDumpCaches);
            break;
        }
        case op_call_varargs: {
            int callee = (++it)->u.operand;
            int thisValue = (++it)->u.operand;
            int arguments = (++it)->u.operand;
            int firstFreeRegister = (++it)->u.operand;
            out.printf("[%4d] call_varargs\t %s, %s, %s, %d", location, registerName(exec, callee).data(), registerName(exec, thisValue).data(), registerName(exec, arguments).data(), firstFreeRegister);
            break;
        }
        case op_tear_off_activation: {
            int r0 = (++it)->u.operand;
            out.printf("[%4d] tear_off_activation\t %s", location, registerName(exec, r0).data());
            break;
        }
        case op_tear_off_arguments: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            out.printf("[%4d] tear_off_arguments %s, %s", location, registerName(exec, r0).data(), registerName(exec, r1).data());
            break;
        }
        case op_ret: {
            int r0 = (++it)->u.operand;
            out.printf("[%4d] ret\t\t %s", location, registerName(exec, r0).data());
            break;
        }
        case op_call_put_result: {
            int r0 = (++it)->u.operand;
            out.printf("[%4d] call_put_result\t\t %s", location, registerName(exec, r0).data());
            dumpValueProfiling(out, it, hasPrintedProfiling);
            break;
        }
        case op_ret_object_or_this: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            out.printf("[%4d] constructor_ret\t\t %s %s", location, registerName(exec, r0).data(), registerName(exec, r1).data());
            break;
        }
        case op_construct: {
            printCallOp(out, exec, location, it, "construct", DumpCaches);
            break;
        }
        case op_strcat: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            int count = (++it)->u.operand;
            out.printf("[%4d] strcat\t\t %s, %s, %d", location, registerName(exec, r0).data(), registerName(exec, r1).data(), count);
            break;
        }
        case op_to_primitive: {
            int r0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            out.printf("[%4d] to_primitive\t %s, %s", location, registerName(exec, r0).data(), registerName(exec, r1).data());
            break;
        }
        case op_get_pnames: {
            int r0 = it[1].u.operand;
            int r1 = it[2].u.operand;
            int r2 = it[3].u.operand;
            int r3 = it[4].u.operand;
            int offset = it[5].u.operand;
            out.printf("[%4d] get_pnames\t %s, %s, %s, %s, %d(->%d)", location, registerName(exec, r0).data(), registerName(exec, r1).data(), registerName(exec, r2).data(), registerName(exec, r3).data(), offset, location + offset);
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
            out.printf("[%4d] next_pname\t %s, %s, %s, %s, %s, %d(->%d)", location, registerName(exec, dest).data(), registerName(exec, base).data(), registerName(exec, i).data(), registerName(exec, size).data(), registerName(exec, iter).data(), offset, location + offset);
            it += OPCODE_LENGTH(op_next_pname) - 1;
            break;
        }
        case op_push_with_scope: {
            int r0 = (++it)->u.operand;
            out.printf("[%4d] push_with_scope\t %s", location, registerName(exec, r0).data());
            break;
        }
        case op_pop_scope: {
            out.printf("[%4d] pop_scope", location);
            break;
        }
        case op_push_name_scope: {
            int id0 = (++it)->u.operand;
            int r1 = (++it)->u.operand;
            unsigned attributes = (++it)->u.operand;
            out.printf("[%4d] push_name_scope \t%s, %s, %u", location, idName(id0, m_identifiers[id0]).data(), registerName(exec, r1).data(), attributes);
            break;
        }
        case op_catch: {
            int r0 = (++it)->u.operand;
            out.printf("[%4d] catch\t\t %s", location, registerName(exec, r0).data());
            break;
        }
        case op_throw: {
            int r0 = (++it)->u.operand;
            out.printf("[%4d] throw\t\t %s", location, registerName(exec, r0).data());
            break;
        }
        case op_throw_static_error: {
            int k0 = (++it)->u.operand;
            int k1 = (++it)->u.operand;
            out.printf("[%4d] throw_static_error\t %s, %s", location, constantName(exec, k0, getConstant(k0)).data(), k1 ? "true" : "false");
            break;
        }
        case op_debug: {
            int debugHookID = (++it)->u.operand;
            int firstLine = (++it)->u.operand;
            int lastLine = (++it)->u.operand;
            int column = (++it)->u.operand;
            out.printf("[%4d] debug\t\t %s, %d, %d, %d", location, debugHookName(debugHookID), firstLine, lastLine, column);
            break;
        }
        case op_profile_will_call: {
            int function = (++it)->u.operand;
            out.printf("[%4d] profile_will_call %s", location, registerName(exec, function).data());
            break;
        }
        case op_profile_did_call: {
            int function = (++it)->u.operand;
            out.printf("[%4d] profile_did_call\t %s", location, registerName(exec, function).data());
            break;
        }
        case op_end: {
            int r0 = (++it)->u.operand;
            out.printf("[%4d] end\t\t %s", location, registerName(exec, r0).data());
            break;
        }
#if ENABLE(LLINT_C_LOOP)
        default:
            RELEASE_ASSERT_NOT_REACHED();
#endif
    }

#if ENABLE(VALUE_PROFILER)
    dumpRareCaseProfile(out, "rare case: ", rareCaseProfileForBytecodeOffset(location), hasPrintedProfiling);
    dumpRareCaseProfile(out, "special fast case: ", specialFastCaseProfileForBytecodeOffset(location), hasPrintedProfiling);
#endif
    
#if ENABLE(DFG_JIT)
    Vector<FrequentExitSite> exitSites = exitProfile().exitSitesFor(location);
    if (!exitSites.isEmpty()) {
        out.print(" !! frequent exits: ");
        CommaPrinter comma;
        for (unsigned i = 0; i < exitSites.size(); ++i)
            out.print(comma, exitSites[i].kind());
    }
#else // ENABLE(DFG_JIT)
    UNUSED_PARAM(location);
#endif // ENABLE(DFG_JIT)
    out.print("\n");
}

void CodeBlock::dumpBytecode(PrintStream& out, unsigned bytecodeOffset)
{
    ExecState* exec = m_globalObject->globalExec();
    const Instruction* it = instructions().begin() + bytecodeOffset;
    dumpBytecode(out, exec, instructions().begin(), it);
}

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

        if (codeBlock->symbolTable() && !codeBlock->symbolTable()->isEmpty()) {
            symbolTableIsNotEmpty++;
            symbolTableTotalSize += (codeBlock->symbolTable()->capacity() * (sizeof(SymbolTable::KeyType) + sizeof(SymbolTable::MappedType)));
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

    dataLogF("Number of live CodeBlocks: %d\n", liveCodeBlockSet.size());
    dataLogF("Size of a single CodeBlock [sizeof(CodeBlock)]: %zu\n", sizeof(CodeBlock));
    dataLogF("Size of all CodeBlocks: %zu\n", totalSize);
    dataLogF("Average size of a CodeBlock: %zu\n", totalSize / liveCodeBlockSet.size());

    dataLogF("Number of FunctionCode CodeBlocks: %zu (%.3f%%)\n", isFunctionCode, static_cast<double>(isFunctionCode) * 100.0 / liveCodeBlockSet.size());
    dataLogF("Number of GlobalCode CodeBlocks: %zu (%.3f%%)\n", isGlobalCode, static_cast<double>(isGlobalCode) * 100.0 / liveCodeBlockSet.size());
    dataLogF("Number of EvalCode CodeBlocks: %zu (%.3f%%)\n", isEvalCode, static_cast<double>(isEvalCode) * 100.0 / liveCodeBlockSet.size());

    dataLogF("Number of CodeBlocks with rare data: %zu (%.3f%%)\n", hasRareData, static_cast<double>(hasRareData) * 100.0 / liveCodeBlockSet.size());

    #define PRINT_STATS(name) dataLogF("Number of CodeBlocks with " #name ": %zu\n", name##IsNotEmpty); dataLogF("Size of all " #name ": %zu\n", name##TotalSize); 
        FOR_EACH_MEMBER_VECTOR(PRINT_STATS)
        FOR_EACH_MEMBER_VECTOR_RARE_DATA(PRINT_STATS)
    #undef PRINT_STATS

    dataLogF("Number of CodeBlocks with evalCodeCache: %zu\n", evalCodeCacheIsNotEmpty);
    dataLogF("Number of CodeBlocks with symbolTable: %zu\n", symbolTableIsNotEmpty);

    dataLogF("Size of all symbolTables: %zu\n", symbolTableTotalSize);

#else
    dataLogF("Dumping CodeBlock statistics is not enabled.\n");
#endif
}

CodeBlock::CodeBlock(CopyParsedBlockTag, CodeBlock& other)
    : m_globalObject(other.m_globalObject)
    , m_heap(other.m_heap)
    , m_numCalleeRegisters(other.m_numCalleeRegisters)
    , m_numVars(other.m_numVars)
    , m_isConstructor(other.m_isConstructor)
    , m_unlinkedCode(*other.m_vm, other.m_ownerExecutable.get(), other.m_unlinkedCode.get())
    , m_ownerExecutable(*other.m_vm, other.m_ownerExecutable.get(), other.m_ownerExecutable.get())
    , m_vm(other.m_vm)
    , m_instructions(other.m_instructions)
    , m_thisRegister(other.m_thisRegister)
    , m_argumentsRegister(other.m_argumentsRegister)
    , m_activationRegister(other.m_activationRegister)
    , m_isStrictMode(other.m_isStrictMode)
    , m_needsActivation(other.m_needsActivation)
    , m_source(other.m_source)
    , m_sourceOffset(other.m_sourceOffset)
    , m_firstLineColumnOffset(other.m_firstLineColumnOffset)
    , m_codeType(other.m_codeType)
    , m_identifiers(other.m_identifiers)
    , m_constantRegisters(other.m_constantRegisters)
    , m_functionDecls(other.m_functionDecls)
    , m_functionExprs(other.m_functionExprs)
    , m_osrExitCounter(0)
    , m_optimizationDelayCounter(0)
    , m_reoptimizationRetryCounter(0)
    , m_resolveOperations(other.m_resolveOperations)
    , m_putToBaseOperations(other.m_putToBaseOperations)
#if ENABLE(JIT)
    , m_canCompileWithDFGState(DFG::CapabilityLevelNotSet)
#endif
{
    setNumParameters(other.numParameters());
    optimizeAfterWarmUp();
    jitAfterWarmUp();

    if (other.m_rareData) {
        createRareDataIfNecessary();
        
        m_rareData->m_exceptionHandlers = other.m_rareData->m_exceptionHandlers;
        m_rareData->m_constantBuffers = other.m_rareData->m_constantBuffers;
        m_rareData->m_immediateSwitchJumpTables = other.m_rareData->m_immediateSwitchJumpTables;
        m_rareData->m_characterSwitchJumpTables = other.m_rareData->m_characterSwitchJumpTables;
        m_rareData->m_stringSwitchJumpTables = other.m_rareData->m_stringSwitchJumpTables;
    }
}

CodeBlock::CodeBlock(ScriptExecutable* ownerExecutable, UnlinkedCodeBlock* unlinkedCodeBlock, JSGlobalObject* globalObject, unsigned baseScopeDepth, PassRefPtr<SourceProvider> sourceProvider, unsigned sourceOffset, unsigned firstLineColumnOffset, PassOwnPtr<CodeBlock> alternative)
    : m_globalObject(globalObject->vm(), ownerExecutable, globalObject)
    , m_heap(&m_globalObject->vm().heap)
    , m_numCalleeRegisters(unlinkedCodeBlock->m_numCalleeRegisters)
    , m_numVars(unlinkedCodeBlock->m_numVars)
    , m_isConstructor(unlinkedCodeBlock->isConstructor())
    , m_unlinkedCode(globalObject->vm(), ownerExecutable, unlinkedCodeBlock)
    , m_ownerExecutable(globalObject->vm(), ownerExecutable, ownerExecutable)
    , m_vm(unlinkedCodeBlock->vm())
    , m_thisRegister(unlinkedCodeBlock->thisRegister())
    , m_argumentsRegister(unlinkedCodeBlock->argumentsRegister())
    , m_activationRegister(unlinkedCodeBlock->activationRegister())
    , m_isStrictMode(unlinkedCodeBlock->isStrictMode())
    , m_needsActivation(unlinkedCodeBlock->needsFullScopeChain())
    , m_source(sourceProvider)
    , m_sourceOffset(sourceOffset)
    , m_firstLineColumnOffset(firstLineColumnOffset)
    , m_codeType(unlinkedCodeBlock->codeType())
    , m_alternative(alternative)
    , m_osrExitCounter(0)
    , m_optimizationDelayCounter(0)
    , m_reoptimizationRetryCounter(0)
{
    m_vm->startedCompiling(this);

    ASSERT(m_source);
    setNumParameters(unlinkedCodeBlock->numParameters());

#if DUMP_CODE_BLOCK_STATISTICS
    liveCodeBlockSet.add(this);
#endif
    setIdentifiers(unlinkedCodeBlock->identifiers());
    setConstantRegisters(unlinkedCodeBlock->constantRegisters());
    if (unlinkedCodeBlock->usesGlobalObject())
        m_constantRegisters[unlinkedCodeBlock->globalObjectRegister()].set(*m_vm, ownerExecutable, globalObject);
    m_functionDecls.grow(unlinkedCodeBlock->numberOfFunctionDecls());
    for (size_t count = unlinkedCodeBlock->numberOfFunctionDecls(), i = 0; i < count; ++i) {
        UnlinkedFunctionExecutable* unlinkedExecutable = unlinkedCodeBlock->functionDecl(i);
        unsigned lineCount = unlinkedExecutable->lineCount();
        unsigned firstLine = ownerExecutable->lineNo() + unlinkedExecutable->firstLineOffset();
        unsigned startColumn = unlinkedExecutable->functionStartColumn();
        startColumn += (unlinkedExecutable->firstLineOffset() ? 1 : ownerExecutable->startColumn());
        unsigned startOffset = sourceOffset + unlinkedExecutable->startOffset();
        unsigned sourceLength = unlinkedExecutable->sourceLength();
        SourceCode code(m_source, startOffset, startOffset + sourceLength, firstLine, startColumn);
        FunctionExecutable* executable = FunctionExecutable::create(*m_vm, code, unlinkedExecutable, firstLine, firstLine + lineCount, startColumn);
        m_functionDecls[i].set(*m_vm, ownerExecutable, executable);
    }

    m_functionExprs.grow(unlinkedCodeBlock->numberOfFunctionExprs());
    for (size_t count = unlinkedCodeBlock->numberOfFunctionExprs(), i = 0; i < count; ++i) {
        UnlinkedFunctionExecutable* unlinkedExecutable = unlinkedCodeBlock->functionExpr(i);
        unsigned lineCount = unlinkedExecutable->lineCount();
        unsigned firstLine = ownerExecutable->lineNo() + unlinkedExecutable->firstLineOffset();
        unsigned startColumn = unlinkedExecutable->functionStartColumn();
        startColumn += (unlinkedExecutable->firstLineOffset() ? 1 : ownerExecutable->startColumn());
        unsigned startOffset = sourceOffset + unlinkedExecutable->startOffset();
        unsigned sourceLength = unlinkedExecutable->sourceLength();
        SourceCode code(m_source, startOffset, startOffset + sourceLength, firstLine, startColumn);
        FunctionExecutable* executable = FunctionExecutable::create(*m_vm, code, unlinkedExecutable, firstLine, firstLine + lineCount, startColumn);
        m_functionExprs[i].set(*m_vm, ownerExecutable, executable);
    }

    if (unlinkedCodeBlock->hasRareData()) {
        createRareDataIfNecessary();
        if (size_t count = unlinkedCodeBlock->constantBufferCount()) {
            m_rareData->m_constantBuffers.grow(count);
            for (size_t i = 0; i < count; i++) {
                const UnlinkedCodeBlock::ConstantBuffer& buffer = unlinkedCodeBlock->constantBuffer(i);
                m_rareData->m_constantBuffers[i] = buffer;
            }
        }
        if (size_t count = unlinkedCodeBlock->numberOfExceptionHandlers()) {
            m_rareData->m_exceptionHandlers.grow(count);
            for (size_t i = 0; i < count; i++) {
                const UnlinkedHandlerInfo& handler = unlinkedCodeBlock->exceptionHandler(i);
                m_rareData->m_exceptionHandlers[i].start = handler.start;
                m_rareData->m_exceptionHandlers[i].end = handler.end;
                m_rareData->m_exceptionHandlers[i].target = handler.target;
                m_rareData->m_exceptionHandlers[i].scopeDepth = handler.scopeDepth + baseScopeDepth;
#if ENABLE(JIT) && ENABLE(LLINT)
                m_rareData->m_exceptionHandlers[i].nativeCode = CodeLocationLabel(MacroAssemblerCodePtr::createFromExecutableAddress(LLInt::getCodePtr(llint_op_catch)));
#endif
            }
        }

        if (size_t count = unlinkedCodeBlock->numberOfStringSwitchJumpTables()) {
            m_rareData->m_stringSwitchJumpTables.grow(count);
            for (size_t i = 0; i < count; i++) {
                UnlinkedStringJumpTable::StringOffsetTable::iterator ptr = unlinkedCodeBlock->stringSwitchJumpTable(i).offsetTable.begin();
                UnlinkedStringJumpTable::StringOffsetTable::iterator end = unlinkedCodeBlock->stringSwitchJumpTable(i).offsetTable.end();
                for (; ptr != end; ++ptr) {
                    OffsetLocation offset;
                    offset.branchOffset = ptr->value;
                    m_rareData->m_stringSwitchJumpTables[i].offsetTable.add(ptr->key, offset);
                }
            }
        }

        if (size_t count = unlinkedCodeBlock->numberOfImmediateSwitchJumpTables()) {
            m_rareData->m_immediateSwitchJumpTables.grow(count);
            for (size_t i = 0; i < count; i++) {
                UnlinkedSimpleJumpTable& sourceTable = unlinkedCodeBlock->immediateSwitchJumpTable(i);
                SimpleJumpTable& destTable = m_rareData->m_immediateSwitchJumpTables[i];
                destTable.branchOffsets = sourceTable.branchOffsets;
                destTable.min = sourceTable.min;
            }
        }

        if (size_t count = unlinkedCodeBlock->numberOfCharacterSwitchJumpTables()) {
            m_rareData->m_characterSwitchJumpTables.grow(count);
            for (size_t i = 0; i < count; i++) {
                UnlinkedSimpleJumpTable& sourceTable = unlinkedCodeBlock->characterSwitchJumpTable(i);
                SimpleJumpTable& destTable = m_rareData->m_characterSwitchJumpTables[i];
                destTable.branchOffsets = sourceTable.branchOffsets;
                destTable.min = sourceTable.min;
            }
        }
    }

    // Allocate metadata buffers for the bytecode
#if ENABLE(LLINT)
    if (size_t size = unlinkedCodeBlock->numberOfLLintCallLinkInfos())
        m_llintCallLinkInfos.grow(size);
#endif
#if ENABLE(DFG_JIT)
    if (size_t size = unlinkedCodeBlock->numberOfArrayProfiles())
        m_arrayProfiles.grow(size);
    if (size_t size = unlinkedCodeBlock->numberOfArrayAllocationProfiles())
        m_arrayAllocationProfiles.grow(size);
    if (size_t size = unlinkedCodeBlock->numberOfValueProfiles())
        m_valueProfiles.grow(size);
#endif
    if (size_t size = unlinkedCodeBlock->numberOfObjectAllocationProfiles())
        m_objectAllocationProfiles.grow(size);
    if (size_t size = unlinkedCodeBlock->numberOfResolveOperations())
        m_resolveOperations.grow(size);
    if (size_t putToBaseCount = unlinkedCodeBlock->numberOfPutToBaseOperations()) {
        m_putToBaseOperations.reserveInitialCapacity(putToBaseCount);
        for (size_t i = 0; i < putToBaseCount; ++i)
            m_putToBaseOperations.uncheckedAppend(PutToBaseOperation(isStrictMode()));
    }

    // Copy and translate the UnlinkedInstructions
    size_t instructionCount = unlinkedCodeBlock->instructions().size();
    UnlinkedInstruction* pc = unlinkedCodeBlock->instructions().data();
    Vector<Instruction, 0, UnsafeVectorOverflow> instructions(instructionCount);
    for (size_t i = 0; i < unlinkedCodeBlock->instructions().size(); ) {
        unsigned opLength = opcodeLength(pc[i].u.opcode);
        instructions[i] = vm()->interpreter->getOpcode(pc[i].u.opcode);
        for (size_t j = 1; j < opLength; ++j) {
            if (sizeof(int32_t) != sizeof(intptr_t))
                instructions[i + j].u.pointer = 0;
            instructions[i + j].u.operand = pc[i + j].u.operand;
        }
        switch (pc[i].u.opcode) {
#if ENABLE(DFG_JIT)
        case op_get_by_val:
        case op_get_argument_by_val: {
            int arrayProfileIndex = pc[i + opLength - 2].u.operand;
            m_arrayProfiles[arrayProfileIndex] = ArrayProfile(i);

            instructions[i + opLength - 2] = &m_arrayProfiles[arrayProfileIndex];
            // fallthrough
        }
        case op_convert_this:
        case op_get_by_id:
        case op_call_put_result:
        case op_get_callee: {
            ValueProfile* profile = &m_valueProfiles[pc[i + opLength - 1].u.operand];
            ASSERT(profile->m_bytecodeOffset == -1);
            profile->m_bytecodeOffset = i;
            instructions[i + opLength - 1] = profile;
            break;
        }
        case op_put_by_val: {
            int arrayProfileIndex = pc[i + opLength - 1].u.operand;
            m_arrayProfiles[arrayProfileIndex] = ArrayProfile(i);
            instructions[i + opLength - 1] = &m_arrayProfiles[arrayProfileIndex];
            break;
        }

        case op_new_array:
        case op_new_array_buffer:
        case op_new_array_with_size: {
            int arrayAllocationProfileIndex = pc[i + opLength - 1].u.operand;
            instructions[i + opLength - 1] = &m_arrayAllocationProfiles[arrayAllocationProfileIndex];
            break;
        }
#endif
        case op_resolve_base:
        case op_resolve_base_to_global:
        case op_resolve_base_to_global_dynamic:
        case op_resolve_base_to_scope:
        case op_resolve_base_to_scope_with_top_scope_check: {
            instructions[i + 4].u.resolveOperations = &m_resolveOperations[pc[i + 4].u.operand];
            instructions[i + 5].u.putToBaseOperation = &m_putToBaseOperations[pc[i + 5].u.operand];
#if ENABLE(DFG_JIT)
            ValueProfile* profile = &m_valueProfiles[pc[i + opLength - 1].u.operand];
            ASSERT(profile->m_bytecodeOffset == -1);
            profile->m_bytecodeOffset = i;
            ASSERT((opLength - 1) > 5);
            instructions[i + opLength - 1] = profile;
#endif
            break;
        }
        case op_resolve_global_property:
        case op_resolve_global_var:
        case op_resolve_scoped_var:
        case op_resolve_scoped_var_on_top_scope:
        case op_resolve_scoped_var_with_top_scope_check: {
            instructions[i + 3].u.resolveOperations = &m_resolveOperations[pc[i + 3].u.operand];
            break;
        }
        case op_put_to_base:
        case op_put_to_base_variable: {
            instructions[i + 4].u.putToBaseOperation = &m_putToBaseOperations[pc[i + 4].u.operand];
            break;
        }
        case op_resolve: {
#if ENABLE(DFG_JIT)
            ValueProfile* profile = &m_valueProfiles[pc[i + opLength - 1].u.operand];
            ASSERT(profile->m_bytecodeOffset == -1);
            profile->m_bytecodeOffset = i;
            ASSERT((opLength - 1) > 3);
            instructions[i + opLength - 1] = profile;
#endif
            instructions[i + 3].u.resolveOperations = &m_resolveOperations[pc[i + 3].u.operand];
            break;
        }
        case op_resolve_with_base:
        case op_resolve_with_this: {
            instructions[i + 4].u.resolveOperations = &m_resolveOperations[pc[i + 4].u.operand];
            if (pc[i].u.opcode != op_resolve_with_this)
                instructions[i + 5].u.putToBaseOperation = &m_putToBaseOperations[pc[i + 5].u.operand];
#if ENABLE(DFG_JIT)
            ValueProfile* profile = &m_valueProfiles[pc[i + opLength - 1].u.operand];
            ASSERT(profile->m_bytecodeOffset == -1);
            profile->m_bytecodeOffset = i;
            instructions[i + opLength - 1] = profile;
#endif
            break;
        }
        case op_new_object: {
            int objectAllocationProfileIndex = pc[i + opLength - 1].u.operand;
            ObjectAllocationProfile* objectAllocationProfile = &m_objectAllocationProfiles[objectAllocationProfileIndex];
            int inferredInlineCapacity = pc[i + opLength - 2].u.operand;

            instructions[i + opLength - 1] = objectAllocationProfile;
            objectAllocationProfile->initialize(*vm(),
                m_ownerExecutable.get(), m_globalObject->objectPrototype(), inferredInlineCapacity);
            break;
        }

        case op_get_scoped_var: {
#if ENABLE(DFG_JIT)
            ValueProfile* profile = &m_valueProfiles[pc[i + opLength - 1].u.operand];
            ASSERT(profile->m_bytecodeOffset == -1);
            profile->m_bytecodeOffset = i;
            instructions[i + opLength - 1] = profile;
#endif
            break;
        }

        case op_call:
        case op_call_eval: {
#if ENABLE(DFG_JIT)
            int arrayProfileIndex = pc[i + opLength - 1].u.operand;
            m_arrayProfiles[arrayProfileIndex] = ArrayProfile(i);
            instructions[i + opLength - 1] = &m_arrayProfiles[arrayProfileIndex];
#endif
#if ENABLE(LLINT)
            instructions[i + 4] = &m_llintCallLinkInfos[pc[i + 4].u.operand];
#endif
            break;
        }
        case op_construct:
#if ENABLE(LLINT)
            instructions[i + 4] = &m_llintCallLinkInfos[pc[i + 4].u.operand];
#endif
            break;
        case op_get_by_id_out_of_line:
        case op_get_by_id_self:
        case op_get_by_id_proto:
        case op_get_by_id_chain:
        case op_get_by_id_getter_self:
        case op_get_by_id_getter_proto:
        case op_get_by_id_getter_chain:
        case op_get_by_id_custom_self:
        case op_get_by_id_custom_proto:
        case op_get_by_id_custom_chain:
        case op_get_by_id_generic:
        case op_get_array_length:
        case op_get_string_length:
            CRASH();

        case op_init_global_const_nop: {
            ASSERT(codeType() == GlobalCode);
            Identifier ident = identifier(pc[i + 4].u.operand);
            SymbolTableEntry entry = globalObject->symbolTable()->get(ident.impl());
            if (entry.isNull())
                break;

            if (entry.couldBeWatched()) {
                instructions[i + 0] = vm()->interpreter->getOpcode(op_init_global_const_check);
                instructions[i + 1] = &globalObject->registerAt(entry.getIndex());
                instructions[i + 3] = entry.addressOfIsWatched();
                break;
            }

            instructions[i + 0] = vm()->interpreter->getOpcode(op_init_global_const);
            instructions[i + 1] = &globalObject->registerAt(entry.getIndex());
            break;
        }

        case op_debug: {
            instructions[i + 4] = columnNumberForBytecodeOffset(i);
            break;
        }

        default:
            break;
        }
        i += opLength;
    }
    m_instructions = WTF::RefCountedArray<Instruction>(instructions);

    // Set optimization thresholds only after m_instructions is initialized, since these
    // rely on the instruction count (and are in theory permitted to also inspect the
    // instruction stream to more accurate assess the cost of tier-up).
    optimizeAfterWarmUp();
    jitAfterWarmUp();

    if (Options::dumpGeneratedBytecodes())
        dumpBytecode();
    m_vm->finishedCompiling(this);
}

CodeBlock::~CodeBlock()
{
    if (m_vm->m_perBytecodeProfiler)
        m_vm->m_perBytecodeProfiler->notifyDestruction(this);
    
#if ENABLE(DFG_JIT)
    // Remove myself from the set of DFG code blocks. Note that I may not be in this set
    // (because I'm not a DFG code block), in which case this is a no-op anyway.
    m_vm->heap.m_dfgCodeBlocks.m_set.remove(this);
#endif
    
#if ENABLE(VERBOSE_VALUE_PROFILE)
    dumpValueProfiles();
#endif

#if ENABLE(LLINT)    
    while (m_incomingLLIntCalls.begin() != m_incomingLLIntCalls.end())
        m_incomingLLIntCalls.begin()->remove();
#endif // ENABLE(LLINT)
#if ENABLE(JIT)
    // We may be destroyed before any CodeBlocks that refer to us are destroyed.
    // Consider that two CodeBlocks become unreachable at the same time. There
    // is no guarantee about the order in which the CodeBlocks are destroyed.
    // So, if we don't remove incoming calls, and get destroyed before the
    // CodeBlock(s) that have calls into us, then the CallLinkInfo vector's
    // destructor will try to remove nodes from our (no longer valid) linked list.
    while (m_incomingCalls.begin() != m_incomingCalls.end())
        m_incomingCalls.begin()->remove();
    
    // Note that our outgoing calls will be removed from other CodeBlocks'
    // m_incomingCalls linked lists through the execution of the ~CallLinkInfo
    // destructors.

    for (size_t size = m_structureStubInfos.size(), i = 0; i < size; ++i)
        m_structureStubInfos[i].deref();
#endif // ENABLE(JIT)

#if DUMP_CODE_BLOCK_STATISTICS
    liveCodeBlockSet.remove(this);
#endif
}

void CodeBlock::setNumParameters(int newValue)
{
    m_numParameters = newValue;

#if ENABLE(VALUE_PROFILER)
    m_argumentValueProfiles.resizeToFit(newValue);
#endif
}

void CodeBlock::visitStructures(SlotVisitor& visitor, Instruction* vPC)
{
    Interpreter* interpreter = m_vm->interpreter;

    if (vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id) && vPC[4].u.structure) {
        visitor.append(&vPC[4].u.structure);
        return;
    }

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
        if (vPC[5].u.structureChain)
            visitor.append(&vPC[5].u.structureChain);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id_transition)) {
        visitor.append(&vPC[4].u.structure);
        visitor.append(&vPC[5].u.structure);
        if (vPC[6].u.structureChain)
            visitor.append(&vPC[6].u.structureChain);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id) && vPC[4].u.structure) {
        visitor.append(&vPC[4].u.structure);
        return;
    }
    if (vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id_replace)) {
        visitor.append(&vPC[4].u.structure);
        return;
    }

    // These instructions don't ref their Structures.
    ASSERT(vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id) || vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id) || vPC[0].u.opcode == interpreter->getOpcode(op_get_by_id_generic) || vPC[0].u.opcode == interpreter->getOpcode(op_put_by_id_generic) || vPC[0].u.opcode == interpreter->getOpcode(op_get_array_length) || vPC[0].u.opcode == interpreter->getOpcode(op_get_string_length));
}

void EvalCodeCache::visitAggregate(SlotVisitor& visitor)
{
    EvalCacheMap::iterator end = m_cacheMap.end();
    for (EvalCacheMap::iterator ptr = m_cacheMap.begin(); ptr != end; ++ptr)
        visitor.append(&ptr->value);
}

void CodeBlock::visitAggregate(SlotVisitor& visitor)
{
#if ENABLE(PARALLEL_GC) && ENABLE(DFG_JIT)
    if (!!m_dfgData) {
        // I may be asked to scan myself more than once, and it may even happen concurrently.
        // To this end, use a CAS loop to check if I've been called already. Only one thread
        // may proceed past this point - whichever one wins the CAS race.
        unsigned oldValue;
        do {
            oldValue = m_dfgData->visitAggregateHasBeenCalled;
            if (oldValue) {
                // Looks like someone else won! Return immediately to ensure that we don't
                // trace the same CodeBlock concurrently. Doing so is hazardous since we will
                // be mutating the state of ValueProfiles, which contain JSValues, which can
                // have word-tearing on 32-bit, leading to awesome timing-dependent crashes
                // that are nearly impossible to track down.
                
                // Also note that it must be safe to return early as soon as we see the
                // value true (well, (unsigned)1), since once a GC thread is in this method
                // and has won the CAS race (i.e. was responsible for setting the value true)
                // it will definitely complete the rest of this method before declaring
                // termination.
                return;
            }
        } while (!WTF::weakCompareAndSwap(&m_dfgData->visitAggregateHasBeenCalled, 0, 1));
    }
#endif // ENABLE(PARALLEL_GC) && ENABLE(DFG_JIT)
    
    if (!!m_alternative)
        m_alternative->visitAggregate(visitor);

    visitor.append(&m_unlinkedCode);

    // There are three things that may use unconditional finalizers: lazy bytecode freeing,
    // inline cache clearing, and jettisoning. The probability of us wanting to do at
    // least one of those things is probably quite close to 1. So we add one no matter what
    // and when it runs, it figures out whether it has any work to do.
    visitor.addUnconditionalFinalizer(this);
    
    if (shouldImmediatelyAssumeLivenessDuringScan()) {
        // This code block is live, so scan all references strongly and return.
        stronglyVisitStrongReferences(visitor);
        stronglyVisitWeakReferences(visitor);
        return;
    }
    
#if ENABLE(DFG_JIT)
    // We get here if we're live in the sense that our owner executable is live,
    // but we're not yet live for sure in another sense: we may yet decide that this
    // code block should be jettisoned based on its outgoing weak references being
    // stale. Set a flag to indicate that we're still assuming that we're dead, and
    // perform one round of determining if we're live. The GC may determine, based on
    // either us marking additional objects, or by other objects being marked for
    // other reasons, that this iteration should run again; it will notify us of this
    // decision by calling harvestWeakReferences().
    
    m_dfgData->livenessHasBeenProved = false;
    m_dfgData->allTransitionsHaveBeenMarked = false;
    
    performTracingFixpointIteration(visitor);

    // GC doesn't have enough information yet for us to decide whether to keep our DFG
    // data, so we need to register a handler to run again at the end of GC, when more
    // information is available.
    if (!(m_dfgData->livenessHasBeenProved && m_dfgData->allTransitionsHaveBeenMarked))
        visitor.addWeakReferenceHarvester(this);
    
#else // ENABLE(DFG_JIT)
    RELEASE_ASSERT_NOT_REACHED();
#endif // ENABLE(DFG_JIT)
}

void CodeBlock::performTracingFixpointIteration(SlotVisitor& visitor)
{
    UNUSED_PARAM(visitor);
    
#if ENABLE(DFG_JIT)
    // Evaluate our weak reference transitions, if there are still some to evaluate.
    if (!m_dfgData->allTransitionsHaveBeenMarked) {
        bool allAreMarkedSoFar = true;
        for (unsigned i = 0; i < m_dfgData->transitions.size(); ++i) {
            if ((!m_dfgData->transitions[i].m_codeOrigin
                 || Heap::isMarked(m_dfgData->transitions[i].m_codeOrigin.get()))
                && Heap::isMarked(m_dfgData->transitions[i].m_from.get())) {
                // If the following three things are live, then the target of the
                // transition is also live:
                // - This code block. We know it's live already because otherwise
                //   we wouldn't be scanning ourselves.
                // - The code origin of the transition. Transitions may arise from
                //   code that was inlined. They are not relevant if the user's
                //   object that is required for the inlinee to run is no longer
                //   live.
                // - The source of the transition. The transition checks if some
                //   heap location holds the source, and if so, stores the target.
                //   Hence the source must be live for the transition to be live.
                visitor.append(&m_dfgData->transitions[i].m_to);
            } else
                allAreMarkedSoFar = false;
        }
        
        if (allAreMarkedSoFar)
            m_dfgData->allTransitionsHaveBeenMarked = true;
    }
    
    // Check if we have any remaining work to do.
    if (m_dfgData->livenessHasBeenProved)
        return;
    
    // Now check all of our weak references. If all of them are live, then we
    // have proved liveness and so we scan our strong references. If at end of
    // GC we still have not proved liveness, then this code block is toast.
    bool allAreLiveSoFar = true;
    for (unsigned i = 0; i < m_dfgData->weakReferences.size(); ++i) {
        if (!Heap::isMarked(m_dfgData->weakReferences[i].get())) {
            allAreLiveSoFar = false;
            break;
        }
    }
    
    // If some weak references are dead, then this fixpoint iteration was
    // unsuccessful.
    if (!allAreLiveSoFar)
        return;
    
    // All weak references are live. Record this information so we don't
    // come back here again, and scan the strong references.
    m_dfgData->livenessHasBeenProved = true;
    stronglyVisitStrongReferences(visitor);
#endif // ENABLE(DFG_JIT)
}

void CodeBlock::visitWeakReferences(SlotVisitor& visitor)
{
    performTracingFixpointIteration(visitor);
}

#if ENABLE(JIT_VERBOSE_OSR)
static const bool verboseUnlinking = true;
#else
static const bool verboseUnlinking = false;
#endif

void CodeBlock::finalizeUnconditionally()
{
#if ENABLE(LLINT)
    Interpreter* interpreter = m_vm->interpreter;
    if (!!numberOfInstructions()) {
        const Vector<unsigned>& propertyAccessInstructions = m_unlinkedCode->propertyAccessInstructions();
        for (size_t size = propertyAccessInstructions.size(), i = 0; i < size; ++i) {
            Instruction* curInstruction = &instructions()[propertyAccessInstructions[i]];
            switch (interpreter->getOpcodeID(curInstruction[0].u.opcode)) {
            case op_get_by_id:
            case op_get_by_id_out_of_line:
            case op_put_by_id:
            case op_put_by_id_out_of_line:
                if (!curInstruction[4].u.structure || Heap::isMarked(curInstruction[4].u.structure.get()))
                    break;
                if (verboseUnlinking)
                    dataLogF("Clearing LLInt property access with structure %p.\n", curInstruction[4].u.structure.get());
                curInstruction[4].u.structure.clear();
                curInstruction[5].u.operand = 0;
                break;
            case op_put_by_id_transition_direct:
            case op_put_by_id_transition_normal:
            case op_put_by_id_transition_direct_out_of_line:
            case op_put_by_id_transition_normal_out_of_line:
                if (Heap::isMarked(curInstruction[4].u.structure.get())
                    && Heap::isMarked(curInstruction[6].u.structure.get())
                    && Heap::isMarked(curInstruction[7].u.structureChain.get()))
                    break;
                if (verboseUnlinking) {
                    dataLogF("Clearing LLInt put transition with structures %p -> %p, chain %p.\n",
                            curInstruction[4].u.structure.get(),
                            curInstruction[6].u.structure.get(),
                            curInstruction[7].u.structureChain.get());
                }
                curInstruction[4].u.structure.clear();
                curInstruction[6].u.structure.clear();
                curInstruction[7].u.structureChain.clear();
                curInstruction[0].u.opcode = interpreter->getOpcode(op_put_by_id);
                break;
            case op_get_array_length:
                break;
            default:
                RELEASE_ASSERT_NOT_REACHED();
            }
        }

        for (unsigned i = 0; i < m_llintCallLinkInfos.size(); ++i) {
            if (m_llintCallLinkInfos[i].isLinked() && !Heap::isMarked(m_llintCallLinkInfos[i].callee.get())) {
                if (verboseUnlinking)
                    dataLog("Clearing LLInt call from ", *this, "\n");
                m_llintCallLinkInfos[i].unlink();
            }
            if (!!m_llintCallLinkInfos[i].lastSeenCallee && !Heap::isMarked(m_llintCallLinkInfos[i].lastSeenCallee.get()))
                m_llintCallLinkInfos[i].lastSeenCallee.clear();
        }
    }
#endif // ENABLE(LLINT)

#if ENABLE(DFG_JIT)
    // Check if we're not live. If we are, then jettison.
    if (!(shouldImmediatelyAssumeLivenessDuringScan() || m_dfgData->livenessHasBeenProved)) {
        if (verboseUnlinking)
            dataLog(*this, " has dead weak references, jettisoning during GC.\n");

        if (DFG::shouldShowDisassembly()) {
            dataLog(*this, " will be jettisoned because of the following dead references:\n");
            for (unsigned i = 0; i < m_dfgData->transitions.size(); ++i) {
                WeakReferenceTransition& transition = m_dfgData->transitions[i];
                JSCell* origin = transition.m_codeOrigin.get();
                JSCell* from = transition.m_from.get();
                JSCell* to = transition.m_to.get();
                if ((!origin || Heap::isMarked(origin)) && Heap::isMarked(from))
                    continue;
                dataLog("    Transition under ", JSValue(origin), ", ", JSValue(from), " -> ", JSValue(to), ".\n");
            }
            for (unsigned i = 0; i < m_dfgData->weakReferences.size(); ++i) {
                JSCell* weak = m_dfgData->weakReferences[i].get();
                if (Heap::isMarked(weak))
                    continue;
                dataLog("    Weak reference ", JSValue(weak), ".\n");
            }
        }
        
        jettison();
        return;
    }
#endif // ENABLE(DFG_JIT)

    for (size_t size = m_putToBaseOperations.size(), i = 0; i < size; ++i) {
        if (m_putToBaseOperations[i].m_structure && !Heap::isMarked(m_putToBaseOperations[i].m_structure.get())) {
            if (verboseUnlinking)
                dataLog("Clearing putToBase info in ", *this, "\n");
            m_putToBaseOperations[i].m_structure.clear();
        }
    }
    for (size_t size = m_resolveOperations.size(), i = 0; i < size; ++i) {
        if (m_resolveOperations[i].isEmpty())
            continue;
#ifndef NDEBUG
        for (size_t insnSize = m_resolveOperations[i].size() - 1, k = 0; k < insnSize; ++k)
            ASSERT(!m_resolveOperations[i][k].m_structure);
#endif
        m_resolveOperations[i].last().m_structure.clear();
        if (m_resolveOperations[i].last().m_structure && !Heap::isMarked(m_resolveOperations[i].last().m_structure.get())) {
            if (verboseUnlinking)
                dataLog("Clearing resolve info in ", *this, "\n");
            m_resolveOperations[i].last().m_structure.clear();
        }
    }

#if ENABLE(JIT)
    // Handle inline caches.
    if (!!getJITCode()) {
        RepatchBuffer repatchBuffer(this);
        for (unsigned i = 0; i < numberOfCallLinkInfos(); ++i) {
            if (callLinkInfo(i).isLinked()) {
                if (ClosureCallStubRoutine* stub = callLinkInfo(i).stub.get()) {
                    if (!Heap::isMarked(stub->structure())
                        || !Heap::isMarked(stub->executable())) {
                        if (verboseUnlinking) {
                            dataLog(
                                "Clearing closure call from ", *this, " to ",
                                stub->executable()->hashFor(callLinkInfo(i).specializationKind()),
                                ", stub routine ", RawPointer(stub), ".\n");
                        }
                        callLinkInfo(i).unlink(*m_vm, repatchBuffer);
                    }
                } else if (!Heap::isMarked(callLinkInfo(i).callee.get())) {
                    if (verboseUnlinking) {
                        dataLog(
                            "Clearing call from ", *this, " to ",
                            RawPointer(callLinkInfo(i).callee.get()), " (",
                            callLinkInfo(i).callee.get()->executable()->hashFor(
                                callLinkInfo(i).specializationKind()),
                            ").\n");
                    }
                    callLinkInfo(i).unlink(*m_vm, repatchBuffer);
                }
            }
            if (!!callLinkInfo(i).lastSeenCallee
                && !Heap::isMarked(callLinkInfo(i).lastSeenCallee.get()))
                callLinkInfo(i).lastSeenCallee.clear();
        }
        for (size_t size = m_structureStubInfos.size(), i = 0; i < size; ++i) {
            StructureStubInfo& stubInfo = m_structureStubInfos[i];
            
            if (stubInfo.visitWeakReferences())
                continue;
            
            resetStubDuringGCInternal(repatchBuffer, stubInfo);
        }
    }
#endif
}

#if ENABLE(JIT)
void CodeBlock::resetStub(StructureStubInfo& stubInfo)
{
    if (stubInfo.accessType == access_unset)
        return;
    
    RepatchBuffer repatchBuffer(this);
    resetStubInternal(repatchBuffer, stubInfo);
}

void CodeBlock::resetStubInternal(RepatchBuffer& repatchBuffer, StructureStubInfo& stubInfo)
{
    AccessType accessType = static_cast<AccessType>(stubInfo.accessType);
    
    if (verboseUnlinking)
        dataLog("Clearing structure cache (kind ", static_cast<int>(stubInfo.accessType), ") in ", *this, ".\n");
    
    if (isGetByIdAccess(accessType)) {
        if (getJITCode().jitType() == JITCode::DFGJIT)
            DFG::dfgResetGetByID(repatchBuffer, stubInfo);
        else
            JIT::resetPatchGetById(repatchBuffer, &stubInfo);
    } else {
        ASSERT(isPutByIdAccess(accessType));
        if (getJITCode().jitType() == JITCode::DFGJIT)
            DFG::dfgResetPutByID(repatchBuffer, stubInfo);
        else 
            JIT::resetPatchPutById(repatchBuffer, &stubInfo);
    }
    
    stubInfo.reset();
}

void CodeBlock::resetStubDuringGCInternal(RepatchBuffer& repatchBuffer, StructureStubInfo& stubInfo)
{
    resetStubInternal(repatchBuffer, stubInfo);
    stubInfo.resetByGC = true;
}
#endif

void CodeBlock::stronglyVisitStrongReferences(SlotVisitor& visitor)
{
    visitor.append(&m_globalObject);
    visitor.append(&m_ownerExecutable);
    visitor.append(&m_unlinkedCode);
    if (m_rareData)
        m_rareData->m_evalCodeCache.visitAggregate(visitor);
    visitor.appendValues(m_constantRegisters.data(), m_constantRegisters.size());
    for (size_t i = 0; i < m_functionExprs.size(); ++i)
        visitor.append(&m_functionExprs[i]);
    for (size_t i = 0; i < m_functionDecls.size(); ++i)
        visitor.append(&m_functionDecls[i]);
    for (unsigned i = 0; i < m_objectAllocationProfiles.size(); ++i)
        m_objectAllocationProfiles[i].visitAggregate(visitor);

    updateAllPredictions(Collection);
}

void CodeBlock::stronglyVisitWeakReferences(SlotVisitor& visitor)
{
    UNUSED_PARAM(visitor);

#if ENABLE(DFG_JIT)
    if (!m_dfgData)
        return;

    for (unsigned i = 0; i < m_dfgData->transitions.size(); ++i) {
        if (!!m_dfgData->transitions[i].m_codeOrigin)
            visitor.append(&m_dfgData->transitions[i].m_codeOrigin); // Almost certainly not necessary, since the code origin should also be a weak reference. Better to be safe, though.
        visitor.append(&m_dfgData->transitions[i].m_from);
        visitor.append(&m_dfgData->transitions[i].m_to);
    }
    
    for (unsigned i = 0; i < m_dfgData->weakReferences.size(); ++i)
        visitor.append(&m_dfgData->weakReferences[i]);
#endif    
}

HandlerInfo* CodeBlock::handlerForBytecodeOffset(unsigned bytecodeOffset)
{
    RELEASE_ASSERT(bytecodeOffset < instructions().size());

    if (!m_rareData)
        return 0;
    
    Vector<HandlerInfo>& exceptionHandlers = m_rareData->m_exceptionHandlers;
    for (size_t i = 0; i < exceptionHandlers.size(); ++i) {
        // Handlers are ordered innermost first, so the first handler we encounter
        // that contains the source address is the correct handler to use.
        if (exceptionHandlers[i].start <= bytecodeOffset && exceptionHandlers[i].end > bytecodeOffset)
            return &exceptionHandlers[i];
    }

    return 0;
}

unsigned CodeBlock::lineNumberForBytecodeOffset(unsigned bytecodeOffset)
{
    RELEASE_ASSERT(bytecodeOffset < instructions().size());
    return m_ownerExecutable->lineNo() + m_unlinkedCode->lineNumberForBytecodeOffset(bytecodeOffset);
}

unsigned CodeBlock::columnNumberForBytecodeOffset(unsigned bytecodeOffset)
{
    int divot;
    int startOffset;
    int endOffset;
    unsigned line;
    unsigned column;
    expressionRangeForBytecodeOffset(bytecodeOffset, divot, startOffset, endOffset, line, column);
    return column;
}

void CodeBlock::expressionRangeForBytecodeOffset(unsigned bytecodeOffset, int& divot, int& startOffset, int& endOffset, unsigned& line, unsigned& column)
{
    m_unlinkedCode->expressionRangeForBytecodeOffset(bytecodeOffset, divot, startOffset, endOffset, line, column);
    divot += m_sourceOffset;
    column += line ? 1 : firstLineColumnOffset();
    line += m_ownerExecutable->lineNo();
}

void CodeBlock::shrinkToFit(ShrinkMode shrinkMode)
{
#if ENABLE(LLINT)
    m_llintCallLinkInfos.shrinkToFit();
#endif
#if ENABLE(JIT)
    m_structureStubInfos.shrinkToFit();
    m_callLinkInfos.shrinkToFit();
#endif
#if ENABLE(VALUE_PROFILER)
    m_rareCaseProfiles.shrinkToFit();
    m_specialFastCaseProfiles.shrinkToFit();
#endif
    
    if (shrinkMode == EarlyShrink) {
        m_identifiers.shrinkToFit();
        m_functionDecls.shrinkToFit();
        m_functionExprs.shrinkToFit();
        m_constantRegisters.shrinkToFit();
    } // else don't shrink these, because we would have already pointed pointers into these tables.

    if (m_rareData) {
        m_rareData->m_exceptionHandlers.shrinkToFit();
        m_rareData->m_immediateSwitchJumpTables.shrinkToFit();
        m_rareData->m_characterSwitchJumpTables.shrinkToFit();
        m_rareData->m_stringSwitchJumpTables.shrinkToFit();
#if ENABLE(JIT)
        m_rareData->m_callReturnIndexVector.shrinkToFit();
#endif
#if ENABLE(DFG_JIT)
        m_rareData->m_inlineCallFrames.shrinkToFit();
        m_rareData->m_codeOrigins.shrinkToFit();
#endif
    }
    
#if ENABLE(DFG_JIT)
    if (m_dfgData) {
        m_dfgData->osrEntry.shrinkToFit();
        m_dfgData->osrExit.shrinkToFit();
        m_dfgData->speculationRecovery.shrinkToFit();
        m_dfgData->weakReferences.shrinkToFit();
        m_dfgData->transitions.shrinkToFit();
        m_dfgData->minifiedDFG.prepareAndShrink();
        m_dfgData->variableEventStream.shrinkToFit();
    }
#endif
}

void CodeBlock::createActivation(CallFrame* callFrame)
{
    ASSERT(codeType() == FunctionCode);
    ASSERT(needsFullScopeChain());
    ASSERT(!callFrame->uncheckedR(activationRegister()).jsValue());
    JSActivation* activation = JSActivation::create(callFrame->vm(), callFrame, this);
    callFrame->uncheckedR(activationRegister()) = JSValue(activation);
    callFrame->setScope(activation);
}

unsigned CodeBlock::addOrFindConstant(JSValue v)
{
    unsigned numberOfConstants = numberOfConstantRegisters();
    for (unsigned i = 0; i < numberOfConstants; ++i) {
        if (getConstant(FirstConstantRegisterIndex + i) == v)
            return i;
    }
    return addConstant(v);
}

#if ENABLE(JIT)
void CodeBlock::unlinkCalls()
{
    if (!!m_alternative)
        m_alternative->unlinkCalls();
#if ENABLE(LLINT)
    for (size_t i = 0; i < m_llintCallLinkInfos.size(); ++i) {
        if (m_llintCallLinkInfos[i].isLinked())
            m_llintCallLinkInfos[i].unlink();
    }
#endif
    if (!m_callLinkInfos.size())
        return;
    if (!m_vm->canUseJIT())
        return;
    RepatchBuffer repatchBuffer(this);
    for (size_t i = 0; i < m_callLinkInfos.size(); i++) {
        if (!m_callLinkInfos[i].isLinked())
            continue;
        m_callLinkInfos[i].unlink(*m_vm, repatchBuffer);
    }
}

void CodeBlock::unlinkIncomingCalls()
{
#if ENABLE(LLINT)
    while (m_incomingLLIntCalls.begin() != m_incomingLLIntCalls.end())
        m_incomingLLIntCalls.begin()->unlink();
#endif
    if (m_incomingCalls.isEmpty())
        return;
    RepatchBuffer repatchBuffer(this);
    while (m_incomingCalls.begin() != m_incomingCalls.end())
        m_incomingCalls.begin()->unlink(*m_vm, repatchBuffer);
}
#endif // ENABLE(JIT)

#if ENABLE(LLINT)
Instruction* CodeBlock::adjustPCIfAtCallSite(Instruction* potentialReturnPC)
{
    ASSERT(potentialReturnPC);

    unsigned returnPCOffset = potentialReturnPC - instructions().begin();
    Instruction* adjustedPC;
    unsigned opcodeLength;

    // If we are at a callsite, the LLInt stores the PC after the call
    // instruction rather than the PC of the call instruction. This requires
    // some correcting. If so, we can rely on the fact that the preceding
    // instruction must be one of the call instructions, so either it's a
    // call_varargs or it's a call, construct, or eval.
    //
    // If we are not at a call site, then we need to guard against the
    // possibility of peeking past the start of the bytecode range for this
    // codeBlock. Hence, we do a bounds check before we peek at the
    // potential "preceding" instruction.
    //     The bounds check is done by comparing the offset of the potential
    // returnPC with the length of the opcode. If there is room for a call
    // instruction before the returnPC, then the offset of the returnPC must
    // be greater than the size of the call opcode we're looking for.

    // The determination of the call instruction present (if we are at a
    // callsite) depends on the following assumptions. So, assert that
    // they are still true:
    ASSERT(OPCODE_LENGTH(op_call_varargs) <= OPCODE_LENGTH(op_call));
    ASSERT(OPCODE_LENGTH(op_call) == OPCODE_LENGTH(op_construct));
    ASSERT(OPCODE_LENGTH(op_call) == OPCODE_LENGTH(op_call_eval));

    // Check for the case of a preceeding op_call_varargs:
    opcodeLength = OPCODE_LENGTH(op_call_varargs);
    adjustedPC = potentialReturnPC - opcodeLength;
    if ((returnPCOffset >= opcodeLength)
        && (adjustedPC->u.pointer == LLInt::getCodePtr(llint_op_call_varargs))) {
        return adjustedPC;
    }

    // Check for the case of the other 3 call instructions:
    opcodeLength = OPCODE_LENGTH(op_call);
    adjustedPC = potentialReturnPC - opcodeLength;
    if ((returnPCOffset >= opcodeLength)
        && (adjustedPC->u.pointer == LLInt::getCodePtr(llint_op_call)
            || adjustedPC->u.pointer == LLInt::getCodePtr(llint_op_construct)
            || adjustedPC->u.pointer == LLInt::getCodePtr(llint_op_call_eval))) {
        return adjustedPC;
    }

    // Not a call site. No need to adjust PC. Just return the original.
    return potentialReturnPC;
}
#endif // ENABLE(LLINT)

#if ENABLE(JIT)
ClosureCallStubRoutine* CodeBlock::findClosureCallForReturnPC(ReturnAddressPtr returnAddress)
{
    for (unsigned i = m_callLinkInfos.size(); i--;) {
        CallLinkInfo& info = m_callLinkInfos[i];
        if (!info.stub)
            continue;
        if (!info.stub->code().executableMemory()->contains(returnAddress.value()))
            continue;

        RELEASE_ASSERT(info.stub->codeOrigin().bytecodeIndex < CodeOrigin::maximumBytecodeIndex);
        return info.stub.get();
    }
    
    // The stub routine may have been jettisoned. This is rare, but we have to handle it.
    const JITStubRoutineSet& set = m_vm->heap.jitStubRoutines();
    for (unsigned i = set.size(); i--;) {
        GCAwareJITStubRoutine* genericStub = set.at(i);
        if (!genericStub->isClosureCall())
            continue;
        ClosureCallStubRoutine* stub = static_cast<ClosureCallStubRoutine*>(genericStub);
        if (!stub->code().executableMemory()->contains(returnAddress.value()))
            continue;
        RELEASE_ASSERT(stub->codeOrigin().bytecodeIndex < CodeOrigin::maximumBytecodeIndex);
        return stub;
    }
    
    return 0;
}
#endif

unsigned CodeBlock::bytecodeOffset(ExecState* exec, ReturnAddressPtr returnAddress)
{
    UNUSED_PARAM(exec);
    UNUSED_PARAM(returnAddress);
#if ENABLE(LLINT)
#if !ENABLE(LLINT_C_LOOP)
    // When using the JIT, we could have addresses that are not bytecode
    // addresses. We check if the return address is in the LLint glue and
    // opcode handlers range here to ensure that we are looking at bytecode
    // before attempting to convert the return address into a bytecode offset.
    //
    // In the case of the C Loop LLInt, the JIT is disabled, and the only
    // valid return addresses should be bytecode PCs. So, we can and need to
    // forego this check because when we do not ENABLE(COMPUTED_GOTO_OPCODES),
    // then the bytecode "PC"s are actually the opcodeIDs and are not bounded
    // by llint_begin and llint_end.
    if (returnAddress.value() >= LLInt::getCodePtr(llint_begin)
        && returnAddress.value() <= LLInt::getCodePtr(llint_end))
#endif
    {
        RELEASE_ASSERT(exec->codeBlock());
        RELEASE_ASSERT(exec->codeBlock() == this);
        RELEASE_ASSERT(JITCode::isBaselineCode(getJITType()));
        Instruction* instruction = exec->currentVPC();
        RELEASE_ASSERT(instruction);

        instruction = adjustPCIfAtCallSite(instruction);
        return bytecodeOffset(instruction);
    }
#endif // !ENABLE(LLINT)

#if ENABLE(JIT)
    if (!m_rareData)
        return 1;
    Vector<CallReturnOffsetToBytecodeOffset, 0, UnsafeVectorOverflow>& callIndices = m_rareData->m_callReturnIndexVector;
    if (!callIndices.size())
        return 1;
    
    if (getJITCode().getExecutableMemory()->contains(returnAddress.value())) {
        unsigned callReturnOffset = getJITCode().offsetOf(returnAddress.value());
        CallReturnOffsetToBytecodeOffset* result =
            binarySearch<CallReturnOffsetToBytecodeOffset, unsigned>(
                callIndices, callIndices.size(), callReturnOffset, getCallReturnOffset);
        RELEASE_ASSERT(result->callReturnOffset == callReturnOffset);
        RELEASE_ASSERT(result->bytecodeOffset < instructionCount());
        return result->bytecodeOffset;
    }
    ClosureCallStubRoutine* closureInfo = findClosureCallForReturnPC(returnAddress);
    CodeOrigin origin = closureInfo->codeOrigin();
    while (InlineCallFrame* inlineCallFrame = origin.inlineCallFrame) {
        if (inlineCallFrame->baselineCodeBlock() == this)
            break;
        origin = inlineCallFrame->caller;
        RELEASE_ASSERT(origin.bytecodeIndex < CodeOrigin::maximumBytecodeIndex);
    }
    RELEASE_ASSERT(origin.bytecodeIndex < CodeOrigin::maximumBytecodeIndex);
    unsigned bytecodeIndex = origin.bytecodeIndex;
    RELEASE_ASSERT(bytecodeIndex < instructionCount());
    return bytecodeIndex;
#endif // ENABLE(JIT)

#if !ENABLE(LLINT) && !ENABLE(JIT)
    return 1;
#endif
}

#if ENABLE(DFG_JIT)
bool CodeBlock::codeOriginForReturn(ReturnAddressPtr returnAddress, CodeOrigin& codeOrigin)
{
    if (!hasCodeOrigins())
        return false;

    if (!getJITCode().getExecutableMemory()->contains(returnAddress.value())) {
        ClosureCallStubRoutine* stub = findClosureCallForReturnPC(returnAddress);
        ASSERT(stub);
        if (!stub)
            return false;
        codeOrigin = stub->codeOrigin();
        return true;
    }
    
    unsigned offset = getJITCode().offsetOf(returnAddress.value());
    CodeOriginAtCallReturnOffset* entry =
        tryBinarySearch<CodeOriginAtCallReturnOffset, unsigned>(
            codeOrigins(), codeOrigins().size(), offset,
            getCallReturnOffsetForCodeOrigin);
    if (!entry)
        return false;
    codeOrigin = entry->codeOrigin;
    return true;
}
#endif // ENABLE(DFG_JIT)

void CodeBlock::clearEvalCache()
{
    if (!!m_alternative)
        m_alternative->clearEvalCache();
    if (!m_rareData)
        return;
    m_rareData->m_evalCodeCache.clear();
}

template<typename T, size_t inlineCapacity, typename U, typename V>
inline void replaceExistingEntries(Vector<T, inlineCapacity, U>& target, Vector<T, inlineCapacity, V>& source)
{
    ASSERT(target.size() <= source.size());
    for (size_t i = 0; i < target.size(); ++i)
        target[i] = source[i];
}

void CodeBlock::copyPostParseDataFrom(CodeBlock* alternative)
{
    if (!alternative)
        return;
    
    replaceExistingEntries(m_constantRegisters, alternative->m_constantRegisters);
    replaceExistingEntries(m_functionDecls, alternative->m_functionDecls);
    replaceExistingEntries(m_functionExprs, alternative->m_functionExprs);
    if (!!m_rareData && !!alternative->m_rareData)
        replaceExistingEntries(m_rareData->m_constantBuffers, alternative->m_rareData->m_constantBuffers);
}

void CodeBlock::copyPostParseDataFromAlternative()
{
    copyPostParseDataFrom(m_alternative.get());
}

#if ENABLE(JIT)
void CodeBlock::reoptimize()
{
    ASSERT(replacement() != this);
    ASSERT(replacement()->alternative() == this);
    if (DFG::shouldShowDisassembly())
        dataLog(*replacement(), " will be jettisoned due to reoptimization of ", *this, ".\n");
    replacement()->jettison();
    countReoptimization();
}

CodeBlock* ProgramCodeBlock::replacement()
{
    return &static_cast<ProgramExecutable*>(ownerExecutable())->generatedBytecode();
}

CodeBlock* EvalCodeBlock::replacement()
{
    return &static_cast<EvalExecutable*>(ownerExecutable())->generatedBytecode();
}

CodeBlock* FunctionCodeBlock::replacement()
{
    return &static_cast<FunctionExecutable*>(ownerExecutable())->generatedBytecodeFor(m_isConstructor ? CodeForConstruct : CodeForCall);
}

JSObject* ProgramCodeBlock::compileOptimized(ExecState* exec, JSScope* scope, unsigned bytecodeIndex)
{
    if (replacement()->getJITType() == JITCode::nextTierJIT(getJITType()))
        return 0;
    JSObject* error = static_cast<ProgramExecutable*>(ownerExecutable())->compileOptimized(exec, scope, bytecodeIndex);
    return error;
}

JSObject* EvalCodeBlock::compileOptimized(ExecState* exec, JSScope* scope, unsigned bytecodeIndex)
{
    if (replacement()->getJITType() == JITCode::nextTierJIT(getJITType()))
        return 0;
    JSObject* error = static_cast<EvalExecutable*>(ownerExecutable())->compileOptimized(exec, scope, bytecodeIndex);
    return error;
}

JSObject* FunctionCodeBlock::compileOptimized(ExecState* exec, JSScope* scope, unsigned bytecodeIndex)
{
    if (replacement()->getJITType() == JITCode::nextTierJIT(getJITType()))
        return 0;
    JSObject* error = static_cast<FunctionExecutable*>(ownerExecutable())->compileOptimizedFor(exec, scope, bytecodeIndex, m_isConstructor ? CodeForConstruct : CodeForCall);
    return error;
}

DFG::CapabilityLevel ProgramCodeBlock::canCompileWithDFGInternal()
{
    return DFG::canCompileProgram(this);
}

DFG::CapabilityLevel EvalCodeBlock::canCompileWithDFGInternal()
{
    return DFG::canCompileEval(this);
}

DFG::CapabilityLevel FunctionCodeBlock::canCompileWithDFGInternal()
{
    if (m_isConstructor)
        return DFG::canCompileFunctionForConstruct(this);
    return DFG::canCompileFunctionForCall(this);
}

void CodeBlock::jettison()
{
    ASSERT(JITCode::isOptimizingJIT(getJITType()));
    ASSERT(this == replacement());
    alternative()->optimizeAfterWarmUp();
    tallyFrequentExitSites();
    if (DFG::shouldShowDisassembly())
        dataLog("Jettisoning ", *this, ".\n");
    jettisonImpl();
}

void ProgramCodeBlock::jettisonImpl()
{
    static_cast<ProgramExecutable*>(ownerExecutable())->jettisonOptimizedCode(*vm());
}

void EvalCodeBlock::jettisonImpl()
{
    static_cast<EvalExecutable*>(ownerExecutable())->jettisonOptimizedCode(*vm());
}

void FunctionCodeBlock::jettisonImpl()
{
    static_cast<FunctionExecutable*>(ownerExecutable())->jettisonOptimizedCodeFor(*vm(), m_isConstructor ? CodeForConstruct : CodeForCall);
}

bool ProgramCodeBlock::jitCompileImpl(ExecState* exec)
{
    ASSERT(getJITType() == JITCode::InterpreterThunk);
    ASSERT(this == replacement());
    return static_cast<ProgramExecutable*>(ownerExecutable())->jitCompile(exec);
}

bool EvalCodeBlock::jitCompileImpl(ExecState* exec)
{
    ASSERT(getJITType() == JITCode::InterpreterThunk);
    ASSERT(this == replacement());
    return static_cast<EvalExecutable*>(ownerExecutable())->jitCompile(exec);
}

bool FunctionCodeBlock::jitCompileImpl(ExecState* exec)
{
    ASSERT(getJITType() == JITCode::InterpreterThunk);
    ASSERT(this == replacement());
    return static_cast<FunctionExecutable*>(ownerExecutable())->jitCompileFor(exec, m_isConstructor ? CodeForConstruct : CodeForCall);
}
#endif

JSGlobalObject* CodeBlock::globalObjectFor(CodeOrigin codeOrigin)
{
    if (!codeOrigin.inlineCallFrame)
        return globalObject();
    return jsCast<FunctionExecutable*>(codeOrigin.inlineCallFrame->executable.get())->generatedBytecode().globalObject();
}

unsigned CodeBlock::reoptimizationRetryCounter() const
{
    ASSERT(m_reoptimizationRetryCounter <= Options::reoptimizationRetryCounterMax());
    return m_reoptimizationRetryCounter;
}

void CodeBlock::countReoptimization()
{
    m_reoptimizationRetryCounter++;
    if (m_reoptimizationRetryCounter > Options::reoptimizationRetryCounterMax())
        m_reoptimizationRetryCounter = Options::reoptimizationRetryCounterMax();
}

unsigned CodeBlock::numberOfDFGCompiles()
{
#if ENABLE(JIT)
    ASSERT(JITCode::isBaselineCode(getJITType()));
    return (JITCode::isOptimizingJIT(replacement()->getJITType()) ? 1 : 0) + m_reoptimizationRetryCounter;
#else
    return 0;
#endif
}

int32_t CodeBlock::codeTypeThresholdMultiplier() const
{
    if (codeType() == EvalCode)
        return Options::evalThresholdMultiplier();
    
    return 1;
}

double CodeBlock::optimizationThresholdScalingFactor()
{
    // This expression arises from doing a least-squares fit of
    //
    // F[x_] =: a * Sqrt[x + b] + Abs[c * x] + d
    //
    // against the data points:
    //
    //    x       F[x_]
    //    10       0.9          (smallest reasonable code block)
    //   200       1.0          (typical small-ish code block)
    //   320       1.2          (something I saw in 3d-cube that I wanted to optimize)
    //  1268       5.0          (something I saw in 3d-cube that I didn't want to optimize)
    //  4000       5.5          (random large size, used to cause the function to converge to a shallow curve of some sort)
    // 10000       6.0          (similar to above)
    //
    // I achieve the minimization using the following Mathematica code:
    //
    // MyFunctionTemplate[x_, a_, b_, c_, d_] := a*Sqrt[x + b] + Abs[c*x] + d
    //
    // samples = {{10, 0.9}, {200, 1}, {320, 1.2}, {1268, 5}, {4000, 5.5}, {10000, 6}}
    //
    // solution = 
    //     Minimize[Plus @@ ((MyFunctionTemplate[#[[1]], a, b, c, d] - #[[2]])^2 & /@ samples),
    //         {a, b, c, d}][[2]]
    //
    // And the code below (to initialize a, b, c, d) is generated by:
    //
    // Print["const double " <> ToString[#[[1]]] <> " = " <>
    //     If[#[[2]] < 0.00001, "0.0", ToString[#[[2]]]] <> ";"] & /@ solution
    //
    // We've long known the following to be true:
    // - Small code blocks are cheap to optimize and so we should do it sooner rather
    //   than later.
    // - Large code blocks are expensive to optimize and so we should postpone doing so,
    //   and sometimes have a large enough threshold that we never optimize them.
    // - The difference in cost is not totally linear because (a) just invoking the
    //   DFG incurs some base cost and (b) for large code blocks there is enough slop
    //   in the correlation between instruction count and the actual compilation cost
    //   that for those large blocks, the instruction count should not have a strong
    //   influence on our threshold.
    //
    // I knew the goals but I didn't know how to achieve them; so I picked an interesting
    // example where the heuristics were right (code block in 3d-cube with instruction
    // count 320, which got compiled early as it should have been) and one where they were
    // totally wrong (code block in 3d-cube with instruction count 1268, which was expensive
    // to compile and didn't run often enough to warrant compilation in my opinion), and
    // then threw in additional data points that represented my own guess of what our
    // heuristics should do for some round-numbered examples.
    //
    // The expression to which I decided to fit the data arose because I started with an
    // affine function, and then did two things: put the linear part in an Abs to ensure
    // that the fit didn't end up choosing a negative value of c (which would result in
    // the function turning over and going negative for large x) and I threw in a Sqrt
    // term because Sqrt represents my intution that the function should be more sensitive
    // to small changes in small values of x, but less sensitive when x gets large.
    
    // Note that the current fit essentially eliminates the linear portion of the
    // expression (c == 0.0).
    const double a = 0.061504;
    const double b = 1.02406;
    const double c = 0.0;
    const double d = 0.825914;
    
    double instructionCount = this->instructionCount();
    
    ASSERT(instructionCount); // Make sure this is called only after we have an instruction stream; otherwise it'll just return the value of d, which makes no sense.
    
    double result = d + a * sqrt(instructionCount + b) + c * instructionCount;
#if ENABLE(JIT_VERBOSE_OSR)
    dataLog(*this, ": instruction count is ", instructionCount, ", scaling execution counter by ", result, " * ", codeTypeThresholdMultiplier(), "\n");
#endif
    return result * codeTypeThresholdMultiplier();
}

static int32_t clipThreshold(double threshold)
{
    if (threshold < 1.0)
        return 1;
    
    if (threshold > static_cast<double>(std::numeric_limits<int32_t>::max()))
        return std::numeric_limits<int32_t>::max();
    
    return static_cast<int32_t>(threshold);
}

int32_t CodeBlock::counterValueForOptimizeAfterWarmUp()
{
    return clipThreshold(
        Options::thresholdForOptimizeAfterWarmUp() *
        optimizationThresholdScalingFactor() *
        (1 << reoptimizationRetryCounter()));
}

int32_t CodeBlock::counterValueForOptimizeAfterLongWarmUp()
{
    return clipThreshold(
        Options::thresholdForOptimizeAfterLongWarmUp() *
        optimizationThresholdScalingFactor() *
        (1 << reoptimizationRetryCounter()));
}

int32_t CodeBlock::counterValueForOptimizeSoon()
{
    return clipThreshold(
        Options::thresholdForOptimizeSoon() *
        optimizationThresholdScalingFactor() *
        (1 << reoptimizationRetryCounter()));
}

bool CodeBlock::checkIfOptimizationThresholdReached()
{
    return m_jitExecuteCounter.checkIfThresholdCrossedAndSet(this);
}

void CodeBlock::optimizeNextInvocation()
{
    m_jitExecuteCounter.setNewThreshold(0, this);
}

void CodeBlock::dontOptimizeAnytimeSoon()
{
    m_jitExecuteCounter.deferIndefinitely();
}

void CodeBlock::optimizeAfterWarmUp()
{
    m_jitExecuteCounter.setNewThreshold(counterValueForOptimizeAfterWarmUp(), this);
}

void CodeBlock::optimizeAfterLongWarmUp()
{
    m_jitExecuteCounter.setNewThreshold(counterValueForOptimizeAfterLongWarmUp(), this);
}

void CodeBlock::optimizeSoon()
{
    m_jitExecuteCounter.setNewThreshold(counterValueForOptimizeSoon(), this);
}

#if ENABLE(JIT)
uint32_t CodeBlock::adjustedExitCountThreshold(uint32_t desiredThreshold)
{
    ASSERT(getJITType() == JITCode::DFGJIT);
    // Compute this the lame way so we don't saturate. This is called infrequently
    // enough that this loop won't hurt us.
    unsigned result = desiredThreshold;
    for (unsigned n = baselineVersion()->reoptimizationRetryCounter(); n--;) {
        unsigned newResult = result << 1;
        if (newResult < result)
            return std::numeric_limits<uint32_t>::max();
        result = newResult;
    }
    return result;
}

uint32_t CodeBlock::exitCountThresholdForReoptimization()
{
    return adjustedExitCountThreshold(Options::osrExitCountForReoptimization() * codeTypeThresholdMultiplier());
}

uint32_t CodeBlock::exitCountThresholdForReoptimizationFromLoop()
{
    return adjustedExitCountThreshold(Options::osrExitCountForReoptimizationFromLoop() * codeTypeThresholdMultiplier());
}

bool CodeBlock::shouldReoptimizeNow()
{
    return osrExitCounter() >= exitCountThresholdForReoptimization();
}

bool CodeBlock::shouldReoptimizeFromLoopNow()
{
    return osrExitCounter() >= exitCountThresholdForReoptimizationFromLoop();
}
#endif

#if ENABLE(VALUE_PROFILER)
ArrayProfile* CodeBlock::getArrayProfile(unsigned bytecodeOffset)
{
    for (unsigned i = 0; i < m_arrayProfiles.size(); ++i) {
        if (m_arrayProfiles[i].bytecodeOffset() == bytecodeOffset)
            return &m_arrayProfiles[i];
    }
    return 0;
}

ArrayProfile* CodeBlock::getOrAddArrayProfile(unsigned bytecodeOffset)
{
    ArrayProfile* result = getArrayProfile(bytecodeOffset);
    if (result)
        return result;
    return addArrayProfile(bytecodeOffset);
}

void CodeBlock::updateAllPredictionsAndCountLiveness(
    OperationInProgress operation, unsigned& numberOfLiveNonArgumentValueProfiles, unsigned& numberOfSamplesInProfiles)
{
    numberOfLiveNonArgumentValueProfiles = 0;
    numberOfSamplesInProfiles = 0; // If this divided by ValueProfile::numberOfBuckets equals numberOfValueProfiles() then value profiles are full.
    for (unsigned i = 0; i < totalNumberOfValueProfiles(); ++i) {
        ValueProfile* profile = getFromAllValueProfiles(i);
        unsigned numSamples = profile->totalNumberOfSamples();
        if (numSamples > ValueProfile::numberOfBuckets)
            numSamples = ValueProfile::numberOfBuckets; // We don't want profiles that are extremely hot to be given more weight.
        numberOfSamplesInProfiles += numSamples;
        if (profile->m_bytecodeOffset < 0) {
            profile->computeUpdatedPrediction(operation);
            continue;
        }
        if (profile->numberOfSamples() || profile->m_prediction != SpecNone)
            numberOfLiveNonArgumentValueProfiles++;
        profile->computeUpdatedPrediction(operation);
    }
    
#if ENABLE(DFG_JIT)
    m_lazyOperandValueProfiles.computeUpdatedPredictions(operation);
#endif
}

void CodeBlock::updateAllValueProfilePredictions(OperationInProgress operation)
{
    unsigned ignoredValue1, ignoredValue2;
    updateAllPredictionsAndCountLiveness(operation, ignoredValue1, ignoredValue2);
}

void CodeBlock::updateAllArrayPredictions(OperationInProgress operation)
{
    for (unsigned i = m_arrayProfiles.size(); i--;)
        m_arrayProfiles[i].computeUpdatedPrediction(this, operation);
    
    // Don't count these either, for similar reasons.
    for (unsigned i = m_arrayAllocationProfiles.size(); i--;)
        m_arrayAllocationProfiles[i].updateIndexingType();
}

void CodeBlock::updateAllPredictions(OperationInProgress operation)
{
    updateAllValueProfilePredictions(operation);
    updateAllArrayPredictions(operation);
}

bool CodeBlock::shouldOptimizeNow()
{
#if ENABLE(JIT_VERBOSE_OSR)
    dataLog("Considering optimizing ", *this, "...\n");
#endif

#if ENABLE(VERBOSE_VALUE_PROFILE)
    dumpValueProfiles();
#endif

    if (m_optimizationDelayCounter >= Options::maximumOptimizationDelay())
        return true;
    
    updateAllArrayPredictions();
    
    unsigned numberOfLiveNonArgumentValueProfiles;
    unsigned numberOfSamplesInProfiles;
    updateAllPredictionsAndCountLiveness(NoOperation, numberOfLiveNonArgumentValueProfiles, numberOfSamplesInProfiles);

#if ENABLE(JIT_VERBOSE_OSR)
    dataLogF("Profile hotness: %lf (%u / %u), %lf (%u / %u)\n", (double)numberOfLiveNonArgumentValueProfiles / numberOfValueProfiles(), numberOfLiveNonArgumentValueProfiles, numberOfValueProfiles(), (double)numberOfSamplesInProfiles / ValueProfile::numberOfBuckets / numberOfValueProfiles(), numberOfSamplesInProfiles, ValueProfile::numberOfBuckets * numberOfValueProfiles());
#endif

    if ((!numberOfValueProfiles() || (double)numberOfLiveNonArgumentValueProfiles / numberOfValueProfiles() >= Options::desiredProfileLivenessRate())
        && (!totalNumberOfValueProfiles() || (double)numberOfSamplesInProfiles / ValueProfile::numberOfBuckets / totalNumberOfValueProfiles() >= Options::desiredProfileFullnessRate())
        && static_cast<unsigned>(m_optimizationDelayCounter) + 1 >= Options::minimumOptimizationDelay())
        return true;
    
    ASSERT(m_optimizationDelayCounter < std::numeric_limits<uint8_t>::max());
    m_optimizationDelayCounter++;
    optimizeAfterWarmUp();
    return false;
}
#endif

#if ENABLE(DFG_JIT)
void CodeBlock::tallyFrequentExitSites()
{
    ASSERT(getJITType() == JITCode::DFGJIT);
    ASSERT(alternative()->getJITType() == JITCode::BaselineJIT);
    ASSERT(!!m_dfgData);
    
    CodeBlock* profiledBlock = alternative();
    
    for (unsigned i = 0; i < m_dfgData->osrExit.size(); ++i) {
        DFG::OSRExit& exit = m_dfgData->osrExit[i];
        
        if (!exit.considerAddingAsFrequentExitSite(profiledBlock))
            continue;
        
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("OSR exit #", i, " (bc#", exit.m_codeOrigin.bytecodeIndex, ", ", exit.m_kind, ") for ", *this, " occurred frequently: counting as frequent exit site.\n");
#endif
    }
}
#endif // ENABLE(DFG_JIT)

#if ENABLE(VERBOSE_VALUE_PROFILE)
void CodeBlock::dumpValueProfiles()
{
    dataLog("ValueProfile for ", *this, ":\n");
    for (unsigned i = 0; i < totalNumberOfValueProfiles(); ++i) {
        ValueProfile* profile = getFromAllValueProfiles(i);
        if (profile->m_bytecodeOffset < 0) {
            ASSERT(profile->m_bytecodeOffset == -1);
            dataLogF("   arg = %u: ", i);
        } else
            dataLogF("   bc = %d: ", profile->m_bytecodeOffset);
        if (!profile->numberOfSamples() && profile->m_prediction == SpecNone) {
            dataLogF("<empty>\n");
            continue;
        }
        profile->dump(WTF::dataFile());
        dataLogF("\n");
    }
    dataLog("RareCaseProfile for ", *this, ":\n");
    for (unsigned i = 0; i < numberOfRareCaseProfiles(); ++i) {
        RareCaseProfile* profile = rareCaseProfile(i);
        dataLogF("   bc = %d: %u\n", profile->m_bytecodeOffset, profile->m_counter);
    }
    dataLog("SpecialFastCaseProfile for ", *this, ":\n");
    for (unsigned i = 0; i < numberOfSpecialFastCaseProfiles(); ++i) {
        RareCaseProfile* profile = specialFastCaseProfile(i);
        dataLogF("   bc = %d: %u\n", profile->m_bytecodeOffset, profile->m_counter);
    }
}
#endif // ENABLE(VERBOSE_VALUE_PROFILE)

size_t CodeBlock::predictedMachineCodeSize()
{
    // This will be called from CodeBlock::CodeBlock before either m_vm or the
    // instructions have been initialized. It's OK to return 0 because what will really
    // matter is the recomputation of this value when the slow path is triggered.
    if (!m_vm)
        return 0;
    
    if (!m_vm->machineCodeBytesPerBytecodeWordForBaselineJIT)
        return 0; // It's as good of a prediction as we'll get.
    
    // Be conservative: return a size that will be an overestimation 84% of the time.
    double multiplier = m_vm->machineCodeBytesPerBytecodeWordForBaselineJIT.mean() +
        m_vm->machineCodeBytesPerBytecodeWordForBaselineJIT.standardDeviation();
    
    // Be paranoid: silently reject bogus multipiers. Silently doing the "wrong" thing
    // here is OK, since this whole method is just a heuristic.
    if (multiplier < 0 || multiplier > 1000)
        return 0;
    
    double doubleResult = multiplier * m_instructions.size();
    
    // Be even more paranoid: silently reject values that won't fit into a size_t. If
    // the function is so huge that we can't even fit it into virtual memory then we
    // should probably have some other guards in place to prevent us from even getting
    // to this point.
    if (doubleResult > std::numeric_limits<size_t>::max())
        return 0;
    
    return static_cast<size_t>(doubleResult);
}

bool CodeBlock::usesOpcode(OpcodeID opcodeID)
{
    Interpreter* interpreter = vm()->interpreter;
    Instruction* instructionsBegin = instructions().begin();
    unsigned instructionCount = instructions().size();
    
    for (unsigned bytecodeOffset = 0; bytecodeOffset < instructionCount; ) {
        switch (interpreter->getOpcodeID(instructionsBegin[bytecodeOffset].u.opcode)) {
#define DEFINE_OP(curOpcode, length)        \
        case curOpcode:                     \
            if (curOpcode == opcodeID)      \
                return true;                \
            bytecodeOffset += length;       \
            break;
            FOR_EACH_OPCODE_ID(DEFINE_OP)
#undef DEFINE_OP
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    return false;
}

String CodeBlock::nameForRegister(int registerNumber)
{
    SymbolTable::iterator end = symbolTable()->end();
    for (SymbolTable::iterator ptr = symbolTable()->begin(); ptr != end; ++ptr) {
        if (ptr->value.getIndex() == registerNumber)
            return String(ptr->key);
    }
    if (needsActivation() && registerNumber == activationRegister())
        return ASCIILiteral("activation");
    if (registerNumber == thisRegister())
        return ASCIILiteral("this");
    if (usesArguments()) {
        if (registerNumber == argumentsRegister())
            return ASCIILiteral("arguments");
        if (unmodifiedArgumentsRegister(argumentsRegister()) == registerNumber)
            return ASCIILiteral("real arguments");
    }
    if (registerNumber < 0) {
        int argumentPosition = -registerNumber;
        argumentPosition -= JSStack::CallFrameHeaderSize + 1;
        return String::format("arguments[%3d]", argumentPosition - 1).impl();
    }
    return "";
}

} // namespace JSC
