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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SlotVisitor_h
#define SlotVisitor_h

#include "HandleTypes.h"
#include "MarkStackInlines.h"

#include <wtf/text/StringHash.h>

namespace JSC {

class ConservativeRoots;
class GCThreadSharedData;
class Heap;
template<typename T> class Weak;
template<typename T> class WriteBarrierBase;
template<typename T> class JITWriteBarrier;

class SlotVisitor {
    WTF_MAKE_NONCOPYABLE(SlotVisitor);
    friend class HeapRootVisitor; // Allowed to mark a JSValue* or JSCell** directly.

public:
    SlotVisitor(GCThreadSharedData&);
    ~SlotVisitor();

    void append(ConservativeRoots&);
    
    template<typename T> void append(JITWriteBarrier<T>*);
    template<typename T> void append(WriteBarrierBase<T>*);
    void appendValues(WriteBarrierBase<Unknown>*, size_t count);
    
    template<typename T>
    void appendUnbarrieredPointer(T**);
    void appendUnbarrieredValue(JSValue*);
    template<typename T>
    void appendUnbarrieredWeak(Weak<T>*);
    
    void addOpaqueRoot(void*);
    bool containsOpaqueRoot(void*);
    TriState containsOpaqueRootTriState(void*);
    int opaqueRootCount();

    GCThreadSharedData& sharedData() { return m_shared; }
    bool isEmpty() { return m_stack.isEmpty(); }

    void setup();
    void reset();

    size_t visitCount() const { return m_visitCount; }

    void donate();
    void drain();
    void donateAndDrain();
    
    enum SharedDrainMode { SlaveDrain, MasterDrain };
    void drainFromShared(SharedDrainMode);

    void harvestWeakReferences();
    void finalizeUnconditionalFinalizers();

    void copyLater(JSCell*, void*, size_t);
    
#if ENABLE(SIMPLE_HEAP_PROFILING)
    VTableSpectrum m_visitedTypeCounts;
#endif

    void addWeakReferenceHarvester(WeakReferenceHarvester*);
    void addUnconditionalFinalizer(UnconditionalFinalizer*);

#if ENABLE(OBJECT_MARK_LOGGING)
    inline void resetChildCount() { m_logChildCount = 0; }
    inline unsigned childCount() { return m_logChildCount; }
    inline void incrementChildCount() { m_logChildCount++; }
#endif

private:
    friend class ParallelModeEnabler;
    
    JS_EXPORT_PRIVATE static void validate(JSCell*);

    void append(JSValue*);
    void append(JSValue*, size_t count);
    void append(JSCell**);

    void internalAppend(JSCell*);
    void internalAppend(JSValue);
    void internalAppend(JSValue*);
    
    JS_EXPORT_PRIVATE void mergeOpaqueRoots();
    void mergeOpaqueRootsIfNecessary();
    void mergeOpaqueRootsIfProfitable();
    
    void donateKnownParallel();

    MarkStackArray m_stack;
    HashSet<void*> m_opaqueRoots; // Handle-owning data structures not visible to the garbage collector.
    
    size_t m_visitCount;
    bool m_isInParallelMode;
    
    GCThreadSharedData& m_shared;

    bool m_shouldHashCons; // Local per-thread copy of shared flag for performance reasons
    typedef HashMap<StringImpl*, JSValue> UniqueStringMap;
    UniqueStringMap m_uniqueStrings;

#if ENABLE(OBJECT_MARK_LOGGING)
    unsigned m_logChildCount;
#endif

public:
#if !ASSERT_DISABLED
    bool m_isCheckingForDefaultMarkViolation;
    bool m_isDraining;
#endif
};

class ParallelModeEnabler {
public:
    ParallelModeEnabler(SlotVisitor& stack)
        : m_stack(stack)
    {
        ASSERT(!m_stack.m_isInParallelMode);
        m_stack.m_isInParallelMode = true;
    }
    
    ~ParallelModeEnabler()
    {
        ASSERT(m_stack.m_isInParallelMode);
        m_stack.m_isInParallelMode = false;
    }
    
private:
    SlotVisitor& m_stack;
};

} // namespace JSC

#endif // SlotVisitor_h
