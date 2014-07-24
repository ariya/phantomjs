/*
 * Copyright (C) 2009, 2010 Google Inc. All rights reserved.
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

#if ENABLE(WORKERS)

#include "WorkerThreadableLoader.h"

#include "Document.h"
#include "DocumentThreadableLoader.h"
#include "CrossThreadTask.h"
#include "ResourceError.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "SecurityOrigin.h"
#include "ThreadableLoader.h"
#include "WorkerGlobalScope.h"
#include "WorkerLoaderProxy.h"
#include "WorkerThread.h"
#include <wtf/MainThread.h>
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>

using namespace std;

namespace WebCore {

static const char loadResourceSynchronouslyMode[] = "loadResourceSynchronouslyMode";

WorkerThreadableLoader::WorkerThreadableLoader(WorkerGlobalScope* workerGlobalScope, ThreadableLoaderClient* client, const String& taskMode, const ResourceRequest& request, const ThreadableLoaderOptions& options)
    : m_workerGlobalScope(workerGlobalScope)
    , m_workerClientWrapper(ThreadableLoaderClientWrapper::create(client))
    , m_bridge(*(new MainThreadBridge(m_workerClientWrapper, m_workerGlobalScope->thread()->workerLoaderProxy(), taskMode, request, options, workerGlobalScope->url().strippedForUseAsReferrer())))
{
}

WorkerThreadableLoader::~WorkerThreadableLoader()
{
    m_bridge.destroy();
}

void WorkerThreadableLoader::loadResourceSynchronously(WorkerGlobalScope* workerGlobalScope, const ResourceRequest& request, ThreadableLoaderClient& client, const ThreadableLoaderOptions& options)
{
    WorkerRunLoop& runLoop = workerGlobalScope->thread()->runLoop();

    // Create a unique mode just for this synchronous resource load.
    String mode = loadResourceSynchronouslyMode;
    mode.append(String::number(runLoop.createUniqueId()));

    RefPtr<WorkerThreadableLoader> loader = WorkerThreadableLoader::create(workerGlobalScope, &client, mode, request, options);
    MessageQueueWaitResult result = MessageQueueMessageReceived;
    while (!loader->done() && result != MessageQueueTerminated)
        result = runLoop.runInMode(workerGlobalScope, mode);

    if (!loader->done() && result == MessageQueueTerminated)
        loader->cancel();
}

void WorkerThreadableLoader::cancel()
{
    m_bridge.cancel();
}

WorkerThreadableLoader::MainThreadBridge::MainThreadBridge(PassRefPtr<ThreadableLoaderClientWrapper> workerClientWrapper, WorkerLoaderProxy& loaderProxy, const String& taskMode,
                                                           const ResourceRequest& request, const ThreadableLoaderOptions& options, const String& outgoingReferrer)
    : m_workerClientWrapper(workerClientWrapper)
    , m_loaderProxy(loaderProxy)
    , m_taskMode(taskMode.isolatedCopy())
{
    ASSERT(m_workerClientWrapper.get());
    m_loaderProxy.postTaskToLoader(
        createCallbackTask(&MainThreadBridge::mainThreadCreateLoader, 
                           AllowCrossThreadAccess(this), request, options, outgoingReferrer));
}

WorkerThreadableLoader::MainThreadBridge::~MainThreadBridge()
{
}

void WorkerThreadableLoader::MainThreadBridge::mainThreadCreateLoader(ScriptExecutionContext* context, MainThreadBridge* thisPtr, PassOwnPtr<CrossThreadResourceRequestData> requestData, ThreadableLoaderOptions options, const String& outgoingReferrer)
{
    ASSERT(isMainThread());
    Document* document = toDocument(context);

    OwnPtr<ResourceRequest> request(ResourceRequest::adopt(requestData));
    request->setHTTPReferrer(outgoingReferrer);
    // FIXME: If the a site requests a local resource, then this will return a non-zero value but the sync path
    // will return a 0 value.  Either this should return 0 or the other code path should do a callback with
    // a failure.
    thisPtr->m_mainThreadLoader = DocumentThreadableLoader::create(document, thisPtr, *request, options);
    ASSERT(thisPtr->m_mainThreadLoader);
}

void WorkerThreadableLoader::MainThreadBridge::mainThreadDestroy(ScriptExecutionContext* context, MainThreadBridge* thisPtr)
{
    ASSERT(isMainThread());
    ASSERT_UNUSED(context, context->isDocument());
    delete thisPtr;
}

void WorkerThreadableLoader::MainThreadBridge::destroy()
{
    // Ensure that no more client callbacks are done in the worker context's thread.
    clearClientWrapper();

    // "delete this" and m_mainThreadLoader::deref() on the worker object's thread.
    m_loaderProxy.postTaskToLoader(
        createCallbackTask(&MainThreadBridge::mainThreadDestroy, AllowCrossThreadAccess(this)));
}

void WorkerThreadableLoader::MainThreadBridge::mainThreadCancel(ScriptExecutionContext* context, MainThreadBridge* thisPtr)
{
    ASSERT(isMainThread());
    ASSERT_UNUSED(context, context->isDocument());

    if (!thisPtr->m_mainThreadLoader)
        return;
    thisPtr->m_mainThreadLoader->cancel();
    thisPtr->m_mainThreadLoader = 0;
}

void WorkerThreadableLoader::MainThreadBridge::cancel()
{
    m_loaderProxy.postTaskToLoader(
        createCallbackTask(&MainThreadBridge::mainThreadCancel, AllowCrossThreadAccess(this)));
    ThreadableLoaderClientWrapper* clientWrapper = m_workerClientWrapper.get();
    if (!clientWrapper->done()) {
        // If the client hasn't reached a termination state, then transition it by sending a cancellation error.
        // Note: no more client callbacks will be done after this method -- the clearClientWrapper() call ensures that.
        ResourceError error(String(), 0, String(), String());
        error.setIsCancellation(true);
        clientWrapper->didFail(error);
    }
    clearClientWrapper();
}

void WorkerThreadableLoader::MainThreadBridge::clearClientWrapper()
{
    m_workerClientWrapper->clearClient();
}

static void workerGlobalScopeDidSendData(ScriptExecutionContext* context, RefPtr<ThreadableLoaderClientWrapper> workerClientWrapper, unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    ASSERT_UNUSED(context, context->isWorkerGlobalScope());
    workerClientWrapper->didSendData(bytesSent, totalBytesToBeSent);
}

void WorkerThreadableLoader::MainThreadBridge::didSendData(unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(createCallbackTask(&workerGlobalScopeDidSendData, m_workerClientWrapper, bytesSent, totalBytesToBeSent), m_taskMode);
}

static void workerGlobalScopeDidReceiveResponse(ScriptExecutionContext* context, RefPtr<ThreadableLoaderClientWrapper> workerClientWrapper, unsigned long identifier, PassOwnPtr<CrossThreadResourceResponseData> responseData)
{
    ASSERT_UNUSED(context, context->isWorkerGlobalScope());
    OwnPtr<ResourceResponse> response(ResourceResponse::adopt(responseData));
    workerClientWrapper->didReceiveResponse(identifier, *response);
}

void WorkerThreadableLoader::MainThreadBridge::didReceiveResponse(unsigned long identifier, const ResourceResponse& response)
{
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(createCallbackTask(&workerGlobalScopeDidReceiveResponse, m_workerClientWrapper, identifier, response), m_taskMode);
}

static void workerGlobalScopeDidReceiveData(ScriptExecutionContext* context, RefPtr<ThreadableLoaderClientWrapper> workerClientWrapper, PassOwnPtr<Vector<char> > vectorData)
{
    ASSERT_UNUSED(context, context->isWorkerGlobalScope());
    workerClientWrapper->didReceiveData(vectorData->data(), vectorData->size());
}

void WorkerThreadableLoader::MainThreadBridge::didReceiveData(const char* data, int dataLength)
{
    OwnPtr<Vector<char> > vector = adoptPtr(new Vector<char>(dataLength)); // needs to be an OwnPtr for usage with createCallbackTask.
    memcpy(vector->data(), data, dataLength);
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(createCallbackTask(&workerGlobalScopeDidReceiveData, m_workerClientWrapper, vector.release()), m_taskMode);
}

static void workerGlobalScopeDidFinishLoading(ScriptExecutionContext* context, RefPtr<ThreadableLoaderClientWrapper> workerClientWrapper, unsigned long identifier, double finishTime)
{
    ASSERT_UNUSED(context, context->isWorkerGlobalScope());
    workerClientWrapper->didFinishLoading(identifier, finishTime);
}

void WorkerThreadableLoader::MainThreadBridge::didFinishLoading(unsigned long identifier, double finishTime)
{
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(createCallbackTask(&workerGlobalScopeDidFinishLoading, m_workerClientWrapper, identifier, finishTime), m_taskMode);
}

static void workerGlobalScopeDidFail(ScriptExecutionContext* context, RefPtr<ThreadableLoaderClientWrapper> workerClientWrapper, const ResourceError& error)
{
    ASSERT_UNUSED(context, context->isWorkerGlobalScope());
    workerClientWrapper->didFail(error);
}

void WorkerThreadableLoader::MainThreadBridge::didFail(const ResourceError& error)
{
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(createCallbackTask(&workerGlobalScopeDidFail, m_workerClientWrapper, error), m_taskMode);
}

static void workerGlobalScopeDidFailAccessControlCheck(ScriptExecutionContext* context, PassRefPtr<ThreadableLoaderClientWrapper> workerClientWrapper, const ResourceError& error)
{
    ASSERT_UNUSED(context, context->isWorkerGlobalScope());
    workerClientWrapper->didFailAccessControlCheck(error);
}

void WorkerThreadableLoader::MainThreadBridge::didFailAccessControlCheck(const ResourceError& error)
{
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(createCallbackTask(&workerGlobalScopeDidFailAccessControlCheck, m_workerClientWrapper, error), m_taskMode);
}

static void workerGlobalScopeDidFailRedirectCheck(ScriptExecutionContext* context, RefPtr<ThreadableLoaderClientWrapper> workerClientWrapper)
{
    ASSERT_UNUSED(context, context->isWorkerGlobalScope());
    workerClientWrapper->didFailRedirectCheck();
}

void WorkerThreadableLoader::MainThreadBridge::didFailRedirectCheck()
{
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(createCallbackTask(&workerGlobalScopeDidFailRedirectCheck, m_workerClientWrapper), m_taskMode);
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
