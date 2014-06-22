/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef DFGInsertionSet_h
#define DFGInsertionSet_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGGraph.h"
#include <wtf/Vector.h>

namespace JSC { namespace DFG {

class Insertion {
public:
    Insertion() { }
    
    Insertion(size_t index, Node* element)
        : m_index(index)
        , m_element(element)
    {
    }
    
    size_t index() const { return m_index; }
    Node* element() const { return m_element; }
private:
    size_t m_index;
    Node* m_element;
};

class InsertionSet {
public:
    InsertionSet(Graph& graph)
        : m_graph(graph)
    {
    }
    
    Node* insert(const Insertion& insertion)
    {
        ASSERT(!m_insertions.size() || m_insertions.last().index() <= insertion.index());
        m_insertions.append(insertion);
        return insertion.element();
    }
    
    Node* insert(size_t index, Node* element)
    {
        return insert(Insertion(index, element));
    }

#define DFG_DEFINE_INSERT_NODE(templatePre, templatePost, typeParams, valueParamsComma, valueParams, valueArgs) \
    templatePre typeParams templatePost Node* insertNode(size_t index, SpeculatedType type valueParamsComma valueParams) \
    { \
        return insert(index, m_graph.addNode(type valueParamsComma valueArgs)); \
    }
    DFG_VARIADIC_TEMPLATE_FUNCTION(DFG_DEFINE_INSERT_NODE)
#undef DFG_DEFINE_INSERT_NODE
    
    void execute(BasicBlock* block)
    {
        if (!m_insertions.size())
            return;
        block->grow(block->size() + m_insertions.size());
        size_t lastIndex = block->size();
        for (size_t indexInInsertions = m_insertions.size(); indexInInsertions--;) {
            Insertion& insertion = m_insertions[indexInInsertions];
            size_t firstIndex = insertion.index() + indexInInsertions;
            size_t indexOffset = indexInInsertions + 1;
            for (size_t i = lastIndex; --i > firstIndex;)
                block->at(i) = block->at(i - indexOffset);
            block->at(firstIndex) = insertion.element();
            lastIndex = firstIndex;
        }
        m_insertions.resize(0);
    }
private:
    Graph& m_graph;
    Vector<Insertion, 8> m_insertions;
};

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGInsertionSet_h

