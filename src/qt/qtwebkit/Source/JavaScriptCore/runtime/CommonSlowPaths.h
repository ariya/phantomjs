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

#ifndef CommonSlowPaths_h
#define CommonSlowPaths_h

#include "CodeBlock.h"
#include "CodeSpecializationKind.h"
#include "ExceptionHelpers.h"
#include "JSArray.h"
#include "NameInstance.h"

namespace JSC {

// The purpose of this namespace is to include slow paths that are shared
// between the interpreter and baseline JIT. They are written to be agnostic
// with respect to the slow-path calling convention, but they do rely on the
// JS code being executed more-or-less directly from bytecode (so the call
// frame layout is unmodified, making it potentially awkward to use these
// from any optimizing JIT, like the DFG).

namespace CommonSlowPaths {

ALWAYS_INLINE ExecState* arityCheckFor(ExecState* exec, JSStack* stack, CodeSpecializationKind kind)
{
    JSFunction* callee = jsCast<JSFunction*>(exec->callee());
    ASSERT(!callee->isHostFunction());
    CodeBlock* newCodeBlock = &callee->jsExecutable()->generatedBytecodeFor(kind);
    int argumentCountIncludingThis = exec->argumentCountIncludingThis();

    // This ensures enough space for the worst case scenario of zero arguments passed by the caller.
    if (!stack->grow(exec->registers() + newCodeBlock->numParameters() + newCodeBlock->m_numCalleeRegisters))
        return 0;

    ASSERT(argumentCountIncludingThis < newCodeBlock->numParameters());

    // Too few arguments -- copy call frame and arguments, then fill in missing arguments with undefined.
    size_t delta = newCodeBlock->numParameters() - argumentCountIncludingThis;
    Register* src = exec->registers();
    Register* dst = exec->registers() + delta;

    int i;
    int end = -ExecState::offsetFor(argumentCountIncludingThis);
    for (i = -1; i >= end; --i)
        dst[i] = src[i];

    end -= delta;
    for ( ; i >= end; --i)
        dst[i] = jsUndefined();

    ExecState* newExec = ExecState::create(dst);
    ASSERT((void*)newExec <= stack->end());
    return newExec;
}

inline bool opIn(ExecState* exec, JSValue propName, JSValue baseVal)
{
    if (!baseVal.isObject()) {
        exec->vm().exception = createInvalidParameterError(exec, "in", baseVal);
        return false;
    }

    JSObject* baseObj = asObject(baseVal);

    uint32_t i;
    if (propName.getUInt32(i))
        return baseObj->hasProperty(exec, i);

    if (isName(propName))
        return baseObj->hasProperty(exec, jsCast<NameInstance*>(propName.asCell())->privateName());

    Identifier property(exec, propName.toString(exec)->value(exec));
    if (exec->vm().exception)
        return false;
    return baseObj->hasProperty(exec, property);
}

} } // namespace JSC::CommonSlowPaths

#endif // CommonSlowPaths_h
