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
#include "ProfilerBytecodes.h"

#include "CodeBlock.h"
#include "JSGlobalObject.h"
#include "ObjectConstructor.h"
#include "Operations.h"
#include <wtf/StringPrintStream.h>

namespace JSC { namespace Profiler {

Bytecodes::Bytecodes(size_t id, CodeBlock* codeBlock)
    : BytecodeSequence(codeBlock)
    , m_id(id)
    , m_inferredName(codeBlock->inferredName())
    , m_sourceCode(codeBlock->sourceCodeForTools())
    , m_hash(codeBlock->hash())
    , m_instructionCount(codeBlock->instructionCount())
{
}

Bytecodes::~Bytecodes() { }

void Bytecodes::dump(PrintStream& out) const
{
    out.print("#", m_hash, "(", m_id, ")");
}

JSValue Bytecodes::toJS(ExecState* exec) const
{
    JSObject* result = constructEmptyObject(exec);
    
    result->putDirect(exec->vm(), exec->propertyNames().bytecodesID, jsNumber(m_id));
    result->putDirect(exec->vm(), exec->propertyNames().inferredName, jsString(exec, m_inferredName));
    result->putDirect(exec->vm(), exec->propertyNames().sourceCode, jsString(exec, m_sourceCode));
    result->putDirect(exec->vm(), exec->propertyNames().hash, jsString(exec, String::fromUTF8(toCString(m_hash))));
    result->putDirect(exec->vm(), exec->propertyNames().instructionCount, jsNumber(m_instructionCount));
    addSequenceProperties(exec, result);
    
    return result;
}

} } // namespace JSC::Profiler

