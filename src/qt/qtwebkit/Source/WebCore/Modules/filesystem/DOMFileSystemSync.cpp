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
#include "DOMFileSystemSync.h"

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystem.h"
#include "AsyncFileWriter.h"
#include "DOMFilePath.h"
#include "DirectoryEntrySync.h"
#include "ErrorCallback.h"
#include "File.h"
#include "FileEntrySync.h"
#include "FileError.h"
#include "FileException.h"
#include "FileMetadata.h"
#include "FileSystemCallbacks.h"
#include "FileWriterBaseCallback.h"
#include "FileWriterSync.h"

namespace WebCore {

class FileWriterBase;

PassRefPtr<DOMFileSystemSync> DOMFileSystemSync::create(DOMFileSystemBase* fileSystem)
{
    return adoptRef(new DOMFileSystemSync(fileSystem->m_context, fileSystem->name(), fileSystem->type(), fileSystem->rootURL(), fileSystem->m_asyncFileSystem.release()));
}

DOMFileSystemSync::DOMFileSystemSync(ScriptExecutionContext* context, const String& name, FileSystemType type, const KURL& rootURL, PassOwnPtr<AsyncFileSystem> asyncFileSystem)
    : DOMFileSystemBase(context, name, type, rootURL, asyncFileSystem)
{
}

DOMFileSystemSync::~DOMFileSystemSync()
{
}

PassRefPtr<DirectoryEntrySync> DOMFileSystemSync::root()
{
    return DirectoryEntrySync::create(this, DOMFilePath::root);
}

namespace {

class CreateFileHelper : public AsyncFileSystemCallbacks {
public:
    class CreateFileResult : public RefCounted<CreateFileResult> {
      public:
        static PassRefPtr<CreateFileResult> create()
        {
            return adoptRef(new CreateFileResult());
        }

        bool m_failed;
        int m_code;
        RefPtr<File> m_file;

      private:
        CreateFileResult()
            : m_failed(false)
            , m_code(0)
        {
        }

        ~CreateFileResult()
        {
        }
        friend class WTF::RefCounted<CreateFileResult>;
    };

    static PassOwnPtr<CreateFileHelper> create(PassRefPtr<CreateFileResult> result, const String& name, const KURL& url, FileSystemType type)
    {
        return adoptPtr(new CreateFileHelper(result, name, url, type));
    }

    virtual void didFail(int code)
    {
        m_result->m_failed = true;
        m_result->m_code = code;
    }

    virtual ~CreateFileHelper()
    {
    }

    virtual void didCreateSnapshotFile(const FileMetadata& metadata, PassRefPtr<BlobDataHandle> /* snapshot */)
    {
        // We can't directly use the snapshot blob data handle because the content type on it hasn't been set.
        // The |snapshot| param is here to provide a a chain of custody thru thread bridging that is held onto until
        // *after* we've coined a File with a new handle that has the correct type set on it. This allows the
        // blob storage system to track when a temp file can and can't be safely deleted.

        // For regular filesystem types (temporary or persistent), we should not cache file metadata as it could change File semantics.
        // For other filesystem types (which could be platform-specific ones), there's a chance that the files are on remote filesystem.
        // If the port has returned metadata just pass it to File constructor (so we may cache the metadata).
        // FIXME: We should use the snapshot metadata for all files.
        // https://www.w3.org/Bugs/Public/show_bug.cgi?id=17746
        if (m_type == FileSystemTypeTemporary || m_type == FileSystemTypePersistent) {
            m_result->m_file = File::createWithName(metadata.platformPath, m_name);
        } else if (!metadata.platformPath.isEmpty()) {
            // If the platformPath in the returned metadata is given, we create a File object for the path.
            m_result->m_file = File::createForFileSystemFile(m_name, metadata).get();
        } else {
            // Otherwise create a File from the FileSystem URL.
            m_result->m_file = File::createForFileSystemFile(m_url, metadata).get();
        }
    }
private:
    CreateFileHelper(PassRefPtr<CreateFileResult> result, const String& name, const KURL& url, FileSystemType type)
        : m_result(result)
        , m_name(name)
        , m_url(url)
        , m_type(type)
    {
    }

