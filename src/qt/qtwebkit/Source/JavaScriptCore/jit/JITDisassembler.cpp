/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "JITDisassembler.h"

#if ENABLE(JIT)

#include "CodeBlock.h"
#include "CodeBlockWithJITType.h"
#include "JIT.h"
#include <wtf/StringPrintStream.h>

namespace JSC {

JITDisassembler::JITDisassembler(CodeBlock *codeBlock)
    : m_codeBlock(codeBlock)
    , m_labelForBytecodeIndexInMainPath(codeBlock->instructionCount())
    , m_labelForBytecodeIndexInSlowPath(codeBlock->instructionCount())
{
}

JITDisassembler::~JITDisassembler()
{
}

void JITDisassembler::dump(PrintStream& out, LinkBuffer& linkBuffer)
{
    dumpHeader(out, linkBuffer);
    dumpDisassembly(out, linkBuffer, m_startOfCode, m_labelForBytecodeIndexInMainPath[0]);
    
    dumpForInstructions(out, linkBuffer, "    ", m_labelForBytecodeIndexInMainPath, firstSlowLabel());
    out.print("    (End Of Main Path)\n");
    dumpForInstructions(out, linkBuffer, "    (S) ", m_labelForBytecodeIndexInSlowPath, m_endOfSlowPath);
    out.print("    (End Of Slow Path)\n");

    dumpDisassembly(out, linkBuffer, m_endOfSlowPath, m_endOfCode);
}

void JITDisassembler::dump(LinkBuffer& linkBuffer)
{
    dump(WTF::dataFile(), linkBuffer);
}

void JITDisassembler::reportToProfiler(Profiler::Compilation* compilation, LinkBuffer& linkBuffer)
{
    StringPrintStream out;
    
    dumpHeader(out, linkBuffer);
    compilation->addDescription(Profiler::CompiledBytecode(Profiler::OriginStack(), out.toCString()));
    out.reset();
    dumpDisassembly(out, linkBuffer, m_startOfCode, m_labelForBytecodeIndexInMainPath[0]);
    compilation->addDescription(Profiler::CompiledBytecode(Profiler::OriginStack(), out.toCString()));
    
    reportInstructions(compilation, linkBuffer, "    ", m_labelForBytecodeIndexInMainPath, firstSlowLabel());
    compilation->addDescription(Profiler::CompiledBytecode(Profiler::OriginStack(), "    (End Of Main Path)\n"));
    reportInstructions(compilation, linkBuffer, "    (S) ", m_labelForBytecodeIndexInSlowPath, m_endOfSlowPath);
    compilation->addDescription(Profiler::CompiledBytecode(Profiler::OriginStack(), "    (End Of Slow Path)\n"));
    out.reset();
    dumpDisassembly(out, linkBuffer, m_endOfSlowPath, m_endOfCode);
    compilation->addDescription(Profiler::CompiledBytecode(Profiler::OriginStack(), out.toCString()));
}

void JITDisassembler::dumpHeader(PrintStream& out, LinkBuffer& linkBuffer)
{
    out.print("Generated Baseline JIT code for ", CodeBlockWithJITType(m_codeBlock, JITCode::BaselineJIT), ", instruction count = ", m_codeBlock->instructionCount(), "\n");
    out.print("   Source: ", m_codeBlock->sourceCodeOnOneLine(), "\n");
    out.print("   Code at [", RawPointer(linkBuffer.debugAddress()), ", ", RawPointer(static_cast<char*>(linkBuffer.debugAddress()) + linkBuffer.debugSize()), "):\n");
}

MacroAssembler::Label JITDisassembler::firstSlowLabel()
{
    MacroAssembler::Label firstSlowLabel;
    for (unsigned i = 0; i < m_labelForBytecodeIndexInSlowPath.size(); ++i) {
        if (m_labelForBytecodeIndexInSlowPath[i].isSet()) {
            firstSlowLabel = m_labelForBytecodeIndexInSlowPath[i];
            break;
        }
    }
    return firstSlowLabel.isSet() ? firstSlowLabel : m_endOfSlowPath;
}

Vector<JITDisassembler::DumpedOp> JITDisassembler::dumpVectorForInstructions(LinkBuffer& linkBuffer, const char* prefix, Vector<MacroAssembler::Label>& labels, MacroAssembler::Label endLabel)
{
    StringPrintStream out;
    Vector<DumpedOp> result;
    
    for (unsigned i = 0; i < labels.size();) {
        if (!labels[i].isSet()) {
            i++;
            continue;
        }
        out.reset();
        result.append(DumpedOp());
        result.last().index = i;
        out.print(prefix);
        m_codeBlock->dumpBytecode(out, i);
        for (unsigned nextIndex = i + 1; ; nextIndex++) {
            if (nextIndex >= labels.size()) {
                dumpDisassembly(out, linkBuffer, labels[i], endLabel);
                result.last().disassembly = out.toCString();
                return result;
            }
            if (labels[nextIndex].isSet()) {
                dumpDisassembly(out, linkBuffer, labels[i], labels[nextIndex]);
                result.last().disassembly = out.toCString();
                i = nextIndex;
                break;
            }
        }
    }
    
    return result;
}

void JITDisassembler::dumpForInstructions(PrintStream& out, LinkBuffer& linkBuffer, const char* prefix, Vector<MacroAssembler::Label>& labels, MacroAssembler::Label endLabel)
{
    Vector<DumpedOp> dumpedOps = dumpVectorForInstructions(linkBuffer, prefix, labels, endLabel);
    
    for (unsigned i = 0; i < dumpedOps.size(); ++i)
        out.print(dumpedOps[i].disassembly);
}

void JITDisassembler::reportInstructions(Profiler::Compilation* compilation, LinkBuffer& linkBuffer, const char* prefix, Vector<MacroAssembler::Label>& labels, MacroAssembler::Label endLabel)
{
    Vector<DumpedOp> dumpedOps = dumpVectorForInstructions(linkBuffer, prefix, labels, endLabel);
    
    for (unsigned i = 0; i < dumpedOps.size(); ++i) {
        compilation->addDescription(
            Profiler::CompiledBytecode(
                Profiler::OriginStack(Profiler::Origin(compilation->bytecodes(), dumpedOps[i].index)),
                dumpedOps[i].disassembly));
    }
}

void JITDisassembler::dumpDisassembly(PrintStream& out, LinkBuffer& linkBuffer, MacroAssembler::Label from, MacroAssembler::Label to)
{
    CodeLocationLabel fromLocation = linkBuffer.locationOf(from);
    CodeLocationLabel toLocation = linkBuffer.locationOf(to);
    disassemble(fromLocation, bitwise_cast<uintptr_t>(toLocation.executableAddress()) - bitwise_cast<uintptr_t>(fromLocation.executableAddress()), "        ", out);
}

} // namespace JSC

#endif // ENABLE(JIT)

