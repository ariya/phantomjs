/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef WorkerGlobalScopeFileSystem_h
#define WorkerGlobalScopeFileSystem_h

#if ENABLE(FILE_SYSTEM)

#include "DOMFileSystemSync.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

class EntryCallback;
class EntrySync;
class ErrorCallback;
class FileSystemCallback;
class WorkerGlobalScope;

class WorkerGlobalScopeFileSystem {
public:
    enum {
        TEMPORARY,
        PERSISTENT,
    };

    static void webkitRequestFileSystem(WorkerGlobalScope*, int type, long long size, PassRefPtr<FileSystemCallback> successCallback, PassRefPtr<ErrorCallback>);
    static PassRefPtr<DOMFileSystemSync> webkitRequestFileSystemSync(WorkerGlobalScope*, int type, long long size, ExceptionCode&);
    static void webkitResolveLocalFileSystemURL(WorkerGlobalScope*, const String& url, PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback>);
    static PassRefPtr<EntrySync> webkitResolveLocalFileSystemSyncURL(WorkerGlobalScope*, const String& url, ExceptionCode&);

private:
    WorkerGlobalScopeFileSystem();
    ~WorkerGlobalScopeFileSystem();
};

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)

#endif // WorkerGlobalScopeFileSystem_h
