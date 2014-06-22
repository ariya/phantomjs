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

#ifndef DOMFileSystemSync_h
#define DOMFileSystemSync_h

#if ENABLE(FILE_SYSTEM)

#include "DOMFileSystemBase.h"

namespace WebCore {

class DirectoryEntrySync;
class File;
class FileEntrySync;
class FileWriterSync;

typedef int ExceptionCode;

class DOMFileSystemSync : public DOMFileSystemBase {
public:
    static PassRefPtr<DOMFileSystemSync> create(ScriptExecutionContext* context, const String& name, FileSystemType type, const KURL& rootURL, PassOwnPtr<AsyncFileSystem> asyncFileSystem)
    {
        return adoptRef(new DOMFileSystemSync(context, name, type, rootURL, asyncFileSystem));
    }

    static PassRefPtr<DOMFileSystemSync> create(DOMFileSystemBase*);

    virtual ~DOMFileSystemSync();

    PassRefPtr<DirectoryEntrySync> root();

    PassRefPtr<File> createFile(const FileEntrySync*, ExceptionCode&);
    PassRefPtr<FileWriterSync> createWriter(const FileEntrySync*, ExceptionCode&);

private:
    DOMFileSystemSync(ScriptExecutionContext*, const String& name, FileSystemType, const KURL& rootURL, PassOwnPtr<AsyncFileSystem>);
};

}

#endif // ENABLE(FILE_SYSTEM)

#endif // DOMFileSystemSync_h