    RefPtr<CreateFileResult> m_result;
    String m_name;
    KURL m_url;
    FileSystemType m_type;
};

} // namespace

PassRefPtr<File> DOMFileSystemSync::createFile(const FileEntrySync* fileEntry, ExceptionCode& ec)
{
    ec = 0;
    KURL fileSystemURL = createFileSystemURL(fileEntry);
    RefPtr<CreateFileHelper::CreateFileResult> result(CreateFileHelper::CreateFileResult::create());
    m_asyncFileSystem->createSnapshotFileAndReadMetadata(fileSystemURL, CreateFileHelper::create(result, fileEntry->name(), fileSystemURL, type()));
    if (!m_asyncFileSystem->waitForOperationToComplete()) {
        ec = FileException::ABORT_ERR;
        return 0;
    }
    if (result->m_failed) {
        ec = result->m_code;
        return 0;
    }
    return result->m_file;
}

namespace {

class ReceiveFileWriterCallback : public FileWriterBaseCallback {
public:
    static PassRefPtr<ReceiveFileWriterCallback> create()
    {
        return adoptRef(new ReceiveFileWriterCallback());
    }

    bool handleEvent(FileWriterBase* fileWriterBase)
    {
#ifndef NDEBUG
        m_fileWriterBase = fileWriterBase;
#else
        ASSERT_UNUSED(fileWriterBase, fileWriterBase);
#endif
        return true;
    }

#ifndef NDEBUG
    FileWriterBase* fileWriterBase()
    {
        return m_fileWriterBase;
    }
#endif

private:
    ReceiveFileWriterCallback()
#ifndef NDEBUG
        : m_fileWriterBase(0)
#endif
    {
    }

#ifndef NDEBUG
    FileWriterBase* m_fileWriterBase;
#endif
};

class LocalErrorCallback : public ErrorCallback {
public:
    static PassRefPtr<LocalErrorCallback> create()
    {
        return adoptRef(new LocalErrorCallback());
    }

    bool handleEvent(FileError* error)
    {
        m_error = error;
        return true;
    }

    FileError* error()
    {
        return m_error.get();
    }

private:
    LocalErrorCallback()
    {
    }
    RefPtr<FileError> m_error;
};

}

PassRefPtr<FileWriterSync> DOMFileSystemSync::createWriter(const FileEntrySync* fileEntry, ExceptionCode& ec)
{
    ASSERT(fileEntry);
    ec = 0;


    RefPtr<FileWriterSync> fileWriter = FileWriterSync::create();
    RefPtr<ReceiveFileWriterCallback> successCallback = ReceiveFileWriterCallback::create();
    RefPtr<LocalErrorCallback> errorCallback = LocalErrorCallback::create();

    OwnPtr<FileWriterBaseCallbacks> callbacks = FileWriterBaseCallbacks::create(fileWriter, successCallback, errorCallback);
    m_asyncFileSystem->createWriter(fileWriter.get(), createFileSystemURL(fileEntry), callbacks.release());
    if (!m_asyncFileSystem->waitForOperationToComplete()) {
        ec = FileException::ABORT_ERR;
        return 0;
    }
    if (errorCallback->error()) {
        ASSERT(!successCallback->fileWriterBase());
        ec = FileException::ErrorCodeToExceptionCode(errorCallback->error()->code());
        return 0;
    }
    ASSERT(successCallback->fileWriterBase());
    ASSERT(static_cast<FileWriterSync*>(successCallback->fileWriterBase()) == fileWriter.get());
    return fileWriter;
}

}

#endif // ENABLE(FILE_SYSTEM)
