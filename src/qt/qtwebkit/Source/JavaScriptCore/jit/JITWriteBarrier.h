/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef JITWriteBarrier_h
#define JITWriteBarrier_h

#if ENABLE(JIT)

#include "MacroAssembler.h"
#include "SlotVisitor.h"
#include "UnusedPointer.h"
#include "WriteBarrier.h"

namespace JSC {

class JSCell;
class VM;

// Needs to be even to appease some of the backends.
#define JITWriteBarrierFlag ((void*)2)
class JITWriteBarrierBase {
public:
    typedef void* (JITWriteBarrierBase::*UnspecifiedBoolType);
    operator UnspecifiedBoolType*() const { return get() ? reinterpret_cast<UnspecifiedBoolType*>(1) : 0; }
    bool operator!() const { return !get(); }

    void setFlagOnBarrier()
    {
        ASSERT(!m_location);
        m_location = CodeLocationDataLabelPtr(JITWriteBarrierFlag);
    }

    bool isFlagged() const
    {
        return !!m_location;
    }

    void setLocation(CodeLocationDataLabelPtr location)
    {
        ASSERT(!m_location);
        m_location = location;
    }

    CodeLocationDataLabelPtr location() const
    {
        ASSERT((!!m_location) && m_location.executableAddress() != JITWriteBarrierFlag);
        return m_location;
    }
    
    void clear() { clear(0); }
    void clearToUnusedPointer() { clear(reinterpret_cast<void*>(unusedPointer)); }

protected:
    JITWriteBarrierBase()
    {
    }

    void set(VM&, CodeLocationDataLabelPtr location, JSCell* owner, JSCell* value)
    {
        Heap::writeBarrier(owner, value);
        m_location = location;
        ASSERT(((!!m_location) && m_location.executableAddress() != JITWriteBarrierFlag) || (location.executableAddress() == m_location.executableAddress()));
        MacroAssembler::repatchPointer(m_location, value);
        ASSERT(get() == value);
    }

    JSCell* get() const
    {
        if (!m_location || m_location.executableAddress() == JITWriteBarrierFlag)
            return 0;
        void* result = static_cast<JSCell*>(MacroAssembler::readPointer(m_location));
        if (result == reinterpret_cast<void*>(unusedPointer))
            return 0;
        return static_cast<JSCell*>(result);
    }

private:
    void clear(void* clearedValue)
    {
        if (!m_location)
            return;
        if (m_location.executableAddress() != JITWriteBarrierFlag)
            MacroAssembler::repatchPointer(m_location, clearedValue);
    }

    CodeLocationDataLabelPtr m_location;
};

#undef JITWriteBarrierFlag

template <typename T> class JITWriteBarrier : public JITWriteBarrierBase {
public:
    JITWriteBarrier()
    {
    }

    void set(VM& vm, CodeLocationDataLabelPtr location, JSCell* owner, T* value)
    {
        validateCell(owner);
        validateCell(value);
        JITWriteBarrierBase::set(vm, location, owner, value);
    }
    void set(VM& vm, JSCell* owner, T* value)
    {
        set(vm, location(), owner, value);
    }
    T* get() const
    {
        T* result = static_cast<T*>(JITWriteBarrierBase::get());
        if (result)
            validateCell(result);
        return result;
    }
};

template<typename T> inline void SlotVisitor::append(JITWriteBarrier<T>* slot)
{
    internalAppend(slot->get());
}

}

#endif // ENABLE(JIT)

#endif
