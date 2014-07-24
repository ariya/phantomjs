/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "config.h"
#include "DOMWindowFileSystem.h"

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystem.h"
#include "DOMFileSystem.h"
#include "DOMWindow.h"
#include "Document.h"
#include "EntryCallback.h"
#include "ErrorCallback.h"
#include "FileError.h"
#include "FileSystemCallback.h"
#include "FileSystemCallbacks.h"
#include "FileSystemType.h"
#include "LocalFileSystem.h"
#include "SecurityOrigin.h"

namespace WebCore {

DOMWindowFileSystem::DOMWindowFileSystem()
{
}

DOMWindowFileSystem::~DOMWindowFileSystem()
{
}

void DOMWindowFileSystem::webkitRequestFileSystem(DOMWindow* window, int type, long long size, PassRefPtr<FileSystemCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    if (!window->isCurrentlyDisplayedInFrame())
        return;

    Document* document = window->document();
    if (!document)
        return;

    if (!AsyncFileSystem::isAvailable() || !document->securityOrigin()->canAccessFileSystem()) {
        DOMFileSystem::scheduleCallback(document, errorCallback, FileError::create(FileError::SECURITY_ERR));
        return;
    }

    FileSystemType fileSystemType = static_cast<FileSystemType>(type);
    if (!DOMFileSystemBase::isValidType(fileSystemType)) {
        DOMFileSystem::scheduleCallback(document, errorCallback, FileError::create(FileError::INVALID_MODIFICATION_ERR));
        return;
    }

    LocalFileSystem::localFileSystem().requestFileSystem(document, fileSystemType, size, FileSystemCallbacks::create(successCallback, errorCallback, document, fileSystemType), AsynchronousFileSystem);
}

void DOMWindowFileSystem::webkitResolveLocalFileSystemURL(DOMWindow* window, const String& url, PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    if (!window->isCurrentlyDisplayedInFrame())
        return;

    Document* document = window->document();
    if (!document)
        return;

    SecurityOrigin* securityOrigin = document->securityOrigin();
    KURL completedURL = document->completeURL(url);
    if (!AsyncFileSystem::isAvailable() || !securityOrigin->canAccessFileSystem() || !securityOrigin->canRequest(completedURL)) {
        DOMFileSystem::scheduleCallback(document, errorCallback, FileError::create(FileError::SECURITY_ERR));
        return;
    }

    FileSystemType type;
    String filePath;
    if (!completedURL.isValid() || !DOMFileSystemBase::crackFileSystemURL(completedURL, type, filePath)) {
        DOMFileSystem::scheduleCallback(document, errorCallback, FileError::create(FileError::ENCODING_ERR));
        return;
    }

    LocalFileSystem::localFileSystem().readFileSystem(document, type, ResolveURICallbacks::create(successCallback, errorCallback, document, type, filePath));
}

COMPILE_ASSERT(static_cast<int>(DOMWindowFileSystem::TEMPORARY) == static_cast<int>(FileSystemTypeTemporary), enum_mismatch);
COMPILE_ASSERT(static_cast<int>(DOMWindowFileSystem::PERSISTENT) == static_cast<int>(FileSystemTypePersistent), enum_mismatch);

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
