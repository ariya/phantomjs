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

#ifndef DFGAdjacencyList_h
#define DFGAdjacencyList_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGCommon.h"
#include "DFGEdge.h"

namespace JSC { namespace DFG {

class AdjacencyList {
public:
    enum Kind {
        Fixed,
        Variable
    };
    
    enum { Size = 3 };
    
    AdjacencyList() { }
    
    AdjacencyList(Kind kind)
    {
        if (kind == Variable) {
            m_words[0].m_encodedWord = UINT_MAX;
            m_words[1].m_encodedWord = UINT_MAX;
        }
    }
    
    AdjacencyList(Kind kind, Edge child1, Edge child2, Edge child3)
    {
        ASSERT_UNUSED(kind, kind == Fixed);
        initialize(child1, child2, child3);
    }
    
    AdjacencyList(Kind kind, unsigned firstChild, unsigned numChildren)
    {
        ASSERT_UNUSED(kind, kind == Variable);
        setFirstChild(firstChild);
        setNumChildren(numChildren);
    }
    
    const Edge& child(unsigned i) const
    {
        ASSERT(i < Size);
        return m_words[i];
    }    
    
    Edge& child(unsigned i)
    {
        ASSERT(i < Size);
        return m_words[i];
    }
    
    void setChild(unsigned i, Edge nodeUse)
    {
        ASSERT(i < Size);
        m_words[i] = nodeUse;
    }
    
    Edge child1() const { return child(0); }
    Edge child2() const { return child(1); }
    Edge child3() const { return child(2); }

    Edge& child1() { return child(0); }
    Edge& child2() { return child(1); }
    Edge& child3() { return child(2); }
    
    void setChild1(Edge nodeUse) { setChild(0, nodeUse); }
    void setChild2(Edge nodeUse) { setChild(1, nodeUse); }
    void setChild3(Edge nodeUse) { setChild(2, nodeUse); }
    
    Edge child1Unchecked() const { return m_words[0]; }
    
    void initialize(Edge child1, Edge child2, Edge child3)
    {
        child(0) = child1;
        child(1) = child2;
        child(2) = child3;
    }
    
    void initialize(Node* child1 = 0, Node* child2 = 0, Node* child3 = 0)
    {
        initialize(Edge(child1), Edge(child2), Edge(child3));
    }
    
    void reset()
    {
        initialize();
    }
    
    // Call this if you wish to remove an edge and the node treats the list of children.
    void removeEdge(unsigned edgeIndex)
    {
        for (unsigned i = edgeIndex; i < Size - 1; ++i)
            setChild(i, child(i + 1));
        setChild(Size - 1, Edge());
    }

    unsigned firstChild() const
    {
        return m_words[0].m_encodedWord;
    }
    void setFirstChild(unsigned firstChild)
    {
        m_words[0].m_encodedWord = firstChild;
    }
    
    unsigned numChildren() const
    {
        return m_words[1].m_encodedWord;
    }
    void setNumChildren(unsigned numChildren)
    {
        m_words[1].m_encodedWord = numChildren;
    }
    
private:
    Edge m_words[Size];
};

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGAdjacencyList_h
