/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JSCallbackData_h
#define JSCallbackData_h

#include "JSDOMBinding.h"
#include "JSDOMGlobalObject.h"
#include "ScriptExecutionContext.h"
#include <heap/Strong.h>
#include <runtime/JSObject.h>
#include <wtf/Threading.h>

namespace WebCore {

// We have to clean up this data on the context thread because unprotecting a
// JSObject on the wrong thread without synchronization would corrupt the heap
// (and synchronization would be slow).

class JSCallbackData {
public:
    static void deleteData(void*);

    JSCallbackData(JSC::JSObject* callback, JSDOMGlobalObject* globalObject)
        : m_callback(globalObject->globalData(), callback)
        , m_globalObject(globalObject->globalData(), globalObject)
#ifndef NDEBUG
        , m_thread(currentThread())
#endif
    {
    }
    
    ~JSCallbackData()
    {
        ASSERT(m_thread == currentThread());
    }

    JSC::JSObject* callback() { return m_callback.get(); }
    JSDOMGlobalObject* globalObject() { return m_globalObject.get(); }
    
    JSC::JSValue invokeCallback(JSC::MarkedArgumentBuffer&, bool* raisedException = 0);

private:
    JSC::Strong<JSC::JSObject> m_callback;
    JSC::Strong<JSDOMGlobalObject> m_globalObject;
#ifndef NDEBUG
    ThreadIdentifier m_thread;
#endif
};

class DeleteCallbackDataTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<DeleteCallbackDataTask> create(JSCallbackData* data)
    {
        return adoptPtr(new DeleteCallbackDataTask(data));
    }

    virtual void performTask(ScriptExecutionContext*)
    {
        delete m_data;
    }
    virtual bool isCleanupTask() const { return true; }
private:

    DeleteCallbackDataTask(JSCallbackData* data) : m_data(data) {}

    JSCallbackData* m_data;
};

} // namespace WebCore

#endif // JSCallbackData_h
