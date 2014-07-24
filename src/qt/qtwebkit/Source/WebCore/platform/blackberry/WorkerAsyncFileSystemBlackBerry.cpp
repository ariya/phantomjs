/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
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
#include "WorkerAsyncFileSystemBlackBerry.h"

#include "AsyncFileSystemCallbacks.h"
#include "AsyncFileWriterClient.h"
#include "CrossThreadTask.h"
#include "WorkerAsyncFileWriterBlackBerry.h"
#include "WorkerGlobalScope.h"
#include "WorkerLoaderProxy.h"
#include "WorkerPlatformAsyncFileSystemCallbacks.h"
#include "WorkerThread.h"

#include <BlackBerryPlatformMessageClient.h>
#include <wtf/MainThread.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

using BlackBerry::Platform::WebFileSystem;
using BlackBerry::Platform::WebFileWriter;
using BlackBerry::Platform::webKitThreadMessageClient;

namespace WebCore {

WorkerAsyncFileSystemBlackBerry::WorkerAsyncFileSystemBlackBerry(WorkerGlobalScope* context, const String& mode, PassOwnPtr<WebFileSystem> platformFileSystem)
    : AsyncFileSystemBlackBerry(platformFileSystem)
    , m_context(context)
    , m_mode(mode)
{
}

WorkerAsyncFileSystemBlackBerry::~WorkerAsyncFileSystemBlackBerry()
{
}

bool WorkerAsyncFileSystemBlackBerry::waitForOperationToComplete()
{
    if (m_context->thread()->runLoop().runInMode(m_context, m_mode) == MessageQueueTerminated)
        return false;
    return true;
}

void WorkerAsyncFileSystemBlackBerry::openFileSystemOnMainThread(ScriptExecutionContext*, const String& basePath, const String& storageIdentifier, FileSystemType type, long long size, bool create, WorkerPlatformAsyncFileSystemCallbacks* callbacks)
{
    WebFileSystem::openFileSystem(webKitThreadMessageClient(), basePath, storageIdentifier, static_cast<WebFileSystem::Type>(type), size, create, callbacks, 0);
}

void WorkerAsyncFileSystemBlackBerry::deleteFileSystemOnMainThread(ScriptExecutionContext*, const String& basePath, const String& storageIdentifier, FileSystemType type, WorkerPlatformAsyncFileSystemCallbacks* callbacks)
{
    WebFileSystem::deleteFileSystem(webKitThreadMessageClient(), basePath, storageIdentifier, static_cast<WebFileSystem::Type>(type), callbacks);
}

void WorkerAsyncFileSystemBlackBerry::moveOnMainThread(ScriptExecutionContext*, WebFileSystem* platformFileSystem, const KURL& sourcePath, const KURL& destinationPath, WorkerPlatformAsyncFileSystemCallbacks* callbacks)
{
    platformFileSystem->move(fileSystemURLToPath(sourcePath), fileSystemURLToPath(destinationPath), callbacks);
}

void WorkerAsyncFileSystemBlackBerry::copyOnMainThread(ScriptExecutionContext*, WebFileSystem* platformFileSystem, const KURL& sourcePath, const KURL& destinationPath, WorkerPlatformAsyncFileSystemCallbacks* callbacks)
{
    platformFileSystem->copy(fileSystemURLToPath(sourcePath), fileSystemURLToPath(destinationPath), callbacks);
}

void WorkerAsyncFileSystemBlackBerry::removeOnMainThread(ScriptExecutionContext*, WebFileSystem* platformFileSystem, const KURL& path, WorkerPlatformAsyncFileSystemCallbacks* callbacks)
{
    platformFileSystem->remove(fileSystemURLToPath(path), callbacks);
}

void WorkerAsyncFileSystemBlackBerry::removeRecursivelyOnMainThread(ScriptExecutionContext*, WebFileSystem* platformFileSystem, const KURL& path, WorkerPlatformAsyncFileSystemCallbacks* callbacks)
{
    platformFileSystem->removeRecursively(fileSystemURLToPath(path), callbacks);
}

void WorkerAsyncFileSystemBlackBerry::readMetadataOnMainThread(ScriptExecutionContext*, WebFileSystem* platformFileSystem, const KURL& path, WorkerPlatformAsyncFileSystemCallbacks* callbacks)
{
    platformFileSystem->readMetadata(fileSystemURLToPath(path), callbacks);
}

void WorkerAsyncFileSystemBlackBerry::createFileOnMainThread(ScriptExecutionContext*, WebFileSystem* platformFileSystem, const KURL& path, bool exclusive, WorkerPlatformAsyncFileSystemCallbacks* callbacks)
{
    platformFileSystem->createFile(fileSystemURLToPath(path), exclusive, callbacks);
}

void WorkerAsyncFileSystemBlackBerry::createDirectoryOnMainThread(ScriptExecutionContext*, WebFileSystem* platformFileSystem, const KURL& path, bool exclusive, WorkerPlatformAsyncFileSystemCallbacks* callbacks)
{
    platformFileSystem->createDirectory(fileSystemURLToPath(path), exclusive, callbacks);
}

void WorkerAsyncFileSystemBlackBerry::fileExistsOnMainThread(ScriptExecutionContext*, WebFileSystem* platformFileSystem, const KURL& path, WorkerPlatformAsyncFileSystemCallbacks* callbacks)
{
    platformFileSystem->fileExists(fileSystemURLToPath(path), callbacks);
}

void WorkerAsyncFileSystemBlackBerry::directoryExistsOnMainThread(ScriptExecutionContext*, WebFileSystem* platformFileSystem, const KURL& path, WorkerPlatformAsyncFileSystemCallbacks* callbacks)
{
    platformFileSystem->directoryExists(fileSystemURLToPath(path), callbacks);
}

void WorkerAsyncFileSystemBlackBerry::readDirectoryOnMainThread(ScriptExecutionContext*, WebFileSystem* platformFileSystem, const KURL& path, WorkerPlatformAsyncFileSystemCallbacks* callbacks)
{
    platformFileSystem->readDirectory(fileSystemURLToPath(path), callbacks);
}

void WorkerAsyncFileSystemBlackBerry::createWriterOnMainThread(ScriptExecutionContext*, WebFileSystem* platformFileSystem, AsyncFileWriterClient*, const KURL& path, WorkerPlatformAsyncFileSystemCallbacks* callbacks)
{
    WebFileWriter::createWriter(platformFileSystem, fileSystemURLToPath(path), callbacks);
}

void WorkerAsyncFileSystemBlackBerry::createSnapshotFileAndReadMetadataOnMainThread(ScriptExecutionContext*, WebFileSystem* platformFileSystem, const KURL& path, WorkerPlatformAsyncFileSystemCallbacks* callbacks)
{
    platformFileSystem->createSnapshotFileAndReadMetadata(fileSystemURLToPath(path), callbacks);
}

void WorkerAsyncFileSystemBlackBerry::openFileSystem(WorkerGlobalScope* context, const KURL& rootURL, const String& mode, const String& basePath, const String& storageIdentifier, FileSystemType type, long long size, bool create, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    ASSERT(context);
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileSystemBlackBerry::openFileSystemOnMainThread, basePath, storageIdentifier, type, size, create, new WorkerPlatformAsyncFileSystemCallbacks(callbacks, context, mode, rootURL)));
}

