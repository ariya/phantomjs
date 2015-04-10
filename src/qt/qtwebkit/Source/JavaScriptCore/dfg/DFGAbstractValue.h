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

#ifndef DFGAbstractValue_h
#define DFGAbstractValue_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "ArrayProfile.h"
#include "DFGStructureAbstractValue.h"
#include "JSCell.h"
#include "SpeculatedType.h"
#include "StructureSet.h"

namespace JSC { namespace DFG {

struct AbstractValue {
    AbstractValue()
        : m_type(SpecNone)
        , m_arrayModes(0)
    {
    }
    
    void clear()
    {
        m_type = SpecNone;
        m_arrayModes = 0;
        m_currentKnownStructure.clear();
        m_futurePossibleStructure.clear();
        m_value = JSValue();
        checkConsistency();
    }
    
    bool isClear() const
    {
        bool result = m_type == SpecNone && !m_arrayModes && m_currentKnownStructure.isClear() && m_futurePossibleStructure.isClear();
        if (result)
            ASSERT(!m_value);
        return result;
    }
    
    void makeTop()
    {
        m_type |= SpecTop; // The state may have included SpecEmpty, in which case we want this to become SpecEmptyOrTop.
        m_arrayModes = ALL_ARRAY_MODES;
        m_currentKnownStructure.makeTop();
        m_futurePossibleStructure.makeTop();
        m_value = JSValue();
        checkConsistency();
    }
    
    void clobberStructures()
    {
        if (m_type & SpecCell) {
            m_currentKnownStructure.makeTop();
            clobberArrayModes();
        } else {
            ASSERT(m_currentKnownStructure.isClear());
            ASSERT(!m_arrayModes);
        }
        checkConsistency();
    }
        
    void clobberValue()
    {
        m_value = JSValue();
    }
    
    bool isTop() const
    {
        return m_type == SpecTop && m_currentKnownStructure.isTop() && m_futurePossibleStructure.isTop();
    }
    
    bool valueIsTop() const
    {
        return !m_value && m_type;
    }
    
    JSValue value() const
    {
        return m_value;
    }
    
    static AbstractValue top()
    {
        AbstractValue result;
        result.makeTop();
        return result;
    }
    
    void setMostSpecific(JSValue value)
    {
        if (!!value && value.isCell()) {
            Structure* structure = value.asCell()->structure();
            m_currentKnownStructure = structure;
            setFuturePossibleStructure(structure);
            m_arrayModes = asArrayModes(structure->indexingType());
        } else {
            m_currentKnownStructure.clear();
            m_futurePossibleStructure.clear();
            m_arrayModes = 0;
        }
        
        m_type = speculationFromValue(value);
        m_value = value;
        
        checkConsistency();
    }
    
    void set(JSValue value)
    {
        if (!!value && value.isCell()) {
            m_currentKnownStructure.makeTop();
            Structure* structure = value.asCell()->structure();
            setFuturePossibleStructure(structure);
            m_arrayModes = asArrayModes(structure->indexingType());
            clobberArrayModes();
        } else {
            m_currentKnownStructure.clear();
            m_futurePossibleStructure.clear();
            m_arrayModes = 0;
        }
        
        m_type = speculationFromValue(value);
        m_value = value;
        
        checkConsistency();
    }
    
    void set(Structure* structure)
    {
        m_currentKnownStructure = structure;
        setFuturePossibleStructure(structure);
        m_arrayModes = asArrayModes(structure->indexingType());
        m_type = speculationFromStructure(structure);
        m_value = JSValue();
        
        checkConsistency();
    }
    
    void set(SpeculatedType type)
    {
        if (type & SpecCell) {
            m_currentKnownStructure.makeTop();
            m_futurePossibleStructure.makeTop();
            m_arrayModes = ALL_ARRAY_MODES;
        } else {
            m_currentKnownStructure.clear();
            m_futurePossibleStructure.clear();
            m_arrayModes = 0;
        }
        m_type = type;
        m_value = JSValue();
        checkConsistency();
    }
    
    bool operator==(const AbstractValue& other) const
    {
        return m_type == other.m_type
            && m_arrayModes == other.m_arrayModes
            && m_currentKnownStructure == other.m_currentKnownStructure
            && m_futurePossibleStructure == other.m_futurePossibleStructure
            && m_value == other.m_value;
    }
    bool operator!=(const AbstractValue& other) const
    {
        return !(*this == other);
    }
    
