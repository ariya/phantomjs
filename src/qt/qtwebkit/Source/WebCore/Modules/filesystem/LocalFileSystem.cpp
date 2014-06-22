/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "LocalFileSystem.h"

#if ENABLE(FILE_SYSTEM)

#include "CrossThreadTask.h"
#include "DOMFileSystem.h"
#include "ErrorCallback.h"
#include "ExceptionCode.h"
#include "FileError.h"
#include "FileSystemCallback.h"
#include "FileSystemCallbacks.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include <wtf/MainThread.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

LocalFileSystem* LocalFileSystem::s_instance = 0;

void LocalFileSystem::initializeLocalFileSystem(const String& basePath)
{
    // FIXME: Should initialize the quota settings as well.
    ASSERT(isMainThread());
    ASSERT(!s_instance);
    if (s_instance)
        return;

    OwnPtr<LocalFileSystem> localFileSystem = adoptPtr(new LocalFileSystem(basePath));
    s_instance = localFileSystem.leakPtr();
}

LocalFileSystem& LocalFileSystem::localFileSystem()
{
    // initializeLocalFileSystem must be called prior calling this.
    ASSERT(s_instance);
    return *s_instance;
}

String LocalFileSystem::fileSystemBasePath() const
{
    return m_basePath;
}

static void openFileSystem(ScriptExecutionContext*, const String& basePath, const String& identifier, FileSystemType type, bool create, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    AsyncFileSystem::openFileSystem(basePath, identifier, type, create, callbacks);
}

static void performDeleteFileSystem(ScriptExecutionContext*, const String& basePath, const String& identifier, FileSystemType type, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    AsyncFileSystem::deleteFileSystem(basePath, identifier, type, callbacks);
}

void LocalFileSystem::readFileSystem(ScriptExecutionContext* context, FileSystemType type, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, FileSystemSynchronousType)
{
    // AsyncFileSystem::openFileSystem calls callbacks synchronously, so the method needs to be called asynchronously.
    context->postTask(createCallbackTask(&openFileSystem, fileSystemBasePath(), context->securityOrigin()->databaseIdentifier(), type, false, callbacks));
}

void LocalFileSystem::requestFileSystem(ScriptExecutionContext* context, FileSystemType type, long long, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, FileSystemSynchronousType)
{
    // AsyncFileSystem::openFileSystem calls callbacks synchronously, so the method needs to be called asynchronously.
    context->postTask(createCallbackTask(&openFileSystem, fileSystemBasePath(), context->securityOrigin()->databaseIdentifier(), type, true, callbacks));
}

void LocalFileSystem::deleteFileSystem(ScriptExecutionContext* context, FileSystemType type, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    // AsyncFileSystem::deleteFileSystem calls callbacks synchronously, so the method needs to be called asynchronously.
    context->postTask(createCallbackTask(&performDeleteFileSystem, fileSystemBasePath(), context->securityOrigin()->databaseIdentifier(), type, callbacks));
}

} // namespace

#endif // ENABLE(FILE_SYSTEM)