void WorkerAsyncFileSystemBlackBerry::deleteFileSystem(WorkerGlobalScope* context, const String& mode, const String& basePath, const String& storageIdentifier, FileSystemType type, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    ASSERT(context);
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileSystemBlackBerry::deleteFileSystemOnMainThread, basePath, storageIdentifier, type, new WorkerPlatformAsyncFileSystemCallbacks(callbacks, context, mode)));
}

void WorkerAsyncFileSystemBlackBerry::move(const KURL& sourcePath, const KURL& destinationPath, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileSystemBlackBerry::moveOnMainThread, m_platformFileSystem.get(), sourcePath, destinationPath, new WorkerPlatformAsyncFileSystemCallbacks(callbacks, m_context, m_mode)));
}

void WorkerAsyncFileSystemBlackBerry::copy(const KURL& sourcePath, const KURL& destinationPath, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileSystemBlackBerry::copyOnMainThread, m_platformFileSystem.get(), sourcePath, destinationPath, new WorkerPlatformAsyncFileSystemCallbacks(callbacks, m_context, m_mode)));
}

void WorkerAsyncFileSystemBlackBerry::remove(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileSystemBlackBerry::removeOnMainThread, m_platformFileSystem.get(), path, new WorkerPlatformAsyncFileSystemCallbacks(callbacks, m_context, m_mode)));
}

void WorkerAsyncFileSystemBlackBerry::removeRecursively(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileSystemBlackBerry::removeRecursivelyOnMainThread, m_platformFileSystem.get(), path, new WorkerPlatformAsyncFileSystemCallbacks(callbacks, m_context, m_mode)));
}

void WorkerAsyncFileSystemBlackBerry::readMetadata(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileSystemBlackBerry::readMetadataOnMainThread, m_platformFileSystem.get(), path, new WorkerPlatformAsyncFileSystemCallbacks(callbacks, m_context, m_mode)));
}

void WorkerAsyncFileSystemBlackBerry::createFile(const KURL& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileSystemBlackBerry::createFileOnMainThread, m_platformFileSystem.get(), path, exclusive, new WorkerPlatformAsyncFileSystemCallbacks(callbacks, m_context, m_mode)));
}

void WorkerAsyncFileSystemBlackBerry::createDirectory(const KURL& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileSystemBlackBerry::createDirectoryOnMainThread, m_platformFileSystem.get(), path, exclusive, new WorkerPlatformAsyncFileSystemCallbacks(callbacks, m_context, m_mode)));
}

void WorkerAsyncFileSystemBlackBerry::fileExists(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileSystemBlackBerry::fileExistsOnMainThread, m_platformFileSystem.get(), path, new WorkerPlatformAsyncFileSystemCallbacks(callbacks, m_context, m_mode)));
}

void WorkerAsyncFileSystemBlackBerry::directoryExists(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileSystemBlackBerry::directoryExistsOnMainThread, m_platformFileSystem.get(), path, new WorkerPlatformAsyncFileSystemCallbacks(callbacks, m_context, m_mode)));
}

void WorkerAsyncFileSystemBlackBerry::readDirectory(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileSystemBlackBerry::readDirectoryOnMainThread, m_platformFileSystem.get(), path, new WorkerPlatformAsyncFileSystemCallbacks(callbacks, m_context, m_mode)));
}

void WorkerAsyncFileSystemBlackBerry::createWriter(AsyncFileWriterClient* client, const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileSystemBlackBerry::createWriterOnMainThread, m_platformFileSystem.get(), client, path, new WorkerPlatformAsyncFileSystemCallbacks(callbacks, client, m_context, m_mode)));
}

void WorkerAsyncFileSystemBlackBerry::createSnapshotFileAndReadMetadata(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    postTaskToMainThread(createCallbackTask(&WorkerAsyncFileSystemBlackBerry::createSnapshotFileAndReadMetadataOnMainThread, m_platformFileSystem.get(), path, new WorkerPlatformAsyncFileSystemCallbacks(callbacks, m_context, m_mode)));
}

} // namespace WebCore
#endif // ENABLE(FILE_SYSTEM) && ENABLE(WORKERS)
