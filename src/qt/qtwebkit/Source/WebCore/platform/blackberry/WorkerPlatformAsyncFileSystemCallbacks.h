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

#ifndef WorkerPlatformAsyncFileSystemCallbacks_h
#define WorkerPlatformAsyncFileSystemCallbacks_h

#if ENABLE(FILE_SYSTEM) && ENABLE(WORKERS)
#include "CrossThreadCopier.h"
#include "PlatformAsyncFileSystemCallbacks.h"
#include "ScriptExecutionContext.h"
#include "WorkerGlobalScope.h"

#include <BlackBerryPlatformWebFileSystemFileWriter.h>
#include <BlackBerryPlatformWebFileSystemPrimitives.h>
#include <wtf/Threading.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

template<> struct CrossThreadCopierBase<false, false, BlackBerry::Platform::WebFileSystem*> : public CrossThreadCopierPassThrough<BlackBerry::Platform::WebFileSystem*> {
};

template<> struct CrossThreadCopierBase<false, false, BlackBerry::Platform::WebFileWriter*> : public CrossThreadCopierPassThrough<BlackBerry::Platform::WebFileWriter*> {
};

class WorkerPlatformAsyncFileSystemCallbacks;
template<> struct CrossThreadCopierBase<false, false, WorkerPlatformAsyncFileSystemCallbacks*> : public CrossThreadCopierPassThrough<WorkerPlatformAsyncFileSystemCallbacks*> {
};

template<> struct CrossThreadCopierBase<false, false, Blob*> : public CrossThreadCopierPassThrough<Blob*> {
};

template<> struct CrossThreadCopierBase<false, false, WorkerGlobalScope*> : public CrossThreadCopierPassThrough<WorkerGlobalScope*> {
};

template<> struct CrossThreadCopierBase<false, false, AsyncFileWriterClient*> : public CrossThreadCopierPassThrough<AsyncFileWriterClient*> {
};

void postTaskToMainThread(PassOwnPtr<ScriptExecutionContext::Task>);

class KURL;

class WorkerPlatformAsyncFileSystemCallbacks: public PlatformAsyncFileSystemCallbacks, public WorkerGlobalScope::Observer {
public:
    WorkerPlatformAsyncFileSystemCallbacks(PassOwnPtr<AsyncFileSystemCallbacks> callbacks, WorkerGlobalScope* context, const String& mode, const KURL& rootURL = KURL())
        : PlatformAsyncFileSystemCallbacks(callbacks, rootURL)
        , WorkerGlobalScope::Observer(context)
        , m_context(context)
        , m_mode(mode)
    {
    }

    // For createWriter.
    WorkerPlatformAsyncFileSystemCallbacks(PassOwnPtr<AsyncFileSystemCallbacks> callbacks, AsyncFileWriterClient* client, WorkerGlobalScope* context, const String& mode)
        : PlatformAsyncFileSystemCallbacks(callbacks, client)
        , WorkerGlobalScope::Observer(context)
        , m_context(context)
        , m_mode(mode)
    {
    }

    void postTaskToWorkerThread(PassOwnPtr<ScriptExecutionContext::Task>);

    // From WorkerGlobalScope::Observer. This will be called on worker thread.
    virtual void notifyStop();

    virtual void notifyOpenFileSystem(BlackBerry::Platform::WebFileSystem* platformFileSystem);
    virtual void notifySucceed();
    virtual void notifyFail(BlackBerry::Platform::WebFileError);
    virtual void notifyReadMetadata(const BlackBerry::Platform::WebFileInfo&);
    virtual void notifyCreateSnapshotFileAndReadMetadata(const BlackBerry::Platform::WebFileInfo&);
    virtual void notifyReadDirectory(const std::vector<BlackBerry::Platform::WebFileSystemEntry>& entries, bool hasMore);
    virtual void notifyCreateFileWriter(BlackBerry::Platform::WebFileWriter* platformWriter, long long length);
    virtual PassOwnPtr<AsyncFileSystem> createAsyncFileSystem(PassOwnPtr<BlackBerry::Platform::WebFileSystem> platformFileSystem);
    virtual PassOwnPtr<AsyncFileWriter> createAsyncFileWriter(PassOwnPtr<BlackBerry::Platform::WebFileWriter> platformWriter);
    virtual void deleteMe();

private:
    ~WorkerPlatformAsyncFileSystemCallbacks() { }

    static void notifyOpenFileSystemOnWorkerThread(ScriptExecutionContext*, WorkerPlatformAsyncFileSystemCallbacks*, BlackBerry::Platform::WebFileSystem* platformFileSystem);
    static void notifySucceedOnWorkerThread(ScriptExecutionContext*, WorkerPlatformAsyncFileSystemCallbacks*);
    static void notifyFailOnWorkerThread(ScriptExecutionContext*, WorkerPlatformAsyncFileSystemCallbacks*, BlackBerry::Platform::WebFileError);
    static void notifyReadMetadataOnWorkerThread(ScriptExecutionContext*, WorkerPlatformAsyncFileSystemCallbacks*, const BlackBerry::Platform::WebFileInfo&);
    static void notifyCreateSnapshotFileAndReadMetadataOnWorkerThread(ScriptExecutionContext*, WorkerPlatformAsyncFileSystemCallbacks*, const BlackBerry::Platform::WebFileInfo&);
    static void notifyReadDirectoryEntryOnWorkerThread(ScriptExecutionContext*, WorkerPlatformAsyncFileSystemCallbacks*, const std::vector<BlackBerry::Platform::WebFileSystemEntry>& entries, bool hasMore);
    static void notifyCreateFileWriterOnWorkerThread(ScriptExecutionContext*, WorkerPlatformAsyncFileSystemCallbacks*, BlackBerry::Platform::WebFileWriter* platformWriter, long long length);

    WorkerGlobalScope* m_context;
    String m_mode;
    WTF::Mutex m_mutex;
};

} // namespace WebCore
#endif
#endif
