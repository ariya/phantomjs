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

#ifndef AsyncFileSystemCallbacks_h
#define AsyncFileSystemCallbacks_h

#if ENABLE(FILE_SYSTEM)

#include "PlatformString.h"

namespace WebCore {

class AsyncFileSystem;
class AsyncFileWriter;
struct FileMetadata;

class AsyncFileSystemCallbacks {
    WTF_MAKE_NONCOPYABLE(AsyncFileSystemCallbacks);
public:
    AsyncFileSystemCallbacks() { }    

    // Called when a requested operation is completed successfully.
    virtual void didSucceed() = 0;

    // Called when a requested file system is opened.
    virtual void didOpenFileSystem(const String& name, PassOwnPtr<AsyncFileSystem>) = 0;

    // Called when a file metadata is read successfully.
    virtual void didReadMetadata(const FileMetadata&) = 0;

    // Called when a directory entry is read.
    virtual void didReadDirectoryEntry(const String& name, bool isDirectory) = 0;

    // Called after a chunk of directory entries have been read (i.e. indicates it's good time to call back to the application).  If hasMore is true there can be more chunks.
    virtual void didReadDirectoryEntries(bool hasMore) = 0;

    // Called when an AsyncFileWrter has been created successfully.
    virtual void didCreateFileWriter(PassOwnPtr<AsyncFileWriter> writer, long long length) = 0;

    // Called when there was an error.
    virtual void didFail(int code) = 0;

    virtual ~AsyncFileSystemCallbacks() { }
};

} // namespace

#endif // ENABLE(FILE_SYSTEM)

#endif // AsyncFileSystemCallbacks_h
