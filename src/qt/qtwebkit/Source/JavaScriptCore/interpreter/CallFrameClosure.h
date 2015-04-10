/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef CallFrameClosure_h
#define CallFrameClosure_h

namespace JSC {

struct CallFrameClosure {
    CallFrame* oldCallFrame;
    CallFrame* newCallFrame;
    JSFunction* function;
    FunctionExecutable* functionExecutable;
    VM* vm;
    JSScope* scope;
    int parameterCountIncludingThis;
    int argumentCountIncludingThis;
    
    void setThis(JSValue value)
    {
        newCallFrame->setThisValue(value);
    }

    void setArgument(int argument, JSValue value)
    {
        newCallFrame->setArgument(argument, value);
    }

    void resetCallFrame()
    {
        newCallFrame->setScope(scope);
        // setArgument() takes an arg index that starts from 0 for the first
        // argument after the 'this' value. Since both argumentCountIncludingThis
        // and parameterCountIncludingThis includes the 'this' value, we need to
        // subtract 1 from them to make i a valid argument index for setArgument().
        for (int i = argumentCountIncludingThis-1; i < parameterCountIncludingThis-1; ++i)
            newCallFrame->setArgument(i, jsUndefined());
    }
};

}

#endif
