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
#include "ProfilerOriginStack.h"

#include "CodeOrigin.h"
#include "JSGlobalObject.h"
#include "Operations.h"
#include "ProfilerDatabase.h"

namespace JSC { namespace Profiler {

OriginStack::OriginStack(WTF::HashTableDeletedValueType)
{
    m_stack.append(Origin(WTF::HashTableDeletedValue));
}

OriginStack::OriginStack(const Origin& origin)
{
    m_stack.append(origin);
}

OriginStack::OriginStack(Database& database, CodeBlock* codeBlock, const CodeOrigin& codeOrigin)
{
    Vector<CodeOrigin> stack = codeOrigin.inlineStack();
    
    append(Origin(database, codeBlock, stack[0].bytecodeIndex));
    
    for (unsigned i = 1; i < stack.size(); ++i) {
        append(Origin(
            database.ensureBytecodesFor(stack[i].inlineCallFrame->baselineCodeBlock()),
            stack[i].bytecodeIndex));
    }
}

OriginStack::~OriginStack() { }

void OriginStack::append(const Origin& origin)
{
    m_stack.append(origin);
}

bool OriginStack::operator==(const OriginStack& other) const
{
    if (m_stack.size() != other.m_stack.size())
        return false;
    
    for (unsigned i = m_stack.size(); i--;) {
        if (m_stack[i] != other.m_stack[i])
            return false;
    }
    
    return true;
}

unsigned OriginStack::hash() const
{
    unsigned result = m_stack.size();
    
    for (unsigned i = m_stack.size(); i--;) {
        result *= 3;
        result += m_stack[i].hash();
    }
    
    return result;
}

void OriginStack::dump(PrintStream& out) const
{
    for (unsigned i = 0; i < m_stack.size(); ++i) {
        if (i)
            out.print(" --> ");
        out.print(m_stack[i]);
    }
}

JSValue OriginStack::toJS(ExecState* exec) const
{
    JSArray* result = constructEmptyArray(exec, 0);
    
    for (unsigned i = 0; i < m_stack.size(); ++i)
        result->putDirectIndex(exec, i, m_stack[i].toJS(exec));
    
    return result;
}

} } // namespace JSC::Profiler

