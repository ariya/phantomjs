/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#ifndef JSWorkerGlobalScopeBase_h
#define JSWorkerGlobalScopeBase_h

#if ENABLE(WORKERS)

#include "JSDOMGlobalObject.h"

namespace WebCore {

    class JSDedicatedWorkerGlobalScope;
    class JSSharedWorkerGlobalScope;
    class JSWorkerGlobalScope;
    class WorkerGlobalScope;

    class JSWorkerGlobalScopeBase : public JSDOMGlobalObject {
        typedef JSDOMGlobalObject Base;
    public:
        static void destroy(JSC::JSCell*);

        static const JSC::ClassInfo s_info;

        WorkerGlobalScope* impl() const { return m_impl.get(); }
        ScriptExecutionContext* scriptExecutionContext() const;

        static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
        {
            return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::GlobalObjectType, StructureFlags), &s_info);
        }

    protected:
        JSWorkerGlobalScopeBase(JSC::VM&, JSC::Structure*, PassRefPtr<WorkerGlobalScope>);
        void finishCreation(JSC::VM&);

    private:
        RefPtr<WorkerGlobalScope> m_impl;
    };

    // Returns a JSWorkerGlobalScope or jsNull()
    // Always ignores the execState and passed globalObject, WorkerGlobalScope is itself a globalObject and will always use its own prototype chain.
    JSC::JSValue toJS(JSC::ExecState*, JSDOMGlobalObject*, WorkerGlobalScope*);
    JSC::JSValue toJS(JSC::ExecState*, WorkerGlobalScope*);

    JSDedicatedWorkerGlobalScope* toJSDedicatedWorkerGlobalScope(JSC::JSValue);
    JSWorkerGlobalScope* toJSWorkerGlobalScope(JSC::JSValue);

#if ENABLE(SHARED_WORKERS)
    JSSharedWorkerGlobalScope* toJSSharedWorkerGlobalScope(JSC::JSValue);
#endif

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // JSWorkerGlobalScopeBase_h
