/*
 * Copyright (C) 2011, 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef DFGStructureAbstractValue_h
#define DFGStructureAbstractValue_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "JSCell.h"
#include "SpeculatedType.h"
#include "StructureSet.h"

namespace JSC { namespace DFG {

class StructureAbstractValue {
public:
    StructureAbstractValue()
        : m_structure(0)
    {
    }
    
    StructureAbstractValue(Structure* structure)
        : m_structure(structure)
    {
    }
    
    StructureAbstractValue(const StructureSet& set)
    {
        switch (set.size()) {
        case 0:
            m_structure = 0;
            break;
            
        case 1:
            m_structure = set[0];
            break;
            
        default:
            m_structure = topValue();
            break;
        }
    }
    
    void clear()
    {
        m_structure = 0;
    }
    
    void makeTop()
    {
        m_structure = topValue();
    }
    
    static StructureAbstractValue top()
    {
        StructureAbstractValue value;
        value.makeTop();
        return value;
    }
    
    void add(Structure* structure)
    {
        ASSERT(!contains(structure) && !isTop());
        if (m_structure)
            makeTop();
        else
            m_structure = structure;
    }
    
    bool addAll(const StructureSet& other)
    {
        if (isTop() || !other.size())
            return false;
        if (other.size() > 1) {
            makeTop();
            return true;
        }
        if (!m_structure) {
            m_structure = other[0];
            return true;
        }
        if (m_structure == other[0])
            return false;
        makeTop();
        return true;
    }
    
    bool addAll(const StructureAbstractValue& other)
    {
        if (!other.m_structure)
            return false;
        if (isTop())
            return false;
        if (other.isTop()) {
            makeTop();
            return true;
        }
        if (m_structure) {
            if (m_structure == other.m_structure)
                return false;
            makeTop();
            return true;
        }
        m_structure = other.m_structure;
        return true;
    }
    
    bool contains(Structure* structure) const
    {
        if (isTop())
            return true;
        if (m_structure == structure)
            return true;
        return false;
    }
    
    bool isSubsetOf(const StructureSet& other) const
    {
        if (isTop())
            return false;
        if (!m_structure)
            return true;
        return other.contains(m_structure);
    }
    
    bool doesNotContainAnyOtherThan(Structure* structure) const
    {
        if (isTop())
            return false;
        if (!m_structure)
            return true;
        return m_structure == structure;
    }
    
    bool isSupersetOf(const StructureSet& other) const
    {
        if (isTop())
            return true;
        if (!other.size())
            return true;
        if (other.size() > 1)
            return false;
        return m_structure == other[0];
    }
    
    bool isSubsetOf(const StructureAbstractValue& other) const
    {
        if (other.isTop())
            return true;
        if (isTop())
            return false;
        if (m_structure) {
            if (other.m_structure)
                return m_structure == other.m_structure;
            return false;
        }
        return true;
    }
    
    bool isSupersetOf(const StructureAbstractValue& other) const
    {
        return other.isSubsetOf(*this);
    }
    
    void filter(const StructureSet& other)
    {
        if (!m_structure)
            return;
        
        if (isTop()) {
            switch (other.size()) {
            case 0:
                m_structure = 0;
                return;
                
            case 1:
                m_structure = other[0];
                return;
                
            default:
                return;
            }
        }
        
        if (other.contains(m_structure))
            return;
        
        m_structure = 0;
    }
    
    void filter(const StructureAbstractValue& other)
    {
        if (isTop()) {
            m_structure = other.m_structure;
            return;
        }
        if (m_structure == other.m_structure)
            return;
        if (other.isTop())
            return;
        m_structure = 0;
    }
    
    void filter(SpeculatedType other)
    {
        if (!(other & SpecCell)) {
            clear();
            return;
        }
        
        if (isClearOrTop())
            return;

        if (!(speculationFromStructure(m_structure) & other))
            m_structure = 0;
    }
    
    bool isClear() const
    {
        return !m_structure;
    }
    
    bool isTop() const { return m_structure == topValue(); }
    
    bool isClearOrTop() const { return m_structure <= topValue(); }
    bool isNeitherClearNorTop() const { return !isClearOrTop(); }
    
    size_t size() const
    {
        ASSERT(!isTop());
        return !!m_structure;
    }
    
    Structure* at(size_t i) const
    {
        ASSERT(!isTop());
        ASSERT(m_structure);
        ASSERT_UNUSED(i, !i);
        return m_structure;
    }
    
    Structure* operator[](size_t i) const
    {
        return at(i);
    }
    
    Structure* last() const
    {
        return at(0);
    }
    
    SpeculatedType speculationFromStructures() const
    {
        if (isTop())
            return SpecCell;
        if (isClear())
            return SpecNone;
        return speculationFromStructure(m_structure);
    }
    
    bool hasSingleton() const
    {
        return isNeitherClearNorTop();
    }
    
    Structure* singleton() const
    {
        ASSERT(isNeitherClearNorTop());
        return m_structure;
    }
    
    bool operator==(const StructureAbstractValue& other) const
    {
        return m_structure == other.m_structure;
    }
    
    void dump(PrintStream& out) const
    {
        if (isTop()) {
            out.print("TOP");
            return;
        }
        
        out.print("[");
        if (m_structure)
            out.print(RawPointer(m_structure), "(", m_structure->classInfo()->className, ")");
        out.print("]");
    }

private:
    static Structure* topValue() { return reinterpret_cast<Structure*>(1); }
    
    // NB. This must have a trivial destructor.
    
    // This can only remember one structure at a time.
    Structure* m_structure;
};

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGStructureAbstractValue_h


