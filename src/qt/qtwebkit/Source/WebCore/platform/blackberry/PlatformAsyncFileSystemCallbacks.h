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

#ifndef PlatformAsyncFileSystemCallbacks_h
#define PlatformAsyncFileSystemCallbacks_h

#if ENABLE(FILE_SYSTEM)
#include "AsyncFileSystemCallbacks.h"
#include "KURL.h"

#include <BlackBerryPlatformWebFileSystem.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

String platformFileSystemTypeToString(BlackBerry::Platform::WebFileSystem::Type);

class AsyncFileSystem;
class AsyncFileWriter;
class AsyncFileWriterClient;

class PlatformAsyncFileSystemCallbacks: public BlackBerry::Platform::WebFileSystemCallbacks {
public:
    PlatformAsyncFileSystemCallbacks(PassOwnPtr<AsyncFileSystemCallbacks> callbacks, const KURL& rootURL = KURL())
        : m_callbacks(callbacks)
        , m_writerClient(0)
        , m_rootURL(rootURL)
    {
    }

    // For createWriter.
    PlatformAsyncFileSystemCallbacks(PassOwnPtr<AsyncFileSystemCallbacks> callbacks, AsyncFileWriterClient* client)
        : m_callbacks(callbacks)
        , m_writerClient(client)
    {
    }

    virtual void notifyOpenFileSystem(BlackBerry::Platform::WebFileSystem* platformFileSystem);
    virtual void notifySucceed();
    virtual void notifyFail(BlackBerry::Platform::WebFileError);
    virtual void notifyReadMetadata(const BlackBerry::Platform::WebFileInfo&);
    virtual void notifyCreateSnapshotFileAndReadMetadata(const BlackBerry::Platform::WebFileInfo&);
    virtual void notifyReadDirectory(const std::vector<BlackBerry::Platform::WebFileSystemEntry>& entries, bool hasMore);
    virtual void notifyCreateFileWriter(BlackBerry::Platform::WebFileWriter* platformWriter, long long length);
    virtual void deleteMe();
    virtual PassOwnPtr<AsyncFileSystem> createAsyncFileSystem(PassOwnPtr<BlackBerry::Platform::WebFileSystem> platformFileSystem);
    virtual PassOwnPtr<AsyncFileWriter> createAsyncFileWriter(PassOwnPtr<BlackBerry::Platform::WebFileWriter> platformWriter);

protected:
    ~PlatformAsyncFileSystemCallbacks() { }
    OwnPtr<AsyncFileSystemCallbacks> m_callbacks;
    AsyncFileWriterClient* m_writerClient;

private:
    KURL m_rootURL;
};

} // namespace WebCore
#endif
#endif
