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

#ifndef WorkerPlatformFileWriterClient_h
#define WorkerPlatformFileWriterClient_h

#if ENABLE(FILE_SYSTEM) && ENABLE(WORKERS)
#include "PlatformFileWriterClient.h"
#include "ScriptExecutionContext.h"
#include "WorkerAsyncFileWriterBlackBerry.h"
#include "WorkerGlobalScope.h"

#include <wtf/PassOwnPtr.h>
#include <wtf/Threading.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class WorkerGlobalScope;

class WorkerPlatformFileWriterClient: public PlatformFileWriterClient, public WorkerGlobalScope::Observer {
public:
    static PassOwnPtr<WorkerPlatformFileWriterClient> create(WorkerGlobalScope* context, const String& mode, AsyncFileWriterClient* client, WorkerAsyncFileWriterBlackBerry* writer)
    {
        return adoptPtr(new WorkerPlatformFileWriterClient(context, mode, client, writer));
    }

    virtual void notifyWrite(long long bytes, bool complete);
    virtual void notifyTruncate();
    virtual void notifyFail(BlackBerry::Platform::WebFileError);

    // From WorkerGlobalScope::Observer
    virtual void notifyStop()
    {
        WTF::MutexLocker locker(m_mutex);
        m_context = 0;
    }

private:
    WorkerPlatformFileWriterClient(WorkerGlobalScope* context, const String& mode, AsyncFileWriterClient* client, WorkerAsyncFileWriterBlackBerry* writer)
        : PlatformFileWriterClient(client, writer)
        , WorkerGlobalScope::Observer(context)
        , m_context(context)
        , m_mode(mode)
    { }
    ~WorkerPlatformFileWriterClient() { }
    void postTaskToWorkerThreadIfNeeded(PassOwnPtr<ScriptExecutionContext::Task>);

    static void notifyWriteOnWorkerThread(ScriptExecutionContext*, WorkerPlatformFileWriterClient* platformClient, long long bytes, bool complete);
    static void notifyFailOnWorkerThread(ScriptExecutionContext*, WorkerPlatformFileWriterClient* platformClient, BlackBerry::Platform::WebFileError);
    static void notifyTruncateOnWorkerThread(ScriptExecutionContext*, WorkerPlatformFileWriterClient* platformClient);

    WorkerGlobalScope* m_context;
    String m_mode;
    WTF::Mutex m_mutex;
};

} // namespace WebCore

namespace WTF {

template <> inline void deleteOwnedPtr<WebCore::WorkerPlatformFileWriterClient>(WebCore::WorkerPlatformFileWriterClient* ptr)
{
    BlackBerry::Platform::deleteGuardedObject(ptr);
}

}

#endif
#endif
