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

#ifndef ProfilerBytecode_h
#define ProfilerBytecode_h

#include "JSCJSValue.h"
#include "Opcode.h"
#include <wtf/text/CString.h>

namespace JSC { namespace Profiler {

class Bytecode {
public:
    Bytecode()
        : m_bytecodeIndex(std::numeric_limits<unsigned>::max())
    {
    }
    
    Bytecode(unsigned bytecodeIndex, OpcodeID opcodeID, const CString& description)
        : m_bytecodeIndex(bytecodeIndex)
        , m_opcodeID(opcodeID)
        , m_description(description)
    {
    }
    
    unsigned bytecodeIndex() const { return m_bytecodeIndex; }
    OpcodeID opcodeID() const { return m_opcodeID; }
    const CString& description() const { return m_description; }
    
    JSValue toJS(ExecState*) const;
private:
    unsigned m_bytecodeIndex;
    OpcodeID m_opcodeID;
    CString m_description;
};

inline unsigned getBytecodeIndexForBytecode(Bytecode* bytecode) { return bytecode->bytecodeIndex(); }

} } // namespace JSC::Profiler

#endif // ProfilerBytecode_h

