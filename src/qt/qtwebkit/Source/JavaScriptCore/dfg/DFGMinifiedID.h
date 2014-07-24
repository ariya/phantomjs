/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef DFGMinifiedID_h
#define DFGMinifiedID_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGCommon.h"
#include <wtf/HashMap.h>
#include <wtf/PrintStream.h>

namespace JSC { namespace DFG {

class Graph;
class MinifiedNode;
class ValueSource;

class MinifiedID {
public:
    MinifiedID() : m_id(invalidID()) { }
    MinifiedID(WTF::HashTableDeletedValueType) : m_id(otherInvalidID()) { }
    explicit MinifiedID(Node* node) : m_id(bitwise_cast<uintptr_t>(node)) { }
    
    bool operator!() const { return m_id == invalidID(); }
    
    // This takes Graph& to remind you, that you should only be calling this method
    // when you're in the main compilation pass (i.e. you have a graph) and not later,
    // like during OSR exit compilation.
    Node* node(const Graph&) const { return bitwise_cast<Node*>(m_id); }
    
    bool operator==(const MinifiedID& other) const { return m_id == other.m_id; }
    bool operator!=(const MinifiedID& other) const { return m_id != other.m_id; }
    bool operator<(const MinifiedID& other) const { return m_id < other.m_id; }
    bool operator>(const MinifiedID& other) const { return m_id > other.m_id; }
    bool operator<=(const MinifiedID& other) const { return m_id <= other.m_id; }
    bool operator>=(const MinifiedID& other) const { return m_id >= other.m_id; }
    
    unsigned hash() const { return WTF::IntHash<uintptr_t>::hash(m_id); }
    
    void dump(PrintStream& out) const { out.print(RawPointer(reinterpret_cast<void*>(m_id))); }
    
    bool isHashTableDeletedValue() const { return m_id == otherInvalidID(); }
    
private:
    friend class MinifiedNode;
    friend class ValueSource;
    
    static uintptr_t invalidID() { return static_cast<uintptr_t>(static_cast<intptr_t>(-1)); }
    static uintptr_t otherInvalidID() { return static_cast<uintptr_t>(static_cast<intptr_t>(-2)); }
    
    static MinifiedID fromBits(uintptr_t value)
    {
        MinifiedID result;
        result.m_id = value;
        return result;
    }

    uintptr_t m_id;
};

struct MinifiedIDHash {
    static unsigned hash(const MinifiedID& key) { return key.hash(); }
    static bool equal(const MinifiedID& a, const MinifiedID& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = true;
};

} } // namespace JSC::DFG

namespace WTF {

template<typename T> struct DefaultHash;
template<> struct DefaultHash<JSC::DFG::MinifiedID> {
    typedef JSC::DFG::MinifiedIDHash Hash;
};

template<typename T> struct HashTraits;
template<> struct HashTraits<JSC::DFG::MinifiedID> : SimpleClassHashTraits<JSC::DFG::MinifiedID> { };

} // namespace WTF

#endif // ENABLE(DFG_JIT)

#endif // DFGMinifiedID_h

