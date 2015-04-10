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

#ifndef AsyncFileSystem_h
#define AsyncFileSystem_h

#if ENABLE(FILE_SYSTEM)

#include "FileSystemType.h"
#include "KURL.h"
#include "Timer.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class AsyncFileSystem;
class AsyncFileSystemCallbacks;
class AsyncFileWriterClient;

// This class provides async interface for platform-specific file system implementation.  Note that all the methods take platform paths.
class AsyncFileSystem {
    WTF_MAKE_NONCOPYABLE(AsyncFileSystem);
public:
    virtual ~AsyncFileSystem() { }

    virtual void stop() { }
    virtual bool hasPendingActivity() { return false; }

    static bool isAvailable();

    // Subclass must implement this if it supports synchronous operations.
    // This should return false if there are no pending operations.
    virtual bool waitForOperationToComplete() { return false; }

    // Creates and returns a new platform-specific AsyncFileSystem instance if the platform has its own implementation.
    static PassOwnPtr<AsyncFileSystem> create();

    // Opens a new file system. The create parameter specifies whether or not to create the path if it does not already exists.
    static void openFileSystem(const String& basePath, const String& storageIdentifier, FileSystemType, bool create, PassOwnPtr<AsyncFileSystemCallbacks>);

    // Deletes the file system.
    static void deleteFileSystem(const String& basePath, const String& storageIdentifier, FileSystemType, PassOwnPtr<AsyncFileSystemCallbacks>);

    // Moves a file or directory from srcPath to destPath.
    // AsyncFileSystemCallbacks::didSucceed() is called when the operation is completed successfully.
    // AsyncFileSystemCallbacks::didFail() is called otherwise.
    virtual void move(const KURL& srcPath, const KURL& destPath, PassOwnPtr<AsyncFileSystemCallbacks>) = 0;

    // Copies a file or directory from srcPath to destPath.
    // AsyncFileSystemCallbacks::didSucceed() is called when the operation is completed successfully.
    // AsyncFileSystemCallbacks::didFail() is called otherwise.
    virtual void copy(const KURL& srcPath, const KURL& destPath, PassOwnPtr<AsyncFileSystemCallbacks>) = 0;

    // Deletes a file or directory at a given path.
    // It is an error to try to remove a directory that is not empty.
    // AsyncFileSystemCallbacks::didSucceed() is called when the operation is completed successfully.
    // AsyncFileSystemCallbacks::didFail() is called otherwise.
    virtual void remove(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>) = 0;

    // Recursively deletes a directory at a given path.
    // AsyncFileSystemCallbacks::didSucceed() is called when the operation is completed successfully.
    // AsyncFileSystemCallbacks::didFail() is called otherwise.
    virtual void removeRecursively(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>) = 0;

    // Retrieves the metadata information of the file or directory at a given path.
    // AsyncFileSystemCallbacks::didReadMetadata() is called when the operation is completed successfully.
    // AsyncFileSystemCallbacks::didFail() is called otherwise.
    virtual void readMetadata(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>) = 0;

    // Creates a file at a given path.  If exclusive flag is true, it fails if the path already exists.
    // AsyncFileSystemCallbacks::didSucceed() is called when the operation is completed successfully.
    // AsyncFileSystemCallbacks::didFail() is called otherwise.
    virtual void createFile(const KURL& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks>) = 0;

    // Creates a directory at a given path.  If exclusive flag is true, it fails if the path already exists.
    // AsyncFileSystemCallbacks::didSucceed() is called when the operation is completed successfully.
    // AsyncFileSystemCallbacks::didFail() is called otherwise.
    virtual void createDirectory(const KURL& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks>) = 0;

    // Checks if a file exists at a given path.
    // AsyncFileSystemCallbacks::didSucceed() is called if the file exists.
    // AsyncFileSystemCallbacks::didFail() is called otherwise.
    virtual void fileExists(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>) = 0;

    // Checks if a directory exists at a given path.
    // AsyncFileSystemCallbacks::didSucceed() is called if the directory exists.
    // AsyncFileSystemCallbacks::didFail() is called otherwise.
    virtual void directoryExists(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>) = 0;

    // Reads directory entries of a given directory at path.
    // AsyncFileSystemCallbacks::didReadDirectoryEntry() is called when each directory entry is called. AsyncFileSystemCallbacks::didReadDirectoryEntries() is called after a chunk of directory entries have been read.
    // AsyncFileSystemCallbacks::didFail() is when there is an error.
    virtual void readDirectory(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>) = 0;

    // Creates an AsyncFileWriter for a given file path.
    // AsyncFileSystemCallbacks::didCreateFileWriter() is called when an AsyncFileWriter is created successfully.
    // AsyncFileSystemCallbacks::didFail() is called otherwise.
    virtual void createWriter(AsyncFileWriterClient*, const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>) = 0;

    // Creates a snapshot file and read its metadata for a new File object.
    // In local filesystem cases the backend may simply return the metadata of the file itself (as well as readMetadata does),
    // while in remote filesystem case the backend may download the file into a temporary snapshot file and return the metadata of the temporary file.
    // AsyncFileSystemCallbacks::didReadMetadata() is called when the metadata for the snapshot file is successfully returned.
    // AsyncFileSystemCallbacks::didFail() is called otherwise.
    //
    // Note: the returned metadata info is cached in the File object for non-regular filesystem types (neither Temporary nor Persistent). The port could return valid metadata if it wants File object to cache metadata (e.g. if the file body is on a remote server), but otherwise should NOT return valid metadata.
    virtual void createSnapshotFileAndReadMetadata(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>) = 0;

protected:
    AsyncFileSystem() { }
};

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)

#endif // AsyncFileSystem_h