    bool merge(const AbstractValue& other)
    {
        if (other.isClear())
            return false;
        
#if !ASSERT_DISABLED
        AbstractValue oldMe = *this;
#endif
        bool result = false;
        if (isClear()) {
            *this = other;
            result = !other.isClear();
        } else {
            result |= mergeSpeculation(m_type, other.m_type);
            result |= mergeArrayModes(m_arrayModes, other.m_arrayModes);
            result |= m_currentKnownStructure.addAll(other.m_currentKnownStructure);
            result |= m_futurePossibleStructure.addAll(other.m_futurePossibleStructure);
            if (m_value != other.m_value) {
                result |= !!m_value;
                m_value = JSValue();
            }
        }
        checkConsistency();
        ASSERT(result == (*this != oldMe));
        return result;
    }
    
    void merge(SpeculatedType type)
    {
        mergeSpeculation(m_type, type);
        
        if (type & SpecCell) {
            m_currentKnownStructure.makeTop();
            m_futurePossibleStructure.makeTop();
            m_arrayModes = ALL_ARRAY_MODES;
        }
        m_value = JSValue();

        checkConsistency();
    }
    
    void filter(const StructureSet& other)
    {
        // FIXME: This could be optimized for the common case of m_type not
        // having structures, array modes, or a specific value.
        // https://bugs.webkit.org/show_bug.cgi?id=109663
        m_type &= other.speculationFromStructures();
        m_arrayModes &= other.arrayModesFromStructures();
        m_currentKnownStructure.filter(other);
        if (m_currentKnownStructure.isClear())
            m_futurePossibleStructure.clear();
        else if (m_currentKnownStructure.hasSingleton())
            filterFuturePossibleStructure(m_currentKnownStructure.singleton());
        
        // It's possible that prior to the above two statements we had (Foo, TOP), where
        // Foo is a SpeculatedType that is disjoint with the passed StructureSet. In that
        // case, we will now have (None, [someStructure]). In general, we need to make
        // sure that new information gleaned from the SpeculatedType needs to be fed back
        // into the information gleaned from the StructureSet.
        m_currentKnownStructure.filter(m_type);
        m_futurePossibleStructure.filter(m_type);
        
        filterArrayModesByType();
        filterValueByType();
        
        checkConsistency();
    }
    
    void filterArrayModes(ArrayModes arrayModes)
    {
        ASSERT(arrayModes);
        
        m_type &= SpecCell;
        m_arrayModes &= arrayModes;
        
        // I could do more fancy filtering here. But it probably won't make any difference.
        
        checkConsistency();
    }
    
    void filter(SpeculatedType type)
    {
        if (type == SpecTop)
            return;
        m_type &= type;
        
        // It's possible that prior to this filter() call we had, say, (Final, TOP), and
        // the passed type is Array. At this point we'll have (None, TOP). The best way
        // to ensure that the structure filtering does the right thing is to filter on
        // the new type (None) rather than the one passed (Array).
        m_currentKnownStructure.filter(m_type);
        m_futurePossibleStructure.filter(m_type);
        
        filterArrayModesByType();
        filterValueByType();
        
        checkConsistency();
    }
    
    void filterByValue(JSValue value)
    {
        filter(speculationFromValue(value));
        if (m_type)
            m_value = value;
    }
    
    bool validateType(JSValue value) const
    {
        if (isTop())
            return true;
        
        if (mergeSpeculations(m_type, speculationFromValue(value)) != m_type)
            return false;
        
        if (value.isEmpty()) {
            ASSERT(m_type & SpecEmpty);
            return true;
        }
        
        return true;
    }
    
    bool validate(JSValue value) const
    {
        if (isTop())
            return true;
        
        if (!!m_value && m_value != value)
            return false;
        
        if (mergeSpeculations(m_type, speculationFromValue(value)) != m_type)
            return false;
        
        if (value.isEmpty()) {
            ASSERT(m_type & SpecEmpty);
            return true;
        }
        
        if (!!value && value.isCell()) {
            ASSERT(m_type & SpecCell);
            Structure* structure = value.asCell()->structure();
            return m_currentKnownStructure.contains(structure)
                && m_futurePossibleStructure.contains(structure)
                && (m_arrayModes & asArrayModes(structure->indexingType()));
        }
        
        return true;
    }
    
