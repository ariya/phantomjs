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

#ifndef ProfilerOrigin_h
#define ProfilerOrigin_h

#include "CodeBlockHash.h"
#include "JSCJSValue.h"
#include <wtf/HashMap.h>
#include <wtf/PrintStream.h>

namespace JSC {

class CodeBlock;

namespace Profiler {

class Bytecodes;
class Database;

class Origin {
public:
    Origin()
        : m_bytecodeIndex(std::numeric_limits<unsigned>::max())
    {
    }
    
    Origin(WTF::HashTableDeletedValueType)
        : m_bytecodeIndex(std::numeric_limits<unsigned>::max() - 1)
    {
    }
    
    Origin(Bytecodes* bytecodes, unsigned bytecodeIndex)
        : m_bytecodes(bytecodes)
        , m_bytecodeIndex(bytecodeIndex)
    {
        ASSERT(m_bytecodeIndex < std::numeric_limits<unsigned>::max() - 1);
    }
    
    Origin(Database&, CodeBlock*, unsigned bytecodeIndex);
    
    bool operator!() const { return m_bytecodeIndex == std::numeric_limits<unsigned>::max(); }
    
    Bytecodes* bytecodes() const { return m_bytecodes; }
    unsigned bytecodeIndex() const { return m_bytecodeIndex; }
    
    bool operator==(const Origin&) const;
    bool operator!=(const Origin& other) const { return !(*this == other); }
    unsigned hash() const;
    
    bool isHashTableDeletedValue() const;
    
    void dump(PrintStream&) const;
    JSValue toJS(ExecState*) const;

private:
    Bytecodes* m_bytecodes;
    unsigned m_bytecodeIndex;
};

inline bool Origin::operator==(const Origin& other) const
{
    return m_bytecodes == other.m_bytecodes
        && m_bytecodeIndex == other.m_bytecodeIndex;
}

inline unsigned Origin::hash() const
{
    return WTF::PtrHash<Bytecodes*>::hash(m_bytecodes) + m_bytecodeIndex;
}

inline bool Origin::isHashTableDeletedValue() const
{
    return m_bytecodeIndex == std::numeric_limits<unsigned>::max();
}

struct OriginHash {
    static unsigned hash(const Origin& key) { return key.hash(); }
    static bool equal(const Origin& a, const Origin& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = true;
};

} } // namespace JSC::Profiler

namespace WTF {

template<typename T> struct DefaultHash;
template<> struct DefaultHash<JSC::Profiler::Origin> {
    typedef JSC::Profiler::OriginHash Hash;
};

template<typename T> struct HashTraits;
template<> struct HashTraits<JSC::Profiler::Origin> : SimpleClassHashTraits<JSC::Profiler::Origin> { };

} // namespace WTF

#endif // ProfilerOrigin_h

