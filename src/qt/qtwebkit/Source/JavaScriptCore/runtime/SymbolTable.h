/*
 * Copyright (C) 2007, 2008, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SymbolTable_h
#define SymbolTable_h

#include "JSObject.h"
#include "Watchpoint.h"
#include <wtf/HashTraits.h>
#include <wtf/text/StringImpl.h>

namespace JSC {

class Watchpoint;
class WatchpointSet;

struct SlowArgument {
    enum Status {
        Normal = 0,
        Captured = 1,
        Deleted = 2
    };

    SlowArgument()
        : status(Normal)
        , index(0)
    {
    }

    Status status;
    int index; // If status is 'Deleted', index is bogus.
};

static ALWAYS_INLINE int missingSymbolMarker() { return std::numeric_limits<int>::max(); }

// The bit twiddling in this class assumes that every register index is a
// reasonably small positive or negative number, and therefore has its high
// four bits all set or all unset.

// In addition to implementing semantics-mandated variable attributes and
// implementation-mandated variable indexing, this class also implements
// watchpoints to be used for JIT optimizations. Because watchpoints are
// meant to be relatively rare, this class optimizes heavily for the case
// that they are not being used. To that end, this class uses the thin-fat
// idiom: either it is thin, in which case it contains an in-place encoded
// word that consists of attributes, the index, and a bit saying that it is
// thin; or it is fat, in which case it contains a pointer to a malloc'd
// data structure and a bit saying that it is fat. The malloc'd data
// structure will be malloced a second time upon copy, to preserve the
// property that in-place edits to SymbolTableEntry do not manifest in any
// copies. However, the malloc'd FatEntry data structure contains a ref-
// counted pointer to a shared WatchpointSet. Thus, in-place edits of the
// WatchpointSet will manifest in all copies. Here's a picture:
//
// SymbolTableEntry --> FatEntry --> WatchpointSet
//
// If you make a copy of a SymbolTableEntry, you will have:
//
// original: SymbolTableEntry --> FatEntry --> WatchpointSet
// copy:     SymbolTableEntry --> FatEntry -----^

struct SymbolTableEntry {
    // Use the SymbolTableEntry::Fast class, either via implicit cast or by calling
    // getFast(), when you (1) only care about isNull(), getIndex(), and isReadOnly(),
    // and (2) you are in a hot path where you need to minimize the number of times
    // that you branch on isFat() when getting the bits().
    class Fast {
    public:
        Fast()
            : m_bits(SlimFlag)
        {
        }
        
        ALWAYS_INLINE Fast(const SymbolTableEntry& entry)
            : m_bits(entry.bits())
        {
        }
    
        bool isNull() const
        {
            return !(m_bits & ~SlimFlag);
        }

        int getIndex() const
        {
            return static_cast<int>(m_bits >> FlagBits);
        }
    
        bool isReadOnly() const
        {
            return m_bits & ReadOnlyFlag;
        }
        
        unsigned getAttributes() const
        {
            unsigned attributes = 0;
            if (m_bits & ReadOnlyFlag)
                attributes |= ReadOnly;
            if (m_bits & DontEnumFlag)
                attributes |= DontEnum;
            return attributes;
        }

        bool isFat() const
        {
            return !(m_bits & SlimFlag);
        }
        
    private:
        friend struct SymbolTableEntry;
        intptr_t m_bits;
    };

    SymbolTableEntry()
        : m_bits(SlimFlag)
    {
    }

    SymbolTableEntry(int index)
        : m_bits(SlimFlag)
    {
        ASSERT(isValidIndex(index));
        pack(index, false, false);
    }

    SymbolTableEntry(int index, unsigned attributes)
        : m_bits(SlimFlag)
    {
        ASSERT(isValidIndex(index));
        pack(index, attributes & ReadOnly, attributes & DontEnum);
    }
    
    ~SymbolTableEntry()
    {
        freeFatEntry();
    }
    
    SymbolTableEntry(const SymbolTableEntry& other)
        : m_bits(SlimFlag)
    {
        *this = other;
    }
    
    SymbolTableEntry& operator=(const SymbolTableEntry& other)
    {
        if (UNLIKELY(other.isFat()))
            return copySlow(other);
        freeFatEntry();
        m_bits = other.m_bits;
        return *this;
    }
    
    bool isNull() const
    {
        return !(bits() & ~SlimFlag);
    }

    int getIndex() const
    {
        return static_cast<int>(bits() >> FlagBits);
    }
    
    ALWAYS_INLINE Fast getFast() const
    {
        return Fast(*this);
    }
    
    ALWAYS_INLINE Fast getFast(bool& wasFat) const
    {
        Fast result;
        wasFat = isFat();
        if (wasFat)
            result.m_bits = fatEntry()->m_bits | SlimFlag;
        else
            result.m_bits = m_bits;
        return result;
    }
    
    unsigned getAttributes() const
    {
        return getFast().getAttributes();
    }

    void setAttributes(unsigned attributes)
    {
        pack(getIndex(), attributes & ReadOnly, attributes & DontEnum);
    }

    bool isReadOnly() const
    {
        return bits() & ReadOnlyFlag;
    }
    
    bool couldBeWatched();
    
    // Notify an opportunity to create a watchpoint for a variable. This is
    // idempotent and fail-silent. It is idempotent in the sense that if
    // a watchpoint set had already been created, then another one will not
    // be created. Hence two calls to this method have the same effect as
    // one call. It is also fail-silent, in the sense that if a watchpoint
    // set had been created and had already been invalidated, then this will
    // just return. This means that couldBeWatched() may return false even
    // immediately after a call to attemptToWatch().
    void attemptToWatch();
    
    bool* addressOfIsWatched();
    
    void addWatchpoint(Watchpoint*);
    
    WatchpointSet* watchpointSet()
    {
        return fatEntry()->m_watchpoints.get();
    }
    
    ALWAYS_INLINE void notifyWrite()
    {
        if (LIKELY(!isFat()))
            return;
        notifyWriteSlow();
    }
    
private:
    static const intptr_t SlimFlag = 0x1;
    static const intptr_t ReadOnlyFlag = 0x2;
    static const intptr_t DontEnumFlag = 0x4;
    static const intptr_t NotNullFlag = 0x8;
    static const intptr_t FlagBits = 4;
    
    class FatEntry {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        FatEntry(intptr_t bits)
            : m_bits(bits & ~SlimFlag)
        {
        }
        
        intptr_t m_bits; // always has FatFlag set and exactly matches what the bits would have been if this wasn't fat.
        
        RefPtr<WatchpointSet> m_watchpoints;
    };
    
    SymbolTableEntry& copySlow(const SymbolTableEntry&);
    JS_EXPORT_PRIVATE void notifyWriteSlow();
    
    bool isFat() const
    {
        return !(m_bits & SlimFlag);
    }
    
    const FatEntry* fatEntry() const
    {
        ASSERT(isFat());
        return bitwise_cast<const FatEntry*>(m_bits);
    }
    
    FatEntry* fatEntry()
    {
        ASSERT(isFat());
        return bitwise_cast<FatEntry*>(m_bits);
    }
    
    FatEntry* inflate()
    {
        if (LIKELY(isFat()))
            return fatEntry();
        return inflateSlow();
    }
    
    FatEntry* inflateSlow();
    
    ALWAYS_INLINE intptr_t bits() const
    {
        if (isFat())
            return fatEntry()->m_bits;
        return m_bits;
    }
    
    ALWAYS_INLINE intptr_t& bits()
    {
        if (isFat())
            return fatEntry()->m_bits;
        return m_bits;
    }
    
    void freeFatEntry()
    {
        if (LIKELY(!isFat()))
            return;
        freeFatEntrySlow();
    }

    JS_EXPORT_PRIVATE void freeFatEntrySlow();

    void pack(int index, bool readOnly, bool dontEnum)
    {
        ASSERT(!isFat());
        intptr_t& bitsRef = bits();
        bitsRef = (static_cast<intptr_t>(index) << FlagBits) | NotNullFlag | SlimFlag;
        if (readOnly)
            bitsRef |= ReadOnlyFlag;
        if (dontEnum)
            bitsRef |= DontEnumFlag;
    }
    
    bool isValidIndex(int index)
    {
        return ((static_cast<intptr_t>(index) << FlagBits) >> FlagBits) == static_cast<intptr_t>(index);
    }

    intptr_t m_bits;
};

struct SymbolTableIndexHashTraits : HashTraits<SymbolTableEntry> {
    static const bool needsDestruction = true;
};

typedef HashMap<RefPtr<StringImpl>, SymbolTableEntry, IdentifierRepHash, HashTraits<RefPtr<StringImpl> >, SymbolTableIndexHashTraits> SymbolTable;

class SharedSymbolTable : public JSCell, public SymbolTable {
public:
    typedef JSCell Base;

    static SharedSymbolTable* create(VM& vm)
    {
        SharedSymbolTable* sharedSymbolTable = new (NotNull, allocateCell<SharedSymbolTable>(vm.heap)) SharedSymbolTable(vm);
        sharedSymbolTable->finishCreation(vm);
        return sharedSymbolTable;
    }
    static const bool needsDestruction = true;
    static const bool hasImmortalStructure = true;
    static void destroy(JSCell*);

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(LeafType, StructureFlags), &s_info);
    }

    bool usesNonStrictEval() { return m_usesNonStrictEval; }
    void setUsesNonStrictEval(bool usesNonStrictEval) { m_usesNonStrictEval = usesNonStrictEval; }

    int captureStart() { return m_captureStart; }
    void setCaptureStart(int captureStart) { m_captureStart = captureStart; }

    int captureEnd() { return m_captureEnd; }
    void setCaptureEnd(int captureEnd) { m_captureEnd = captureEnd; }

    int captureCount() { return m_captureEnd - m_captureStart; }

    int parameterCount() { return m_parameterCountIncludingThis - 1; }
    int parameterCountIncludingThis() { return m_parameterCountIncludingThis; }
    void setParameterCountIncludingThis(int parameterCountIncludingThis) { m_parameterCountIncludingThis = parameterCountIncludingThis; }

    // 0 if we don't capture any arguments; parameterCount() in length if we do.
    const SlowArgument* slowArguments() { return m_slowArguments.get(); }
    void setSlowArguments(PassOwnArrayPtr<SlowArgument> slowArguments) { m_slowArguments = slowArguments; }

    static JS_EXPORTDATA const ClassInfo s_info;

private:
    SharedSymbolTable(VM& vm)
        : JSCell(vm, vm.sharedSymbolTableStructure.get())
        , m_parameterCountIncludingThis(0)
        , m_usesNonStrictEval(false)
        , m_captureStart(0)
        , m_captureEnd(0)
    {
    }

    int m_parameterCountIncludingThis;
    bool m_usesNonStrictEval;

    int m_captureStart;
    int m_captureEnd;

    OwnArrayPtr<SlowArgument> m_slowArguments;
};

} // namespace JSC

#endif // SymbolTable_h
