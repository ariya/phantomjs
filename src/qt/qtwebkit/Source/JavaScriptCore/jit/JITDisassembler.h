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

#ifndef JITDisassembler_h
#define JITDisassembler_h

#include <wtf/Platform.h>

#if ENABLE(JIT)

#include "LinkBuffer.h"
#include "MacroAssembler.h"
#include "ProfilerDatabase.h"
#include <wtf/Vector.h>

namespace JSC {

class CodeBlock;

class JITDisassembler {
    WTF_MAKE_FAST_ALLOCATED;
public:
    JITDisassembler(CodeBlock*);
    ~JITDisassembler();
    
    void setStartOfCode(MacroAssembler::Label label) { m_startOfCode = label; }
    void setForBytecodeMainPath(unsigned bytecodeIndex, MacroAssembler::Label label)
    {
        m_labelForBytecodeIndexInMainPath[bytecodeIndex] = label;
    }
    void setForBytecodeSlowPath(unsigned bytecodeIndex, MacroAssembler::Label label)
    {
        m_labelForBytecodeIndexInSlowPath[bytecodeIndex] = label;
    }
    void setEndOfSlowPath(MacroAssembler::Label label) { m_endOfSlowPath = label; }
    void setEndOfCode(MacroAssembler::Label label) { m_endOfCode = label; }
    
    void dump(LinkBuffer&);
    void dump(PrintStream&, LinkBuffer&);
    void reportToProfiler(Profiler::Compilation*, LinkBuffer&);

private:
    void dumpHeader(PrintStream&, LinkBuffer&);
    MacroAssembler::Label firstSlowLabel();
    
    struct DumpedOp {
        unsigned index;
        CString disassembly;
    };
    Vector<DumpedOp> dumpVectorForInstructions(LinkBuffer&, const char* prefix, Vector<MacroAssembler::Label>& labels, MacroAssembler::Label endLabel);
        
    void dumpForInstructions(PrintStream&, LinkBuffer&, const char* prefix, Vector<MacroAssembler::Label>& labels, MacroAssembler::Label endLabel);
    void reportInstructions(Profiler::Compilation*, LinkBuffer&, const char* prefix, Vector<MacroAssembler::Label>& labels, MacroAssembler::Label endLabel);
    
    void dumpDisassembly(PrintStream&, LinkBuffer&, MacroAssembler::Label from, MacroAssembler::Label to);
    
    CodeBlock* m_codeBlock;
    MacroAssembler::Label m_startOfCode;
    Vector<MacroAssembler::Label> m_labelForBytecodeIndexInMainPath;
    Vector<MacroAssembler::Label> m_labelForBytecodeIndexInSlowPath;
    MacroAssembler::Label m_endOfSlowPath;
    MacroAssembler::Label m_endOfCode;
};

} // namespace JSC

#endif // ENABLE(JIT)

#endif // JITDisassembler_h

