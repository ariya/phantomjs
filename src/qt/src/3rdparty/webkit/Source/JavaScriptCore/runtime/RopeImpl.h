/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef RopeImpl_h
#define RopeImpl_h

#include <wtf/text/StringImpl.h>

namespace JSC {

class RopeImpl : public StringImplBase {
public:
    // A RopeImpl is composed from a set of smaller strings called Fibers.
    // Each Fiber in a rope is either StringImpl or another RopeImpl.
    typedef StringImplBase* Fiber;

    // Creates a RopeImpl comprising of 'fiberCount' Fibers.
    // The RopeImpl is constructed in an uninitialized state - initialize must be called for each Fiber in the RopeImpl.
    static PassRefPtr<RopeImpl> tryCreateUninitialized(unsigned fiberCount)
    {
        void* allocation;
        if (tryFastMalloc(sizeof(RopeImpl) + (fiberCount - 1) * sizeof(Fiber)).getValue(allocation))
            return adoptRef(new (allocation) RopeImpl(fiberCount));
        return 0;
    }

    static bool isRope(Fiber fiber)
    {
        return !fiber->isStringImpl();
    }

    static void deref(Fiber fiber)
    {
        if (isRope(fiber))
            static_cast<RopeImpl*>(fiber)->deref();
        else
            static_cast<StringImpl*>(fiber)->deref();
    }

    void initializeFiber(unsigned &index, Fiber fiber)
    {
        m_fibers[index++] = fiber;
        fiber->ref();
        m_length += fiber->length();
    }

    unsigned fiberCount() { return m_size; }
    Fiber* fibers() { return m_fibers; }

    ALWAYS_INLINE void deref()
    {
        m_refCountAndFlags -= s_refCountIncrement;
        if (!(m_refCountAndFlags & s_refCountMask))
            destructNonRecursive();
    }

private:
    RopeImpl(unsigned fiberCount)
        : StringImplBase(ConstructNonStringImpl)
        , m_size(fiberCount)
    {
    }

    void destructNonRecursive();
    void derefFibersNonRecursive(Vector<RopeImpl*, 32>& workQueue);

    bool hasOneRef() { return (m_refCountAndFlags & s_refCountMask) == s_refCountIncrement; }

    unsigned m_size;
    Fiber m_fibers[1];
};

}

#endif
