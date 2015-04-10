/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009, 2011 Google Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#include "config.h"
#include "WorkerGlobalScopeFileSystem.h"

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystem.h"
#include "DOMFileSystemBase.h"
#include "DOMFileSystemSync.h"
#include "DirectoryEntrySync.h"
#include "ErrorCallback.h"
#include "FileEntrySync.h"
#include "FileError.h"
#include "FileException.h"
#include "FileSystemCallback.h"
#include "FileSystemCallbacks.h"
#include "FileSystemType.h"
#include "LocalFileSystem.h"
#include "SecurityOrigin.h"
#include "SyncCallbackHelper.h"
#include "WorkerGlobalScope.h"

namespace WebCore {

void WorkerGlobalScopeFileSystem::webkitRequestFileSystem(WorkerGlobalScope* worker, int type, long long size, PassRefPtr<FileSystemCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    ScriptExecutionContext* secureContext = worker->scriptExecutionContext();
    if (!AsyncFileSystem::isAvailable() || !secureContext->securityOrigin()->canAccessFileSystem()) {
        DOMFileSystem::scheduleCallback(worker, errorCallback, FileError::create(FileError::SECURITY_ERR));
        return;
    }

    FileSystemType fileSystemType = static_cast<FileSystemType>(type);
    if (!DOMFileSystemBase::isValidType(fileSystemType)) {
        DOMFileSystem::scheduleCallback(worker, errorCallback, FileError::create(FileError::INVALID_MODIFICATION_ERR));
        return;
    }

    LocalFileSystem::localFileSystem().requestFileSystem(worker, fileSystemType, size, FileSystemCallbacks::create(successCallback, errorCallback, worker, fileSystemType), AsynchronousFileSystem);
}

PassRefPtr<DOMFileSystemSync> WorkerGlobalScopeFileSystem::webkitRequestFileSystemSync(WorkerGlobalScope* worker, int type, long long size, ExceptionCode& ec)
{
    ec = 0;
    ScriptExecutionContext* secureContext = worker->scriptExecutionContext();
    if (!AsyncFileSystem::isAvailable() || !secureContext->securityOrigin()->canAccessFileSystem()) {
        ec = FileException::SECURITY_ERR;
        return 0;
    }

    FileSystemType fileSystemType = static_cast<FileSystemType>(type);
    if (!DOMFileSystemBase::isValidType(fileSystemType)) {
        ec = FileException::INVALID_MODIFICATION_ERR;
        return 0;
    }

    FileSystemSyncCallbackHelper helper;
    LocalFileSystem::localFileSystem().requestFileSystem(worker, fileSystemType, size, FileSystemCallbacks::create(helper.successCallback(), helper.errorCallback(), worker, fileSystemType), SynchronousFileSystem);
    return helper.getResult(ec);
}

void WorkerGlobalScopeFileSystem::webkitResolveLocalFileSystemURL(WorkerGlobalScope* worker, const String& url, PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    KURL completedURL = worker->completeURL(url);
    ScriptExecutionContext* secureContext = worker->scriptExecutionContext();
    if (!AsyncFileSystem::isAvailable() || !secureContext->securityOrigin()->canAccessFileSystem() || !secureContext->securityOrigin()->canRequest(completedURL)) {
        DOMFileSystem::scheduleCallback(worker, errorCallback, FileError::create(FileError::SECURITY_ERR));
        return;
    }

    FileSystemType type;
    String filePath;
    if (!completedURL.isValid() || !DOMFileSystemBase::crackFileSystemURL(completedURL, type, filePath)) {
        DOMFileSystem::scheduleCallback(worker, errorCallback, FileError::create(FileError::ENCODING_ERR));
        return;
    }

    LocalFileSystem::localFileSystem().readFileSystem(worker, type, ResolveURICallbacks::create(successCallback, errorCallback, worker, type, filePath));
}

PassRefPtr<EntrySync> WorkerGlobalScopeFileSystem::webkitResolveLocalFileSystemSyncURL(WorkerGlobalScope* worker, const String& url, ExceptionCode& ec)
{
    ec = 0;
    KURL completedURL = worker->completeURL(url);
    ScriptExecutionContext* secureContext = worker->scriptExecutionContext();
    if (!AsyncFileSystem::isAvailable() || !secureContext->securityOrigin()->canAccessFileSystem() || !secureContext->securityOrigin()->canRequest(completedURL)) {
        ec = FileException::SECURITY_ERR;
        return 0;
    }

    FileSystemType type;
    String filePath;
    if (!completedURL.isValid() || !DOMFileSystemBase::crackFileSystemURL(completedURL, type, filePath)) {
        ec = FileException::ENCODING_ERR;
        return 0;
    }

    FileSystemSyncCallbackHelper readFileSystemHelper;
    LocalFileSystem::localFileSystem().readFileSystem(worker, type, FileSystemCallbacks::create(readFileSystemHelper.successCallback(), readFileSystemHelper.errorCallback(), worker, type), SynchronousFileSystem);
    RefPtr<DOMFileSystemSync> fileSystem = readFileSystemHelper.getResult(ec);
    if (!fileSystem)
        return 0;

    RefPtr<EntrySync> entry = fileSystem->root()->getDirectory(filePath, Dictionary(), ec);
    if (ec == FileException::TYPE_MISMATCH_ERR)
        return fileSystem->root()->getFile(filePath, Dictionary(), ec);

    return entry.release();
}

COMPILE_ASSERT(static_cast<int>(WorkerGlobalScopeFileSystem::TEMPORARY) == static_cast<int>(FileSystemTypeTemporary), enum_mismatch);
COMPILE_ASSERT(static_cast<int>(WorkerGlobalScopeFileSystem::PERSISTENT) == static_cast<int>(FileSystemTypePersistent), enum_mismatch);

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