    Structure* bestProvenStructure() const
    {
        if (m_currentKnownStructure.hasSingleton())
            return m_currentKnownStructure.singleton();
        if (m_futurePossibleStructure.hasSingleton())
            return m_futurePossibleStructure.singleton();
        return 0;
    }
    
    void checkConsistency() const
    {
        if (!(m_type & SpecCell)) {
            ASSERT(m_currentKnownStructure.isClear());
            ASSERT(m_futurePossibleStructure.isClear());
            ASSERT(!m_arrayModes);
        }
        
        if (isClear())
            ASSERT(!m_value);
        
        if (!!m_value)
            ASSERT(mergeSpeculations(m_type, speculationFromValue(m_value)) == m_type);
        
        // Note that it's possible for a prediction like (Final, []). This really means that
        // the value is bottom and that any code that uses the value is unreachable. But
        // we don't want to get pedantic about this as it would only increase the computational
        // complexity of the code.
    }
    
    void dump(PrintStream& out) const
    {
        out.print(
            "(", SpeculationDump(m_type), ", ", ArrayModesDump(m_arrayModes), ", ",
            m_currentKnownStructure, ", ", m_futurePossibleStructure);
        if (!!m_value)
            out.print(", ", m_value);
        out.print(")");
    }
    
    // A great way to think about the difference between m_currentKnownStructure and
    // m_futurePossibleStructure is to consider these four examples:
    //
    // 1) x = foo();
    //
    //    In this case x's m_currentKnownStructure and m_futurePossibleStructure will
    //    both be TOP, since we don't know anything about x for sure, yet.
    //
    // 2) x = foo();
    //    y = x.f;
    //
    //    Where x will later have a new property added to it, 'g'. Because of the
    //    known but not-yet-executed property addition, x's currently structure will
    //    not be watchpointable; hence we have no way of statically bounding the set
    //    of possible structures that x may have if a clobbering event happens. So,
    //    x's m_currentKnownStructure will be whatever structure we check to get
    //    property 'f', and m_futurePossibleStructure will be TOP.
    //
    // 3) x = foo();
    //    y = x.f;
    //
    //    Where x has a terminal structure that is still watchpointable. In this case,
    //    x's m_currentKnownStructure and m_futurePossibleStructure will both be
    //    whatever structure we checked for when getting 'f'.
    //
    // 4) x = foo();
    //    y = x.f;
    //    bar();
    //
    //    Where x has a terminal structure that is still watchpointable. In this
    //    case, m_currentKnownStructure will be TOP because bar() may potentially
    //    change x's structure and we have no way of proving otherwise, but
    //    x's m_futurePossibleStructure will be whatever structure we had checked
    //    when getting property 'f'.
    
    // NB. All fields in this struct must have trivial destructors.

    // This is a proven constraint on the structures that this value can have right
    // now. The structure of the current value must belong to this set. The set may
    // be TOP, indicating that it is the set of all possible structures, in which
    // case the current value can have any structure. The set may be BOTTOM (empty)
    // in which case this value cannot be a cell. This is all subject to change
    // anytime a new value is assigned to this one, anytime there is a control flow
    // merge, or most crucially, anytime a side-effect or structure check happens.
    // In case of a side-effect, we typically must assume that any value may have
    // had its structure changed, hence contravening our proof. We make the proof
    // valid again by switching this to TOP (i.e. claiming that we have proved that
    // this value may have any structure). Of note is that the proof represented by
    // this field is not subject to structure transition watchpoints - even if one
    // fires, we can be sure that this proof is still valid.
    StructureAbstractValue m_currentKnownStructure;
    
    // This is a proven constraint on the structures that this value can have now
    // or any time in the future subject to the structure transition watchpoints of
    // all members of this set not having fired. This set is impervious to side-
    // effects; even if one happens the side-effect can only cause the value to
    // change to at worst another structure that is also a member of this set. But,
    // the theorem being proved by this field is predicated upon there not being
    // any new structure transitions introduced into any members of this set. In
    // cases where there is no way for us to guard this happening, the set must be
    // TOP. But in cases where we can guard new structure transitions (all members
    // of the set have still-valid structure transition watchpoints) then this set
    // will be finite. Anytime that we make use of the finite nature of this set,
    // we must first issue a structure transition watchpoint, which will effectively
    // result in m_currentKnownStructure being filtered according to
    // m_futurePossibleStructure.
    StructureAbstractValue m_futurePossibleStructure;
    
