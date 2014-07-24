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
#if ENABLE(FILE_SYSTEM)
#include "LocalFileSystem.h"

#include "AsyncFileSystemBlackBerry.h"
#include "AsyncFileSystemCallbacks.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "CrossThreadTask.h"
#include "DOMFileSystem.h"
#include "Document.h"
#include "ErrorCallback.h"
#include "ExceptionCode.h"
#include "FileError.h"
#include "FileSystemCallback.h"
#include "FileSystemCallbacks.h"
#include "FileSystemType.h"
#include "KURL.h"
#include "Page.h"
#include "PageClientBlackBerry.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "WorkerAsyncFileSystemBlackBerry.h"
#include "WorkerGlobalScope.h"
#include "WorkerThread.h"

#include <wtf/MainThread.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

#if ENABLE(WORKERS)
static const char openFileSystemMode[] = "openFileSystemMode";
static const char deleteFileSystemMode[] = "deleteFileSystemMode";
#endif

static const char* fileSystemTypeString(FileSystemType type)
{
    switch (type) {
    case FileSystemTypePersistent:
        return DOMFileSystemBase::persistentPathPrefix;
    case FileSystemTypeTemporary:
        return DOMFileSystemBase::temporaryPathPrefix;
    case FileSystemTypeIsolated:
        return DOMFileSystemBase::isolatedPathPrefix;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

static void openFileSystem(ScriptExecutionContext* context, const String& basePath, FileSystemType type, long long size, bool create, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, FileSystemSynchronousType synchronousType)
{
    ASSERT(context);
    ASSERT(type != FileSystemTypeIsolated);
    if (type == FileSystemTypeIsolated)
        return;

    KURL url = context->url();
    if (url.hasFragmentIdentifier())
        url.removeFragmentIdentifier();
    url.setQuery(String());
    url.setPath("/");
    StringBuilder builder;
    builder.append("filesystem:");
    builder.append(url.string());
    builder.append(fileSystemTypeString(type));
    KURL rootURL = context->completeURL(builder.toString());
    ASSERT(rootURL.isValid());

    // TODO: Ask user for file system permission.

    if (context->isDocument()) {
        int playerId = 0;
        Page* page = static_cast<Document*>(context)->page();
        if (page)
            playerId = page->chrome().client()->platformPageClient()->playerID();
        AsyncFileSystemBlackBerry::openFileSystem(rootURL, basePath, context->securityOrigin()->databaseIdentifier(), type, size, create, playerId, callbacks);
    } else {
#if ENABLE(WORKERS)
        WorkerGlobalScope* workerGlobalScope = static_cast<WorkerGlobalScope*>(context);
        String mode = openFileSystemMode;
        mode.append(String::number(workerGlobalScope->thread()->runLoop().createUniqueId()));
        WorkerAsyncFileSystemBlackBerry::openFileSystem(workerGlobalScope, rootURL, mode, basePath, context->securityOrigin()->databaseIdentifier(), type, size, create, callbacks);
        if (synchronousType == SynchronousFileSystem)
            workerGlobalScope->thread()->runLoop().runInMode(workerGlobalScope, mode);
#else
        ASSERT_NOT_REACHED();
#endif
    }
}

void LocalFileSystem::deleteFileSystem(ScriptExecutionContext* context, FileSystemType type, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    ASSERT(context);
    ASSERT(type != FileSystemTypeIsolated);
    if (type == FileSystemTypeIsolated)
        return;

    // TODO: Ask user for file system permission.

    if (context->isDocument())
        AsyncFileSystemBlackBerry::deleteFileSystem(fileSystemBasePath(), context->securityOrigin()->databaseIdentifier(), type, callbacks);
    else {
#if ENABLE(WORKERS)
        WorkerGlobalScope* workerGlobalScope = static_cast<WorkerGlobalScope*>(context);
        String mode = deleteFileSystemMode;
        mode.append(String::number(workerGlobalScope->thread()->runLoop().createUniqueId()));
        WorkerAsyncFileSystemBlackBerry::deleteFileSystem(workerGlobalScope, mode, fileSystemBasePath(), context->securityOrigin()->databaseIdentifier(), type, callbacks);
#else
        ASSERT_NOT_REACHED();
#endif
    }
}


void LocalFileSystem::readFileSystem(ScriptExecutionContext* context, FileSystemType type, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, FileSystemSynchronousType synchronousType)
{
    openFileSystem(context, fileSystemBasePath(), type, 0, false, callbacks, synchronousType);
}

void LocalFileSystem::requestFileSystem(ScriptExecutionContext* context, FileSystemType type, long long size, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, FileSystemSynchronousType synchronousType)
{
    openFileSystem(context, fileSystemBasePath(), type, size, true, callbacks, synchronousType);
}

} // namespace WebCore

#endif
