/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef DFGDisassembler_h
#define DFGDisassembler_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGCommon.h"
#include "LinkBuffer.h"
#include "MacroAssembler.h"
#include <wtf/HashMap.h>
#include <wtf/StringPrintStream.h>
#include <wtf/Vector.h>

namespace JSC { namespace DFG {

class Graph;

class Disassembler {
    WTF_MAKE_FAST_ALLOCATED;
public:
    Disassembler(Graph&);
    
    void setStartOfCode(MacroAssembler::Label label) { m_startOfCode = label; }
    void setForBlock(BlockIndex blockIndex, MacroAssembler::Label label)
    {
        m_labelForBlockIndex[blockIndex] = label;
    }
    void setForNode(Node* node, MacroAssembler::Label label)
    {
        ASSERT(label.isSet());
        m_labelForNode.add(node, label);
    }
    void setEndOfMainPath(MacroAssembler::Label label)
    {
        m_endOfMainPath = label;
    }
    void setEndOfCode(MacroAssembler::Label label)
    {
        m_endOfCode = label;
    }
    
    void dump(PrintStream&, LinkBuffer&);
    void dump(LinkBuffer&);
    void reportToProfiler(Profiler::Compilation*, LinkBuffer&);
    
private:
    void dumpHeader(PrintStream&, LinkBuffer&);
    
    struct DumpedOp {
        DumpedOp(CodeOrigin codeOrigin, CString text)
            : codeOrigin(codeOrigin)
            , text(text)
        {
        }
        
        CodeOrigin codeOrigin;
        CString text;
    };
    void append(Vector<DumpedOp>&, StringPrintStream&, CodeOrigin&);
    Vector<DumpedOp> createDumpList(LinkBuffer&);
    
    void dumpDisassembly(PrintStream&, const char* prefix, LinkBuffer&, MacroAssembler::Label& previousLabel, MacroAssembler::Label currentLabel, Node* context);
    
    Graph& m_graph;
    MacroAssembler::Label m_startOfCode;
    Vector<MacroAssembler::Label> m_labelForBlockIndex;
    HashMap<Node*, MacroAssembler::Label> m_labelForNode;
    MacroAssembler::Label m_endOfMainPath;
    MacroAssembler::Label m_endOfCode;
};

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGDisassembler_h
