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

#ifndef WorkerAsyncFileWriterBlackBerry_h
#define WorkerAsyncFileWriterBlackBerry_h

#if ENABLE(FILE_SYSTEM) && ENABLE(WORKERS)
#include "AsyncFileWriterBlackBerry.h"

#include <BlackBerryPlatformWebFileSystemFileWriter.h>
#include <wtf/Assertions.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ScriptExecutionContext;
class WorkerGlobalScope;

class WorkerAsyncFileWriterBlackBerry: public AsyncFileWriterBlackBerry {
public:
    static PassOwnPtr<WorkerAsyncFileWriterBlackBerry> create(WorkerGlobalScope* context, const String& mode, PassOwnPtr<BlackBerry::Platform::WebFileWriter> platformWriter, AsyncFileWriterClient* client)
    {
        return adoptPtr(new WorkerAsyncFileWriterBlackBerry(context, mode, platformWriter, client));
    }

    virtual void write(long long position, Blob*);
    virtual void truncate(long long length);
    virtual void abort();
    virtual bool waitForOperationToComplete();
    virtual PlatformFileWriterClient* platformWriterClient();

private:
    WorkerAsyncFileWriterBlackBerry(WorkerGlobalScope* context, const String& mode, PassOwnPtr<BlackBerry::Platform::WebFileWriter> platformWriter, AsyncFileWriterClient* client)
        : AsyncFileWriterBlackBerry(platformWriter, client)
        , m_context(context)
        , m_mode(mode)
    {
    }

    static void writeOnMainThread(ScriptExecutionContext*, BlackBerry::Platform::WebFileWriter* platformWriter, long long position, Blob*);
    static void truncateOnMainThread(ScriptExecutionContext*, BlackBerry::Platform::WebFileWriter* platformWriter, long long length);
    static void abortOnMainThread(ScriptExecutionContext*, BlackBerry::Platform::WebFileWriter* platformWriter);

    WorkerGlobalScope* m_context;
    String m_mode;
};

} // namespace WebCore
#endif
#endif
