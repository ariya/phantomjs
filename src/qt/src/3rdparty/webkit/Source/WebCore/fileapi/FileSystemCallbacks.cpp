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
#include "FileSystemCallbacks.h"

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystem.h"
#include "AsyncFileWriter.h"
#include "DOMFilePath.h"
#include "DOMFileSystemBase.h"
#include "DirectoryEntry.h"
#include "DirectoryReader.h"
#include "EntriesCallback.h"
#include "EntryArray.h"
#include "EntryCallback.h"
#include "ErrorCallback.h"
#include "FileEntry.h"
#include "FileError.h"
#include "FileMetadata.h"
#include "FileSystemCallback.h"
#include "FileWriterBase.h"
#include "FileWriterBaseCallback.h"
#include "Metadata.h"
#include "MetadataCallback.h"
#include "ScriptExecutionContext.h"
#include "VoidCallback.h"

namespace WebCore {

FileSystemCallbacksBase::FileSystemCallbacksBase(PassRefPtr<ErrorCallback> errorCallback)
    : m_errorCallback(errorCallback)
{
}

FileSystemCallbacksBase::~FileSystemCallbacksBase()
{
}

void FileSystemCallbacksBase::didSucceed()
{
    // Each subclass must implement an appropriate one.
    ASSERT_NOT_REACHED();
}

void FileSystemCallbacksBase::didOpenFileSystem(const String&, PassOwnPtr<AsyncFileSystem>)
{
    // Each subclass must implement an appropriate one.
    ASSERT_NOT_REACHED();
}

void FileSystemCallbacksBase::didReadMetadata(const FileMetadata&)
{
    // Each subclass must implement an appropriate one.
    ASSERT_NOT_REACHED();
}

void FileSystemCallbacksBase::didReadDirectoryEntries(bool)
{
    // Each subclass must implement an appropriate one.
    ASSERT_NOT_REACHED();
}

void FileSystemCallbacksBase::didReadDirectoryEntry(const String&, bool)
{
    // Each subclass must implement an appropriate one.
    ASSERT_NOT_REACHED();
}

void FileSystemCallbacksBase::didCreateFileWriter(PassOwnPtr<AsyncFileWriter>, long long)
{
    // Each subclass must implement an appropriate one.
    ASSERT_NOT_REACHED();
}

void FileSystemCallbacksBase::didFail(int code)
{
    if (m_errorCallback) {
        m_errorCallback->handleEvent(FileError::create(static_cast<FileError::ErrorCode>(code)).get());
        m_errorCallback.clear();
    }
}

// EntryCallbacks -------------------------------------------------------------

PassOwnPtr<EntryCallbacks> EntryCallbacks::create(PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback, PassRefPtr<DOMFileSystemBase> fileSystem, const String& expectedPath, bool isDirectory)
{
    return adoptPtr(new EntryCallbacks(successCallback, errorCallback, fileSystem, expectedPath, isDirectory));
}

EntryCallbacks::EntryCallbacks(PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback, PassRefPtr<DOMFileSystemBase> fileSystem, const String& expectedPath, bool isDirectory)
    : FileSystemCallbacksBase(errorCallback)
    , m_successCallback(successCallback)
    , m_fileSystem(fileSystem)
    , m_expectedPath(expectedPath)
    , m_isDirectory(isDirectory)
{
}

void EntryCallbacks::didSucceed()
{
    if (m_successCallback) {
        if (m_isDirectory)
            m_successCallback->handleEvent(DirectoryEntry::create(m_fileSystem, m_expectedPath).get());
        else
            m_successCallback->handleEvent(FileEntry::create(m_fileSystem, m_expectedPath).get());
    }
    m_successCallback.clear();
}

// EntriesCallbacks -----------------------------------------------------------

PassOwnPtr<EntriesCallbacks> EntriesCallbacks::create(PassRefPtr<EntriesCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback, PassRefPtr<DirectoryReaderBase> directoryReader, const String& basePath)
{
    return adoptPtr(new EntriesCallbacks(successCallback, errorCallback, directoryReader, basePath));
}

EntriesCallbacks::EntriesCallbacks(PassRefPtr<EntriesCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback, PassRefPtr<DirectoryReaderBase> directoryReader, const String& basePath)
    : FileSystemCallbacksBase(errorCallback)
    , m_successCallback(successCallback)
    , m_directoryReader(directoryReader)
    , m_basePath(basePath)
    , m_entries(EntryArray::create())
{
    ASSERT(m_directoryReader);
}

void EntriesCallbacks::didReadDirectoryEntry(const String& name, bool isDirectory)
{
    if (isDirectory)
        m_entries->append(DirectoryEntry::create(m_directoryReader->filesystem(), DOMFilePath::append(m_basePath, name)));
    else
        m_entries->append(FileEntry::create(m_directoryReader->filesystem(), DOMFilePath::append(m_basePath, name)));
}

void EntriesCallbacks::didReadDirectoryEntries(bool hasMore)
{
    m_directoryReader->setHasMoreEntries(hasMore);
    if (m_successCallback)
        m_successCallback->handleEvent(m_entries.get());
}

// FileSystemCallbacks --------------------------------------------------------

PassOwnPtr<FileSystemCallbacks> FileSystemCallbacks::create(PassRefPtr<FileSystemCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback, ScriptExecutionContext* scriptExecutionContext)
{
    return adoptPtr(new FileSystemCallbacks(successCallback, errorCallback, scriptExecutionContext));
}

FileSystemCallbacks::FileSystemCallbacks(PassRefPtr<FileSystemCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback, ScriptExecutionContext* context)
    : FileSystemCallbacksBase(errorCallback)
    , m_successCallback(successCallback)
    , m_scriptExecutionContext(context)
{
}

void FileSystemCallbacks::didOpenFileSystem(const String& name, PassOwnPtr<AsyncFileSystem> asyncFileSystem)
{
    if (m_successCallback) {
        ASSERT(asyncFileSystem);
        m_successCallback->handleEvent(DOMFileSystem::create(m_scriptExecutionContext.get(), name, asyncFileSystem.leakPtr()).get());
        m_scriptExecutionContext.clear();
    }
    m_successCallback.clear();
}

// ResolveURICallbacks --------------------------------------------------------

namespace {

class ErrorCallbackWrapper : public ErrorCallback {
public:
    static PassRefPtr<ErrorCallbackWrapper> create(PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback, PassRefPtr<DirectoryEntry> root, const String& filePath)
    {
        return adoptRef(new ErrorCallbackWrapper(successCallback, errorCallback, root, filePath));
    }

