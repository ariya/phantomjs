/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef JITStubRoutine_h
#define JITStubRoutine_h

#include <wtf/Platform.h>

#if ENABLE(JIT)

#include "ExecutableAllocator.h"
#include "MacroAssemblerCodeRef.h"
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace JSC {

class JITStubRoutineSet;

// This is a base-class for JIT stub routines, and also the class you want
// to instantiate directly if you have a routine that does not need any
// help from the GC. If in doubt, use one of the other stub routines. But
// if you know for sure that the stub routine cannot be on the stack while
// someone triggers a stub routine reset, then using this will speed up
// memory reclamation. One case where a stub routine satisfies this
// condition is if it doesn't make any calls, to either C++ or JS code. In
// such a routine you know that it cannot be on the stack when anything
// interesting happens.
// See GCAwareJITStubRoutine.h for the other stub routines.
class JITStubRoutine {
    WTF_MAKE_NONCOPYABLE(JITStubRoutine);
    WTF_MAKE_FAST_ALLOCATED;
public:
    JITStubRoutine(const MacroAssemblerCodeRef& code)
        : m_code(code)
        , m_refCount(1)
    {
    }
    
    // Use this if you want to pass a CodePtr to someone who insists on taking
    // a RefPtr<JITStubRoutine>.
    static PassRefPtr<JITStubRoutine> createSelfManagedRoutine(
        MacroAssemblerCodePtr rawCodePointer)
    {
        return adoptRef(new JITStubRoutine(MacroAssemblerCodeRef::createSelfManagedCodeRef(rawCodePointer)));
    }
    
    virtual ~JITStubRoutine();
    
    // MacroAssemblerCodeRef is copyable, but at the cost of reference
    // counting churn. Returning a reference is a good way of reducing
    // the churn.
    const MacroAssemblerCodeRef& code() const { return m_code; }
    
    static MacroAssemblerCodePtr asCodePtr(PassRefPtr<JITStubRoutine> stubRoutine)
    {
        if (!stubRoutine)
            return MacroAssemblerCodePtr();
        
        MacroAssemblerCodePtr result = stubRoutine->code().code();
        ASSERT(!!result);
        return result;
    }
    
    void ref()
    {
        m_refCount++;
    }
    
    void deref()
    {
        if (--m_refCount)
            return;
        observeZeroRefCount();
    }
    
    // Helpers for the GC to determine how to deal with marking JIT stub
    // routines.
    uintptr_t startAddress() const { return m_code.executableMemory()->startAsInteger(); }
    uintptr_t endAddress() const { return m_code.executableMemory()->endAsInteger(); }
    static uintptr_t addressStep() { return jitAllocationGranule; }
    
    static bool canPerformRangeFilter()
    {
#if ENABLE(EXECUTABLE_ALLOCATOR_FIXED)
        return true;
#else
        return false;
#endif
    }
    static uintptr_t filteringStartAddress()
    {
#if ENABLE(EXECUTABLE_ALLOCATOR_FIXED)
        return startOfFixedExecutableMemoryPool;
#else
        UNREACHABLE_FOR_PLATFORM();
        return 0;
#endif
    }
    static size_t filteringExtentSize()
    {
#if ENABLE(EXECUTABLE_ALLOCATOR_FIXED)
        return fixedExecutableMemoryPoolSize;
#else
        UNREACHABLE_FOR_PLATFORM();
        return 0;
#endif
    }
    static bool passesFilter(uintptr_t address)
    {
        if (!canPerformRangeFilter()) {
            // Just check that the address doesn't use any special values that would make
            // our hashtables upset.
            return address >= jitAllocationGranule && address != std::numeric_limits<uintptr_t>::max();
        }
        
        if (address - filteringStartAddress() >= filteringExtentSize())
            return false;
        
        return true;
    }

protected:
    virtual void observeZeroRefCount();

    MacroAssemblerCodeRef m_code;
    unsigned m_refCount;
};

// Helper for the creation of simple stub routines that need no help from the GC.
#define FINALIZE_CODE_FOR_STUB(patchBuffer, dataLogFArguments) \
    (adoptRef(new JITStubRoutine(FINALIZE_CODE((patchBuffer), dataLogFArguments))))

#define FINALIZE_CODE_FOR_DFG_STUB(patchBuffer, dataLogFArguments) \
    (adoptRef(new JITStubRoutine(FINALIZE_DFG_CODE((patchBuffer), dataLogFArguments))))

} // namespace JSC

#endif // ENABLE(JIT)

#endif // JITStubRoutine_h

