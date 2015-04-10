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

#ifndef ProfilerBytecodes_h
#define ProfilerBytecodes_h

#include "CodeBlockHash.h"
#include "JSCJSValue.h"
#include "ProfilerBytecodeSequence.h"
#include <wtf/PrintStream.h>
#include <wtf/text/WTFString.h>

namespace JSC { namespace Profiler {

class Bytecodes : public BytecodeSequence {
public:
    Bytecodes(size_t id, CodeBlock*);
    ~Bytecodes();
    
    size_t id() const { return m_id; }
    const String& inferredName() const { return m_inferredName; }
    const String& sourceCode() const { return m_sourceCode; }
    unsigned instructionCount() const { return m_instructionCount; }
    CodeBlockHash hash() const { return m_hash; }

    void dump(PrintStream&) const;
    
    JSValue toJS(ExecState*) const;
    
private:
    size_t m_id;
    String m_inferredName;
    String m_sourceCode;
    CodeBlockHash m_hash;
    unsigned m_instructionCount;
};

} } // namespace JSC::Profiler

#endif // ProfilerBytecodes_h

