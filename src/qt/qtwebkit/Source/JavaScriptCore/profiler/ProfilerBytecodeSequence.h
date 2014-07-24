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

#ifndef ProfilerBytecodeSequence_h
#define ProfilerBytecodeSequence_h

#include "JSCJSValue.h"
#include "ProfilerBytecode.h"
#include <wtf/PrintStream.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace JSC {

class CodeBlock;

namespace Profiler {

class BytecodeSequence {
public:
    BytecodeSequence(CodeBlock*);
    ~BytecodeSequence();
    
    // Note that this data structure is not indexed by bytecode index.
    unsigned size() const { return m_sequence.size(); }
    const Bytecode& at(unsigned i) const { return m_sequence[i]; }

    unsigned indexForBytecodeIndex(unsigned bytecodeIndex) const;
    const Bytecode& forBytecodeIndex(unsigned bytecodeIndex) const;

protected:
    void addSequenceProperties(ExecState*, JSObject*) const;
    
private:
    Vector<CString> m_header;
    Vector<Bytecode> m_sequence;
};

} } // namespace JSC::Profiler

#endif // ProfilerBytecodeSequence_h

