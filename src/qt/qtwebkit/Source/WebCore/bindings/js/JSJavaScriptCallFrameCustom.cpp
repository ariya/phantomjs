/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#include "config.h"

#if ENABLE(JAVASCRIPT_DEBUGGER)

#include "JSJavaScriptCallFrame.h"

#include "JavaScriptCallFrame.h"
#include <runtime/ArrayPrototype.h>
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

JSValue JSJavaScriptCallFrame::evaluate(ExecState* exec)
{
    JSValue exception;
    JSValue result = impl()->evaluate(exec->argument(0).toString(exec)->value(exec), exception);

    if (exception)
        throwError(exec, exception);

    return result;
}

JSValue JSJavaScriptCallFrame::restart(ExecState*)
{
    // FIXME(40300): implement this.
    return JSValue(JSValue::JSFalse);
}

JSValue JSJavaScriptCallFrame::thisObject(ExecState*) const
{
    return impl()->thisObject() ? JSValue(impl()->thisObject()) : jsNull();
}

JSValue JSJavaScriptCallFrame::type(ExecState* exec) const
{
    switch (impl()->type()) {
        case DebuggerCallFrame::FunctionType:
            return jsString(exec, ASCIILiteral("function"));
        case DebuggerCallFrame::ProgramType:
            return jsString(exec, ASCIILiteral("program"));
    }

    ASSERT_NOT_REACHED();
    return jsNull();
}

JSValue JSJavaScriptCallFrame::scopeChain(ExecState* exec) const
{
    if (!impl()->scopeChain())
        return jsNull();

    JSScope* scopeChain = impl()->scopeChain();
    ScopeChainIterator iter = scopeChain->begin();
    ScopeChainIterator end = scopeChain->end();

    // we must always have something in the scope chain
    ASSERT(iter != end);

    MarkedArgumentBuffer list;
    do {
        list.append(iter.get());
        ++iter;
    } while (iter != end);

    return constructArray(exec, 0, globalObject(), list);
}

JSValue JSJavaScriptCallFrame::scopeType(ExecState* exec)
{
    if (!impl()->scopeChain())
        return jsUndefined();

    if (!exec->argument(0).isInt32())
        return jsUndefined();
    int index = exec->argument(0).asInt32();

    JSScope* scopeChain = impl()->scopeChain();
    ScopeChainIterator end = scopeChain->end();

    bool foundLocalScope = false;
    for (ScopeChainIterator iter = scopeChain->begin(); iter != end; ++iter) {
        JSObject* scope = iter.get();
        if (scope->isActivationObject()) {
            if (!foundLocalScope) {
                // First activation object is local scope, each successive activation object is closure.
                if (!index)
                    return jsJavaScriptCallFrameLOCAL_SCOPE(exec, JSValue(), Identifier());
                foundLocalScope = true;
            } else if (!index)
                return jsJavaScriptCallFrameCLOSURE_SCOPE(exec, JSValue(), Identifier());
        }

        if (!index) {
            // Last in the chain is global scope.
            if (++iter == end)
                return jsJavaScriptCallFrameGLOBAL_SCOPE(exec, JSValue(), Identifier());
            return jsJavaScriptCallFrameWITH_SCOPE(exec, JSValue(), Identifier());
        }

        --index;
    }
    return jsUndefined();
}

JSValue JSJavaScriptCallFrame::setVariableValue(JSC::ExecState* exec)
{
    // FIXME: implement this. https://bugs.webkit.org/show_bug.cgi?id=107830
    throwError(exec, createTypeError(exec, "Variable value mutation is not supported"));
    return jsUndefined();
}

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER)
