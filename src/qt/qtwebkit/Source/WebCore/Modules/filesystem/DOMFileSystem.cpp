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
#include "DOMFileSystem.h"

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystem.h"
#include "DOMFilePath.h"
#include "DirectoryEntry.h"
#include "ErrorCallback.h"
#include "File.h"
#include "FileCallback.h"
#include "FileEntry.h"
#include "FileMetadata.h"
#include "FileSystemCallbacks.h"
#include "FileWriter.h"
#include "FileWriterBaseCallback.h"
#include "FileWriterCallback.h"
#include "MetadataCallback.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include <wtf/OwnPtr.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

// static
PassRefPtr<DOMFileSystem> DOMFileSystem::create(ScriptExecutionContext* context, const String& name, FileSystemType type, const KURL& rootURL, PassOwnPtr<AsyncFileSystem> asyncFileSystem)
{
    RefPtr<DOMFileSystem> fileSystem(adoptRef(new DOMFileSystem(context, name, type, rootURL, asyncFileSystem)));
    fileSystem->suspendIfNeeded();
    return fileSystem.release();
}

PassRefPtr<DOMFileSystem> DOMFileSystem::createIsolatedFileSystem(ScriptExecutionContext* context, const String& filesystemId)
{
    if (filesystemId.isEmpty())
        return 0;

    StringBuilder filesystemName;
    filesystemName.append(context->securityOrigin()->databaseIdentifier());
    filesystemName.append(":Isolated_");
    filesystemName.append(filesystemId);

    // The rootURL created here is going to be attached to each filesystem request and
    // is to be validated each time the request is being handled.
    StringBuilder rootURL;
    rootURL.append("filesystem:");
    rootURL.append(context->securityOrigin()->toString());
    rootURL.append("/");
    rootURL.append(isolatedPathPrefix);
    rootURL.append("/");
    rootURL.append(filesystemId);
    rootURL.append("/");

    return DOMFileSystem::create(context, filesystemName.toString(), FileSystemTypeIsolated, KURL(ParsedURLString, rootURL.toString()), AsyncFileSystem::create());
}

DOMFileSystem::DOMFileSystem(ScriptExecutionContext* context, const String& name, FileSystemType type, const KURL& rootURL, PassOwnPtr<AsyncFileSystem> asyncFileSystem)
    : DOMFileSystemBase(context, name, type, rootURL, asyncFileSystem)
    , ActiveDOMObject(context)
{
}

PassRefPtr<DirectoryEntry> DOMFileSystem::root()
{
    return DirectoryEntry::create(this, DOMFilePath::root);
}

void DOMFileSystem::stop()
{
    m_asyncFileSystem->stop();
}

bool DOMFileSystem::hasPendingActivity() const
{
    return m_asyncFileSystem->hasPendingActivity();
}

void DOMFileSystem::contextDestroyed()
{
    m_asyncFileSystem->stop();
    ActiveDOMObject::contextDestroyed();
}

namespace {

class ConvertToFileWriterCallback : public FileWriterBaseCallback {
public:
    static PassRefPtr<ConvertToFileWriterCallback> create(PassRefPtr<FileWriterCallback> callback)
    {
        return adoptRef(new ConvertToFileWriterCallback(callback));
    }

    bool handleEvent(FileWriterBase* fileWriterBase)
    {
        return m_callback->handleEvent(static_cast<FileWriter*>(fileWriterBase));
    }
private:
    ConvertToFileWriterCallback(PassRefPtr<FileWriterCallback> callback)
        : m_callback(callback)
    {
    }
    RefPtr<FileWriterCallback> m_callback;
};

}

void DOMFileSystem::createWriter(const FileEntry* fileEntry, PassRefPtr<FileWriterCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    ASSERT(fileEntry);

    RefPtr<FileWriter> fileWriter = FileWriter::create(scriptExecutionContext());
    RefPtr<FileWriterBaseCallback> conversionCallback = ConvertToFileWriterCallback::create(successCallback);
    OwnPtr<FileWriterBaseCallbacks> callbacks = FileWriterBaseCallbacks::create(fileWriter, conversionCallback, errorCallback);
    m_asyncFileSystem->createWriter(fileWriter.get(), createFileSystemURL(fileEntry), callbacks.release());
}

namespace {

class SnapshotFileCallback : public FileSystemCallbacksBase {
public:
    static PassOwnPtr<SnapshotFileCallback> create(PassRefPtr<DOMFileSystem> filesystem, const String& name, const KURL& url, PassRefPtr<FileCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
    {
        return adoptPtr(new SnapshotFileCallback(filesystem, name, url, successCallback, errorCallback));
    }

    virtual void didCreateSnapshotFile(const FileMetadata& metadata, PassRefPtr<BlobDataHandle> /* snapshot */)
    {
        ASSERT(!metadata.platformPath.isEmpty());
        if (!m_successCallback)
            return;

        // We can't directly use the snapshot blob data handle because the content type on it hasn't been set.
        // The |snapshot| param is here to provide a a chain of custody thru thread bridging that is held onto until
        // *after* we've coined a File with a new handle that has the correct type set on it. This allows the
        // blob storage system to track when a temp file can and can't be safely deleted.

        // For regular filesystem types (temporary or persistent), we should not cache file metadata as it could change File semantics.
        // For other filesystem types (which could be platform-specific ones), there's a chance that the files are on remote filesystem. If the port has returned metadata just pass it to File constructor (so we may cache the metadata).
        // FIXME: We should use the snapshot metadata for all files.
        // https://www.w3.org/Bugs/Public/show_bug.cgi?id=17746
        if (m_filesystem->type() == FileSystemTypeTemporary || m_filesystem->type() == FileSystemTypePersistent) {
            m_successCallback->handleEvent(File::createWithName(metadata.platformPath, m_name).get());
        } else if (!metadata.platformPath.isEmpty()) {
            // If the platformPath in the returned metadata is given, we create a File object for the path.
            m_successCallback->handleEvent(File::createForFileSystemFile(m_name, metadata).get());
        } else {
            // Otherwise create a File from the FileSystem URL.
            m_successCallback->handleEvent(File::createForFileSystemFile(m_url, metadata).get());
        }

        m_successCallback.release();
    }

private:
    SnapshotFileCallback(PassRefPtr<DOMFileSystem> filesystem, const String& name,  const KURL& url, PassRefPtr<FileCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
        : FileSystemCallbacksBase(errorCallback)
        , m_filesystem(filesystem)
        , m_name(name)
        , m_url(url)
        , m_successCallback(successCallback)
    {
    }

    RefPtr<DOMFileSystem> m_filesystem;
    String m_name;
    KURL m_url;
    RefPtr<FileCallback> m_successCallback;
};

} // namespace

void DOMFileSystem::createFile(const FileEntry* fileEntry, PassRefPtr<FileCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    KURL fileSystemURL = createFileSystemURL(fileEntry);
    m_asyncFileSystem->createSnapshotFileAndReadMetadata(fileSystemURL, SnapshotFileCallback::create(this, fileEntry->name(), fileSystemURL, successCallback, errorCallback));
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
