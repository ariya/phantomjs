/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */
#include "config.h"
#if ENABLE(FILE_SYSTEM) && ENABLE(WORKERS)
#include "WorkerPlatformFileWriterClient.h"

#include "AsyncFileWriterBlackBerry.h"
#include "AsyncFileWriterClient.h"
#include "CrossThreadTask.h"
#include "FileError.h"
#include "WorkerGlobalScope.h"
#include "WorkerLoaderProxy.h"
#include "WorkerThread.h"

namespace WebCore {

template<> struct CrossThreadCopierBase<false, false, WorkerPlatformFileWriterClient*> : public CrossThreadCopierPassThrough<WorkerPlatformFileWriterClient*> {
};

void WorkerPlatformFileWriterClient::notifyWriteOnWorkerThread(ScriptExecutionContext*, WorkerPlatformFileWriterClient* platformClient, long long bytes, bool complete)
{
    platformClient->PlatformFileWriterClient::notifyWrite(bytes, complete);
}

void WorkerPlatformFileWriterClient::notifyFailOnWorkerThread(ScriptExecutionContext*, WorkerPlatformFileWriterClient* platformClient, BlackBerry::Platform::WebFileError error)
{
    platformClient->PlatformFileWriterClient::notifyFail(error);
}

void WorkerPlatformFileWriterClient::notifyTruncateOnWorkerThread(ScriptExecutionContext*, WorkerPlatformFileWriterClient* platformClient)
{
    platformClient->PlatformFileWriterClient::notifyTruncate();
}

void WorkerPlatformFileWriterClient::notifyWrite(long long bytes, bool complete)
{
    postTaskToWorkerThreadIfNeeded(createCallbackTask(&notifyWriteOnWorkerThread, this, bytes, complete));
}

void WorkerPlatformFileWriterClient::notifyTruncate()
{
    postTaskToWorkerThreadIfNeeded(createCallbackTask(&notifyTruncateOnWorkerThread, this));
}

void WorkerPlatformFileWriterClient::notifyFail(BlackBerry::Platform::WebFileError error)
{
    postTaskToWorkerThreadIfNeeded(createCallbackTask(&notifyFailOnWorkerThread, this, error));
}

void WorkerPlatformFileWriterClient::postTaskToWorkerThreadIfNeeded(PassOwnPtr<ScriptExecutionContext::Task> task)
{
    WTF::MutexLocker locker(m_mutex);
    if (!m_context)
        return;

    m_context->thread()->workerLoaderProxy().postTaskForModeToWorkerGlobalScope(task, m_mode);
}

}

#endif
