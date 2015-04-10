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

#ifndef WorkerLoaderProxy_h
#define WorkerLoaderProxy_h

#if ENABLE(WORKERS)

#include "ScriptExecutionContext.h"
#include <wtf/Forward.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

    // A proxy to talk to the loader context. Normally, the document on the main thread
    // provides loading services for the subordinate workers. This interface provides 2-way
    // communications to the Document context and back to the worker.
    // Note that in multi-process browsers, the Worker object context and the Document
    // context can be distinct.
    class WorkerLoaderProxy {
    public:
        virtual ~WorkerLoaderProxy() { }

        // Posts a task to the thread which runs the loading code (normally, the main thread).
        virtual void postTaskToLoader(PassOwnPtr<ScriptExecutionContext::Task>) = 0;

        // Posts callbacks from loading code to the WorkerGlobalScope. The 'mode' is used to differentiate
        // specific synchronous loading requests so they can be 'nested', per spec.
        // Returns true if the task was posted successfully.
        virtual bool postTaskForModeToWorkerGlobalScope(PassOwnPtr<ScriptExecutionContext::Task>, const String& mode) = 0;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // WorkerLoaderProxy_h
