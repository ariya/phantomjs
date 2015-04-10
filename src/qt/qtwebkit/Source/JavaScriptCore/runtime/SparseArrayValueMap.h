/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef SparseArrayValueMap_h
#define SparseArrayValueMap_h

#include "JSCell.h"
#include "JSTypeInfo.h"
#include "PropertyDescriptor.h"
#include "PutDirectIndexMode.h"
#include "WriteBarrier.h"
#include <wtf/HashMap.h>
#include <wtf/Platform.h>

namespace JSC {

class SparseArrayValueMap;

struct SparseArrayEntry : public WriteBarrier<Unknown> {
    typedef WriteBarrier<Unknown> Base;

    SparseArrayEntry() : attributes(0) { }

    JSValue get(ExecState*, JSObject*) const;
    void get(PropertySlot&) const;
    void get(PropertyDescriptor&) const;
    void put(ExecState*, JSValue thisValue, SparseArrayValueMap*, JSValue, bool shouldThrow);
    JSValue getNonSparseMode() const;

    unsigned attributes;
};

class SparseArrayValueMap : public JSCell {
public:
    typedef JSCell Base;
    
private:
    typedef HashMap<uint64_t, SparseArrayEntry, WTF::IntHash<uint64_t>, WTF::UnsignedWithZeroKeyHashTraits<uint64_t> > Map;

    enum Flags {
        Normal = 0,
        SparseMode = 1,
        LengthIsReadOnly = 2,
    };

    SparseArrayValueMap(VM&);
    ~SparseArrayValueMap();
    
    void finishCreation(VM&);

    static const unsigned StructureFlags = OverridesVisitChildren | JSCell::StructureFlags;

public:
    static JS_EXPORTDATA const ClassInfo s_info;
    
    typedef Map::iterator iterator;
    typedef Map::const_iterator const_iterator;
    typedef Map::AddResult AddResult;

    static SparseArrayValueMap* create(VM&);
    
    static const bool needsDestruction = true;
    static const bool hasImmortalStructure = true;
    static void destroy(JSCell*);
    
    static Structure* createStructure(VM&, JSGlobalObject*, JSValue prototype);

    static void visitChildren(JSCell*, SlotVisitor&);

    bool sparseMode()
    {
        return m_flags & SparseMode;
    }

    void setSparseMode()
    {
        m_flags = static_cast<Flags>(m_flags | SparseMode);
    }

    bool lengthIsReadOnly()
    {
        return m_flags & LengthIsReadOnly;
    }

    void setLengthIsReadOnly()
    {
        m_flags = static_cast<Flags>(m_flags | LengthIsReadOnly);
    }

    // These methods may mutate the contents of the map
    void putEntry(ExecState*, JSObject*, unsigned, JSValue, bool shouldThrow);
    bool putDirect(ExecState*, JSObject*, unsigned, JSValue, unsigned attributes, PutDirectIndexMode);
    AddResult add(JSObject*, unsigned);
    iterator find(unsigned i) { return m_map.find(i); }
    // This should ASSERT the remove is valid (check the result of the find).
    void remove(iterator it) { m_map.remove(it); }
    void remove(unsigned i) { m_map.remove(i); }

    // These methods do not mutate the contents of the map.
    iterator notFound() { return m_map.end(); }
    bool isEmpty() const { return m_map.isEmpty(); }
    bool contains(unsigned i) const { return m_map.contains(i); }
    size_t size() const { return m_map.size(); }
    // Only allow const begin/end iteration.
    const_iterator begin() const { return m_map.begin(); }
    const_iterator end() const { return m_map.end(); }

private:
    Map m_map;
    Flags m_flags;
    size_t m_reportedCapacity;
};

} // namespace JSC

#endif // SparseArrayValueMap_h

