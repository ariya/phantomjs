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

#ifndef WorkerContextProxy_h
#define WorkerContextProxy_h

#if ENABLE(WORKERS)

#include "MessagePort.h"
#include <wtf/Forward.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

    class KURL;
    class Worker;
    class WorkerContextInspectorProxy;

    // A proxy to talk to the worker context.
    class WorkerContextProxy {
    public:
        static WorkerContextProxy* create(Worker*);

        virtual ~WorkerContextProxy() {}

        virtual void startWorkerContext(const KURL& scriptURL, const String& userAgent, const String& sourceCode) = 0;

        virtual void terminateWorkerContext() = 0;

        virtual void postMessageToWorkerContext(PassRefPtr<SerializedScriptValue>, PassOwnPtr<MessagePortChannelArray>) = 0;

        virtual bool hasPendingActivity() const = 0;

        virtual void workerObjectDestroyed() = 0;

#if ENABLE(INSPECTOR)
        virtual WorkerContextInspectorProxy* inspectorProxy() { return 0; }
#endif
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // WorkerContextProxy_h
