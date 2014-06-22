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
#include "WorkerAsyncFileWriterBlackBerry.h"

#include "CrossThreadCopier.h"
#include "CrossThreadTask.h"
#include "PlatformBlob.h"
#include "WorkerPlatformAsyncFileSystemCallbacks.h"
#include "WorkerPlatformFileWriterClient.h"
#include "WorkerThread.h"

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/CString.h>

namespace WebCore {

void WorkerAsyncFileWriterBlackBerry::writeOnMainThread(ScriptExecutionContext*, BlackBerry::Platform::WebFileWriter* platformWriter, long long position, Blob* blob)
{
    platformWriter->write(position, new PlatformBlob(blob));
}

void WorkerAsyncFileWriterBlackBerry::truncateOnMainThread(ScriptExecutionContext*, BlackBerry::Platform::WebFileWriter* platformWriter, long long length)
{
    platformWriter->truncate(length);
}

void WorkerAsyncFileWriterBlackBerry::abortOnMainThread(ScriptExecutionContext*, BlackBerry::Platform::WebFileWriter* platformWriter)
{
    platformWriter->abort();
}

void WorkerAsyncFileWriterBlackBerry::write(long long position, Blob* blob)
{
    beginWriteOrTruncate();
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileWriterBlackBerry::writeOnMainThread, m_platformWriter.get(), position, blob));
}

void WorkerAsyncFileWriterBlackBerry::truncate(long long length)
{
    beginWriteOrTruncate();
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileWriterBlackBerry::truncateOnMainThread, m_platformWriter.get(), length));
}

void WorkerAsyncFileWriterBlackBerry::abort()
{
    beginAbort();
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileWriterBlackBerry::abortOnMainThread, m_platformWriter.get()));
}

bool WorkerAsyncFileWriterBlackBerry::waitForOperationToComplete()
{
    while (m_isOperationInProgress) {
        if (m_context->thread()->runLoop().runInMode(m_context, m_mode) == MessageQueueTerminated)
            return false;
    }
    return true;
}

PlatformFileWriterClient* WorkerAsyncFileWriterBlackBerry::platformWriterClient()
{
    if (!m_platformWriterClient)
        m_platformWriterClient = WorkerPlatformFileWriterClient::create(m_context, m_mode, m_client, this);
    return m_platformWriterClient.get();
}

} // namespace WebCore
#endif