    // This is a proven constraint on the possible types that this value can have
    // now or any time in the future, unless it is reassigned. This field is
    // impervious to side-effects unless the side-effect can reassign the value
    // (for example if we're talking about a captured variable). The relationship
    // between this field, and the structure fields above, is as follows. The
    // fields above constraint the structures that a cell may have, but they say
    // nothing about whether or not the value is known to be a cell. More formally,
    // the m_currentKnownStructure is itself an abstract value that consists of the
    // union of the set of all non-cell values and the set of cell values that have
    // the given structure. This abstract value is then the intersection of the
    // m_currentKnownStructure and the set of values whose type is m_type. So, for
    // example if m_type is SpecFinal|SpecInt32 and m_currentKnownStructure is
    // [0x12345] then this abstract value corresponds to the set of all integers
    // unified with the set of all objects with structure 0x12345.
    SpeculatedType m_type;
    
    // This is a proven constraint on the possible indexing types that this value
    // can have right now. It also implicitly constraints the set of structures
    // that the value may have right now, since a structure has an immutable
    // indexing type. This is subject to change upon reassignment, or any side
    // effect that makes non-obvious changes to the heap.
    ArrayModes m_arrayModes;
    
    // This is a proven constraint on the possible values that this value can
    // have now or any time in the future, unless it is reassigned. Note that this
    // implies nothing about the structure. Oddly, JSValue() (i.e. the empty value)
    // means either BOTTOM or TOP depending on the state of m_type: if m_type is
    // BOTTOM then JSValue() means BOTTOM; if m_type is not BOTTOM then JSValue()
    // means TOP.
    JSValue m_value;

private:
    void clobberArrayModes()
    {
        // FIXME: We could make this try to predict the set of array modes that this object
        // could have in the future. For now, just do the simple thing.
        m_arrayModes = ALL_ARRAY_MODES;
    }
    
    void setFuturePossibleStructure(Structure* structure)
    {
        if (structure->transitionWatchpointSetIsStillValid())
            m_futurePossibleStructure = structure;
        else
            m_futurePossibleStructure.makeTop();
    }
    
    void filterFuturePossibleStructure(Structure* structure)
    {
        if (structure->transitionWatchpointSetIsStillValid())
            m_futurePossibleStructure.filter(StructureAbstractValue(structure));
    }

    // We could go further, and ensure that if the futurePossibleStructure contravenes
    // the value, then we could clear both of those things. But that's unlikely to help
    // in any realistic scenario, so we don't do it. Simpler is better.
    void filterValueByType()
    {
        if (!!m_type) {
            // The type is still non-empty. This implies that regardless of what filtering
            // was done, we either didn't have a value to begin with, or that value is still
            // valid.
            ASSERT(!m_value || validateType(m_value));
            return;
        }
        
        // The type has been rendered empty. That means that the value must now be invalid,
        // as well.
        ASSERT(!m_value || !validateType(m_value));
        m_value = JSValue();
    }
    
    void filterArrayModesByType()
    {
        if (!(m_type & SpecCell))
            m_arrayModes = 0;
        else if (!(m_type & ~SpecArray))
            m_arrayModes &= ALL_ARRAY_ARRAY_MODES;

        // NOTE: If m_type doesn't have SpecArray set, that doesn't mean that the
        // array modes have to be a subset of ALL_NON_ARRAY_ARRAY_MODES, since
        // in the speculated type type-system, RegExpMatchesArry and ArrayPrototype
        // are Otherobj (since they are not *exactly* JSArray) but in the ArrayModes
        // type system they are arrays (since they expose the magical length
        // property and are otherwise allocated using array allocation). Hence the
        // following would be wrong:
        //
        // if (!(m_type & SpecArray))
        //    m_arrayModes &= ALL_NON_ARRAY_ARRAY_MODES;
    }
};

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGAbstractValue_h


