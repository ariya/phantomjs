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

#ifndef JSFunctionInlines_h
#define JSFunctionInlines_h

#include "Executable.h"
#include "JSFunction.h"

namespace JSC {

inline JSFunction::JSFunction(VM& vm, FunctionExecutable* executable, JSScope* scope)
    : Base(vm, scope->globalObject()->functionStructure())
    , m_executable(vm, this, executable)
    , m_scope(vm, this, scope)
    , m_allocationProfileWatchpoint(InitializedBlind) // See comment in JSFunction.cpp concerning the reason for using InitializedBlind as opposed to InitializedWatching.
{
}

inline FunctionExecutable* JSFunction::jsExecutable() const
{
    ASSERT(!isHostFunctionNonInline());
    return static_cast<FunctionExecutable*>(m_executable.get());
}

inline bool JSFunction::isHostFunction() const
{
    ASSERT(m_executable);
    return m_executable->isHostFunction();
}

inline NativeFunction JSFunction::nativeFunction()
{
    ASSERT(isHostFunction());
    return static_cast<NativeExecutable*>(m_executable.get())->function();
}

inline NativeFunction JSFunction::nativeConstructor()
{
    ASSERT(isHostFunction());
    return static_cast<NativeExecutable*>(m_executable.get())->constructor();
}

} // namespace JSC

#endif // JSFunctionInlines_h

