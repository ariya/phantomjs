/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef DedicatedWorkerContext_h
#define DedicatedWorkerContext_h

#if ENABLE(WORKERS)

#include "MessagePort.h"
#include "WorkerContext.h"

namespace WebCore {

    class DedicatedWorkerThread;

    class DedicatedWorkerContext : public WorkerContext {
    public:
        typedef WorkerContext Base;
        static PassRefPtr<DedicatedWorkerContext> create(const KURL& url, const String& userAgent, DedicatedWorkerThread* thread)
        {
            return adoptRef(new DedicatedWorkerContext(url, userAgent, thread));
        }

        virtual bool isDedicatedWorkerContext() const { return true; }

        // Overridden to allow us to check our pending activity after executing imported script.
        virtual void importScripts(const Vector<String>& urls, ExceptionCode&);

        // EventTarget
        virtual DedicatedWorkerContext* toDedicatedWorkerContext() { return this; }
        void postMessage(PassRefPtr<SerializedScriptValue>, ExceptionCode&);
        void postMessage(PassRefPtr<SerializedScriptValue>, const MessagePortArray*, ExceptionCode&);
        // FIXME: remove this when we update the ObjC bindings (bug #28774).
        void postMessage(PassRefPtr<SerializedScriptValue>, MessagePort*, ExceptionCode&);

        DEFINE_ATTRIBUTE_EVENT_LISTENER(message);

        DedicatedWorkerThread* thread();
    private:
        DedicatedWorkerContext(const KURL&, const String&, DedicatedWorkerThread*);
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // DedicatedWorkerContext_h
