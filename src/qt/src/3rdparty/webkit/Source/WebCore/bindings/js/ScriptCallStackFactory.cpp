/*
 * Copyright (c) 2010 Google Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ScriptCallStackFactory.h"

#include "JSDOMBinding.h"
#include "ScriptArguments.h"
#include "ScriptCallFrame.h"
#include "ScriptCallStack.h"
#include "ScriptValue.h"
#include <interpreter/CallFrame.h>
#include <interpreter/Interpreter.h>
#include <runtime/ArgList.h>
#include <runtime/JSFunction.h>
#include <runtime/JSGlobalData.h>
#include <runtime/JSValue.h>
#include <runtime/UString.h>

using namespace JSC;

namespace WebCore {

PassRefPtr<ScriptCallStack> createScriptCallStack(size_t, bool)
{
    return 0;
}

PassRefPtr<ScriptCallStack> createScriptCallStack(JSC::ExecState* exec, size_t maxStackSize)
{
    Vector<ScriptCallFrame> frames;
    CallFrame* callFrame = exec;
    while (true) {
        ASSERT(callFrame);
        int signedLineNumber;
        intptr_t sourceID;
        UString urlString;
        JSValue function;

        exec->interpreter()->retrieveLastCaller(callFrame, signedLineNumber, sourceID, urlString, function);
        UString functionName;
        if (function)
            functionName = asFunction(function)->name(exec);
        else {
            // Caller is unknown, but if frames is empty we should still add the frame, because
            // something called us, and gave us arguments.
            if (!frames.isEmpty())
                break;
        }
        unsigned lineNumber = signedLineNumber >= 0 ? signedLineNumber : 0;
        frames.append(ScriptCallFrame(ustringToString(functionName), ustringToString(urlString), lineNumber));
        if (!function || frames.size() == maxStackSize)
            break;
        callFrame = callFrame->callerFrame();
    }
    return ScriptCallStack::create(frames);
}

PassRefPtr<ScriptArguments> createScriptArguments(JSC::ExecState* exec, unsigned skipArgumentCount)
{
    Vector<ScriptValue> arguments;
    size_t argumentCount = exec->argumentCount();
    for (size_t i = skipArgumentCount; i < argumentCount; ++i)
        arguments.append(ScriptValue(exec->globalData(), exec->argument(i)));
    return ScriptArguments::create(exec, arguments);
}

} // namespace WebCore
