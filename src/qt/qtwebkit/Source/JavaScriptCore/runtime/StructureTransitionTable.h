/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef StructureTransitionTable_h
#define StructureTransitionTable_h

#include "IndexingType.h"
#include "WeakGCMap.h"
#include <wtf/HashFunctions.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/StringImpl.h>

namespace JSC {

class JSCell;
class Structure;

static const unsigned FirstInternalAttribute = 1 << 6; // Use for transitions that don't have to do with property additions.

// Support for attributes used to indicate transitions not related to properties.
// If any of these are used, the string portion of the key should be 0.
enum NonPropertyTransition {
    AllocateUndecided,
    AllocateInt32,
    AllocateDouble,
    AllocateContiguous,
    AllocateArrayStorage,
    AllocateSlowPutArrayStorage,
    SwitchToSlowPutArrayStorage,
    AddIndexedAccessors
};

inline unsigned toAttributes(NonPropertyTransition transition)
{
    return transition + FirstInternalAttribute;
}

inline IndexingType newIndexingType(IndexingType oldType, NonPropertyTransition transition)
{
    switch (transition) {
    case AllocateUndecided:
        ASSERT(!hasIndexedProperties(oldType));
        return oldType | UndecidedShape;
    case AllocateInt32:
        ASSERT(!hasIndexedProperties(oldType) || hasUndecided(oldType));
        return (oldType & ~IndexingShapeMask) | Int32Shape;
    case AllocateDouble:
        ASSERT(!hasIndexedProperties(oldType) || hasUndecided(oldType) || hasInt32(oldType));
        return (oldType & ~IndexingShapeMask) | DoubleShape;
    case AllocateContiguous:
        ASSERT(!hasIndexedProperties(oldType) || hasUndecided(oldType) || hasInt32(oldType) || hasDouble(oldType));
        return (oldType & ~IndexingShapeMask) | ContiguousShape;
    case AllocateArrayStorage:
        ASSERT(!hasIndexedProperties(oldType) || hasUndecided(oldType) || hasInt32(oldType) || hasDouble(oldType) || hasContiguous(oldType));
        return (oldType & ~IndexingShapeMask) | ArrayStorageShape;
    case AllocateSlowPutArrayStorage:
        ASSERT(!hasIndexedProperties(oldType) || hasUndecided(oldType) || hasInt32(oldType) || hasDouble(oldType) || hasContiguous(oldType) || hasContiguous(oldType));
        return (oldType & ~IndexingShapeMask) | SlowPutArrayStorageShape;
    case SwitchToSlowPutArrayStorage:
        ASSERT(hasFastArrayStorage(oldType));
        return (oldType & ~IndexingShapeMask) | SlowPutArrayStorageShape;
    case AddIndexedAccessors:
        return oldType | MayHaveIndexedAccessors;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return oldType;
    }
}

class StructureTransitionTable {
    static const intptr_t UsingSingleSlotFlag = 1;

    struct Hash {
        typedef std::pair<RefPtr<StringImpl>, unsigned> Key;
        static unsigned hash(const Key& p)
        {
            unsigned result = p.second;
            if (p.first)
                result += p.first->existingHash();
            return result;
        }

        static bool equal(const Key& a, const Key& b)
        {
            return a == b;
        }

        static const bool safeToCompareToEmptyOrDeleted = true;
    };

    typedef WeakGCMap<Hash::Key, Structure, Hash> TransitionMap;

public:
    StructureTransitionTable()
        : m_data(UsingSingleSlotFlag)
    {
    }

    ~StructureTransitionTable()
    {
        if (!isUsingSingleSlot()) {
            delete map();
            return;
        }

        WeakImpl* impl = this->weakImpl();
        if (!impl)
            return;
        WeakSet::deallocate(impl);
    }

    inline void add(VM&, Structure*);
    inline bool contains(StringImpl* rep, unsigned attributes) const;
    inline Structure* get(StringImpl* rep, unsigned attributes) const;

private:
    bool isUsingSingleSlot() const
    {
        return m_data & UsingSingleSlotFlag;
    }

    TransitionMap* map() const
    {
        ASSERT(!isUsingSingleSlot());
        return reinterpret_cast<TransitionMap*>(m_data);
    }

    WeakImpl* weakImpl() const
    {
        ASSERT(isUsingSingleSlot());
        return reinterpret_cast<WeakImpl*>(m_data & ~UsingSingleSlotFlag);
    }

    void setMap(TransitionMap* map)
    {
        ASSERT(isUsingSingleSlot());
        
        if (WeakImpl* impl = this->weakImpl())
            WeakSet::deallocate(impl);

        // This implicitly clears the flag that indicates we're using a single transition
        m_data = reinterpret_cast<intptr_t>(map);

        ASSERT(!isUsingSingleSlot());
    }

    Structure* singleTransition() const
    {
        ASSERT(isUsingSingleSlot());
        if (WeakImpl* impl = this->weakImpl()) {
            if (impl->state() == WeakImpl::Live)
                return reinterpret_cast<Structure*>(impl->jsValue().asCell());
        }
        return 0;
    }
    
    void setSingleTransition(VM&, Structure* structure)
    {
        ASSERT(isUsingSingleSlot());
        if (WeakImpl* impl = this->weakImpl())
            WeakSet::deallocate(impl);
        WeakImpl* impl = WeakSet::allocate(reinterpret_cast<JSCell*>(structure));
        m_data = reinterpret_cast<intptr_t>(impl) | UsingSingleSlotFlag;
    }

    intptr_t m_data;
};

} // namespace JSC

#endif // StructureTransitionTable_h
