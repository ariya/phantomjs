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

#ifndef ProfilerOriginStack_h
#define ProfilerOriginStack_h

#include "JSCJSValue.h"
#include "ProfilerOrigin.h"
#include <wtf/HashMap.h>
#include <wtf/PrintStream.h>
#include <wtf/Vector.h>

namespace JSC {

class CodeBlock;
struct CodeOrigin;

namespace Profiler {

class Database;

class OriginStack {
public:
    OriginStack() { }
    
    OriginStack(WTF::HashTableDeletedValueType);
    
    explicit OriginStack(const Origin&);
    
    explicit OriginStack(Database&, CodeBlock*, const CodeOrigin&);
    
    ~OriginStack();
    
    void append(const Origin&);
    
    bool operator!() const { return m_stack.isEmpty(); }
    
    unsigned size() const { return m_stack.size(); }
    const Origin& fromBottom(unsigned i) const { return m_stack[i]; }
    const Origin& fromTop(unsigned i) const { return m_stack[m_stack.size() - i - 1]; }
    
    bool operator==(const OriginStack&) const;
    unsigned hash() const;
    
    bool isHashTableDeletedValue() const;
    
    void dump(PrintStream&) const;
    JSValue toJS(ExecState*) const;
    
private:
    Vector<Origin, 1> m_stack;
};

inline bool OriginStack::isHashTableDeletedValue() const
{
    return m_stack.size() == 1 && m_stack[0].isHashTableDeletedValue();
}

struct OriginStackHash {
    static unsigned hash(const OriginStack& key) { return key.hash(); }
    static bool equal(const OriginStack& a, const OriginStack& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = true;
};

} } // namespace JSC::Profiler

namespace WTF {

template<typename T> struct DefaultHash;
template<> struct DefaultHash<JSC::Profiler::OriginStack> {
    typedef JSC::Profiler::OriginStackHash Hash;
};

template<typename T> struct HashTraits;
template<> struct HashTraits<JSC::Profiler::OriginStack> : SimpleClassHashTraits<JSC::Profiler::OriginStack> { };

} // namespace WTF

#endif // ProfilerOriginStack_h

