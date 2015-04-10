/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef VMStackBounds_h
#define VMStackBounds_h

#include "VM.h"
#include <wtf/StackBounds.h>

namespace JSC {

class VMStackBounds {
public:
    VMStackBounds(VM& vm, const StackBounds& bounds)
        : m_vm(vm)
        , m_bounds(bounds)
    {
    }

    bool isSafeToRecurse() const { return m_bounds.isSafeToRecurse(requiredCapacity()); }

private:
    inline size_t requiredCapacity() const
    {
        Interpreter* interpreter = m_vm.interpreter;

        // We have two separate stack limits, one for regular JS execution, and one
        // for when we're handling errors. We need the error stack to be smaller
        // otherwise there would obviously not be any stack left to execute JS in when
        // there's a stack overflow.
        //
        // These sizes were derived from the stack usage of a number of sites when
        // layout occurs when we've already consumed most of the C stack.
        const size_t requiredStack = 128 * KB;
        const size_t errorModeRequiredStack = 64 * KB;

        size_t requiredCapacity = interpreter->isInErrorHandlingMode() ? errorModeRequiredStack : requiredStack;
        RELEASE_ASSERT(m_bounds.size() >= requiredCapacity);
        return requiredCapacity; 
    }

    VM& m_vm;
    const StackBounds& m_bounds;
};

} // namespace JSC

#endif // VMStackBounds_h

