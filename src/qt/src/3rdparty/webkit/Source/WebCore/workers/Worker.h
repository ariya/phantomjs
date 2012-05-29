/*
 * Copyright (C) 2008, 2010 Apple Inc. All Rights Reserved.
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

#ifndef Worker_h
#define Worker_h

#if ENABLE(WORKERS)

#include "AbstractWorker.h"
#include "ActiveDOMObject.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "MessagePort.h"
#include "WorkerScriptLoaderClient.h"
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

    class ScriptExecutionContext;
    class WorkerContextProxy;
    class WorkerScriptLoader;

    typedef int ExceptionCode;

    class Worker : public AbstractWorker, private WorkerScriptLoaderClient {
    public:
        static PassRefPtr<Worker> create(const String& url, ScriptExecutionContext*, ExceptionCode&);
        virtual ~Worker();

        virtual Worker* toWorker() { return this; }

        void postMessage(PassRefPtr<SerializedScriptValue> message, ExceptionCode&);
        void postMessage(PassRefPtr<SerializedScriptValue> message, const MessagePortArray*, ExceptionCode&);
        // FIXME: remove this when we update the ObjC bindings (bug #28774).
        void postMessage(PassRefPtr<SerializedScriptValue> message, MessagePort*, ExceptionCode&);

        void terminate();

        virtual bool canSuspend() const;
        virtual void stop();
        virtual bool hasPendingActivity() const;
    
        DEFINE_ATTRIBUTE_EVENT_LISTENER(message);

    private:
        Worker(ScriptExecutionContext*);

        virtual void notifyFinished();

        virtual void refEventTarget() { ref(); }
        virtual void derefEventTarget() { deref(); }

        OwnPtr<WorkerScriptLoader> m_scriptLoader;
        WorkerContextProxy* m_contextProxy; // The proxy outlives the worker to perform thread shutdown.
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // Worker_h
