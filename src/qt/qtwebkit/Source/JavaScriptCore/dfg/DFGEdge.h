/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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

#ifndef DFGEdge_h
#define DFGEdge_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGCommon.h"
#include "DFGUseKind.h"

namespace JSC { namespace DFG {

class AdjacencyList;

class Edge {
public:
    explicit Edge(Node* node = 0, UseKind useKind = UntypedUse, ProofStatus proofStatus = NeedsCheck)
#if USE(JSVALUE64)
        : m_encodedWord(makeWord(node, useKind, proofStatus))
#else
        : m_node(node)
        , m_encodedWord(makeWord(useKind, proofStatus))
#endif
    {
    }
    
#if USE(JSVALUE64)
    Node* node() const { return bitwise_cast<Node*>(m_encodedWord >> shift()); }
#else
    Node* node() const { return m_node; }
#endif

    Node& operator*() const { return *node(); }
    Node* operator->() const { return node(); }
    
    void setNode(Node* node)
    {
#if USE(JSVALUE64)
        m_encodedWord = makeWord(node, useKind(), proofStatus());
#else
        m_node = node;
#endif
    }
    
    UseKind useKindUnchecked() const
    {
#if USE(JSVALUE64)
        unsigned masked = m_encodedWord & (((1 << shift()) - 1));
        unsigned shifted = masked >> 1;
#else
        unsigned shifted = static_cast<UseKind>(m_encodedWord) >> 1;
#endif
        ASSERT(shifted < static_cast<unsigned>(LastUseKind));
        UseKind result = static_cast<UseKind>(shifted);
        ASSERT(node() || result == UntypedUse);
        return result;
    }
    UseKind useKind() const
    {
        ASSERT(node());
        return useKindUnchecked();
    }
    void setUseKind(UseKind useKind)
    {
        ASSERT(node());
#if USE(JSVALUE64)
        m_encodedWord = makeWord(node(), useKind, proofStatus());
#else
        m_encodedWord = makeWord(useKind, proofStatus());
#endif
    }
    
    ProofStatus proofStatusUnchecked() const
    {
        return proofStatusForIsProved(m_encodedWord & 1);
    }
    ProofStatus proofStatus() const
    {
        ASSERT(node());
        return proofStatusUnchecked();
    }
    void setProofStatus(ProofStatus proofStatus)
    {
        ASSERT(node());
#if USE(JSVALUE64)
        m_encodedWord = makeWord(node(), useKind(), proofStatus);
#else
        m_encodedWord = makeWord(useKind(), proofStatus);
#endif
    }
    bool isProved() const
    {
        return proofStatus() == IsProved;
    }
    bool needsCheck() const
    {
        return proofStatus() == NeedsCheck;
    }
    
    bool isSet() const { return !!node(); }
    
    typedef void* Edge::*UnspecifiedBoolType;
    operator UnspecifiedBoolType*() const { return reinterpret_cast<UnspecifiedBoolType*>(isSet()); }
    
    bool operator!() const { return !isSet(); }
    
    bool operator==(Edge other) const
    {
#if USE(JSVALUE64)
        return m_encodedWord == other.m_encodedWord;
#else
        return m_node == other.m_node && m_encodedWord == other.m_encodedWord;
#endif
    }
    bool operator!=(Edge other) const
    {
        return !(*this == other);
    }
    
    void dump(PrintStream&) const;

private:
    friend class AdjacencyList;
    
#if USE(JSVALUE64)
    static uint32_t shift() { return 6; }
    
    static uintptr_t makeWord(Node* node, UseKind useKind, ProofStatus proofStatus)
    {
        ASSERT(sizeof(node) == 8);
        uintptr_t shiftedValue = bitwise_cast<uintptr_t>(node) << shift();
        ASSERT((shiftedValue >> shift()) == bitwise_cast<uintptr_t>(node));
        ASSERT(useKind >= 0 && useKind < LastUseKind);
        ASSERT((static_cast<uintptr_t>(LastUseKind) << 1) <= (static_cast<uintptr_t>(1) << shift()));
        return shiftedValue | (static_cast<uintptr_t>(useKind) << 1) | DFG::isProved(proofStatus);
    }
    
#else
    static uintptr_t makeWord(UseKind useKind, ProofStatus proofStatus)
    {
        return (static_cast<uintptr_t>(useKind) << 1) | DFG::isProved(proofStatus);
    }
    
    Node* m_node;
#endif
    // On 64-bit this holds both the pointer and the use kind, while on 32-bit
    // this just holds the use kind. In both cases this may be hijacked by
    // AdjacencyList for storing firstChild and numChildren.
    uintptr_t m_encodedWord;
};

inline bool operator==(Edge edge, Node* node)
{
    return edge.node() == node;
}
inline bool operator==(Node* node, Edge edge)
{
    return edge.node() == node;
}
inline bool operator!=(Edge edge, Node* node)
{
    return edge.node() != node;
}
inline bool operator!=(Node* node, Edge edge)
{
    return edge.node() != node;
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGEdge_h