    virtual bool handleEvent(FileError* error)
    {
        ASSERT(error);
        if (error->code() == FileError::TYPE_MISMATCH_ERR)
            m_root->getFile(m_filePath, 0, m_successCallback, m_errorCallback);
        else if (m_errorCallback)
            m_errorCallback->handleEvent(error);
        return true;
    }

private:
    ErrorCallbackWrapper(PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback, PassRefPtr<DirectoryEntry> root, const String& filePath)
        : m_successCallback(successCallback)
        , m_errorCallback(errorCallback)
        , m_root(root)
        , m_filePath(filePath)
    {
        ASSERT(m_root);
    }

    RefPtr<EntryCallback> m_successCallback;
    RefPtr<ErrorCallback> m_errorCallback;
    RefPtr<DirectoryEntry> m_root;
    String m_filePath;
};

} // namespace

PassOwnPtr<ResolveURICallbacks> ResolveURICallbacks::create(PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback, ScriptExecutionContext* scriptExecutionContext, const String& filePath)
{
    return adoptPtr(new ResolveURICallbacks(successCallback, errorCallback, scriptExecutionContext, filePath));
}

ResolveURICallbacks::ResolveURICallbacks(PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback, ScriptExecutionContext* context, const String& filePath)
    : FileSystemCallbacksBase(errorCallback)
    , m_successCallback(successCallback)
    , m_scriptExecutionContext(context)
    , m_filePath(filePath)
{
}

void ResolveURICallbacks::didOpenFileSystem(const String& name, PassOwnPtr<AsyncFileSystem> asyncFileSystem)
{
    ASSERT(asyncFileSystem);
    RefPtr<DirectoryEntry> root = DOMFileSystem::create(m_scriptExecutionContext.get(), name, asyncFileSystem.leakPtr())->root();
    root->getDirectory(m_filePath, 0, m_successCallback, ErrorCallbackWrapper::create(m_successCallback, m_errorCallback, root, m_filePath));
}

// MetadataCallbacks ----------------------------------------------------------

PassOwnPtr<MetadataCallbacks> MetadataCallbacks::create(PassRefPtr<MetadataCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    return adoptPtr(new MetadataCallbacks(successCallback, errorCallback));
}

MetadataCallbacks::MetadataCallbacks(PassRefPtr<MetadataCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
    : FileSystemCallbacksBase(errorCallback)
    , m_successCallback(successCallback)
{
}

void MetadataCallbacks::didReadMetadata(const FileMetadata& metadata)
{
    if (m_successCallback)
        m_successCallback->handleEvent(Metadata::create(metadata.modificationTime).get());
    m_successCallback.clear();
}

// FileWriterBaseCallbacks ----------------------------------------------------------

PassOwnPtr<FileWriterBaseCallbacks> FileWriterBaseCallbacks::create(PassRefPtr<FileWriterBase> fileWriter, PassRefPtr<FileWriterBaseCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    return adoptPtr(new FileWriterBaseCallbacks(fileWriter, successCallback, errorCallback));
}

FileWriterBaseCallbacks::FileWriterBaseCallbacks(PassRefPtr<FileWriterBase> fileWriter, PassRefPtr<FileWriterBaseCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
    : FileSystemCallbacksBase(errorCallback)
    , m_fileWriter(fileWriter)
    , m_successCallback(successCallback)
{
}

void FileWriterBaseCallbacks::didCreateFileWriter(PassOwnPtr<AsyncFileWriter> asyncFileWriter, long long length)
{
    m_fileWriter->initialize(asyncFileWriter, length);
    if (m_successCallback)
        m_successCallback->handleEvent(m_fileWriter.release().get());
    m_successCallback.clear();
}

// VoidCallbacks --------------------------------------------------------------

PassOwnPtr<VoidCallbacks> VoidCallbacks::create(PassRefPtr<VoidCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    return adoptPtr(new VoidCallbacks(successCallback, errorCallback));
}

VoidCallbacks::VoidCallbacks(PassRefPtr<VoidCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
    : FileSystemCallbacksBase(errorCallback)
    , m_successCallback(successCallback)
{
}

void VoidCallbacks::didSucceed()
{
    if (m_successCallback)
        m_successCallback->handleEvent();
    m_successCallback.clear();
}

} // namespace

#endif // ENABLE(FILE_SYSTEM)
