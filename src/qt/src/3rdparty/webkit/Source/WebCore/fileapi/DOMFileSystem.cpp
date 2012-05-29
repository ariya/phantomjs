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
#include "FileEntry.h"
#include "FileMetadata.h"
#include "FileSystemCallbacks.h"
#include "FileWriter.h"
#include "FileWriterBaseCallback.h"
#include "FileWriterCallback.h"
#include "MetadataCallback.h"
#include "ScriptExecutionContext.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

DOMFileSystem::DOMFileSystem(ScriptExecutionContext* context, const String& name, PassOwnPtr<AsyncFileSystem> asyncFileSystem)
    : DOMFileSystemBase(context, name, asyncFileSystem)
    , ActiveDOMObject(context, this)
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

    String platformPath = m_asyncFileSystem->virtualToPlatformPath(fileEntry->fullPath());

    RefPtr<FileWriter> fileWriter = FileWriter::create(scriptExecutionContext());
    RefPtr<FileWriterBaseCallback> conversionCallback = ConvertToFileWriterCallback::create(successCallback);
    OwnPtr<FileWriterBaseCallbacks> callbacks = FileWriterBaseCallbacks::create(fileWriter, conversionCallback, errorCallback);
    m_asyncFileSystem->createWriter(fileWriter.get(), platformPath, callbacks.release());
}

namespace {

class GetPathCallback : public FileSystemCallbacksBase {
public:
    static PassOwnPtr<GetPathCallback> create(PassRefPtr<DOMFileSystem> filesystem, const String& path, PassRefPtr<FileCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
    {
        return adoptPtr(new GetPathCallback(filesystem, path, successCallback, errorCallback));
    }

    virtual void didReadMetadata(const FileMetadata& metadata)
    {
        if (!metadata.platformPath.isEmpty())
            m_path = metadata.platformPath;

        m_filesystem->scheduleCallback(m_successCallback.release(), File::create(m_path));
    }

private:
    GetPathCallback(PassRefPtr<DOMFileSystem> filesystem, const String& path, PassRefPtr<FileCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
        : FileSystemCallbacksBase(errorCallback)
        , m_filesystem(filesystem)
        , m_path(path)
        , m_successCallback(successCallback)
    {
    }

    RefPtr<DOMFileSystem> m_filesystem;
    String m_path;
    RefPtr<FileCallback> m_successCallback;
};

} // namespace

void DOMFileSystem::createFile(const FileEntry* fileEntry, PassRefPtr<FileCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    String platformPath = m_asyncFileSystem->virtualToPlatformPath(fileEntry->fullPath());

    m_asyncFileSystem->readMetadata(platformPath, GetPathCallback::create(this, platformPath, successCallback, errorCallback));
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
