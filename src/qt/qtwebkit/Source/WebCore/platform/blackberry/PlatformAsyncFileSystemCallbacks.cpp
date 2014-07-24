/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */

#include "config.h"
#if ENABLE(FILE_SYSTEM)
#include "PlatformAsyncFileSystemCallbacks.h"

#include "Assertions.h"
#include "AsyncFileSystem.h"
#include "AsyncFileSystemBlackBerry.h"
#include "AsyncFileWriterBlackBerry.h"
#include "AsyncFileWriterClient.h"
#include "FileError.h"
#include "FileMetadata.h"

#include <GuardedPointerDeleter.h>

namespace WebCore {

void PlatformAsyncFileSystemCallbacks::notifyOpenFileSystem(BlackBerry::Platform::WebFileSystem* platformFileSystem)
{
    ASSERT(platformFileSystem);
    m_callbacks->didOpenFileSystem(String::fromUTF8(platformFileSystem->name().c_str()), m_rootURL, createAsyncFileSystem(adoptPtr(platformFileSystem)));
    deleteMe();
}

void PlatformAsyncFileSystemCallbacks::notifySucceed()
{
    m_callbacks->didSucceed();
    deleteMe();
}

void PlatformAsyncFileSystemCallbacks::notifyFail(BlackBerry::Platform::WebFileError error)
{
    m_callbacks->didFail(static_cast<FileError::ErrorCode>(error));
    deleteMe();
}

static inline void getFileMetadata(const BlackBerry::Platform::WebFileInfo& fileInfo, FileMetadata& fileMetadata)
{
    fileMetadata.modificationTime = fileInfo.m_modificationTime;
    fileMetadata.length = fileInfo.m_length;
    fileMetadata.type = static_cast<FileMetadata::Type>(fileInfo.m_type);
    fileMetadata.platformPath = String::fromUTF8(fileInfo.m_platformPath.c_str());
}

void PlatformAsyncFileSystemCallbacks::notifyReadMetadata(const BlackBerry::Platform::WebFileInfo& fileInfo)
{
    FileMetadata fileMetadata;
    getFileMetadata(fileInfo, fileMetadata);
    m_callbacks->didReadMetadata(fileMetadata);
    deleteMe();
}

void PlatformAsyncFileSystemCallbacks::notifyCreateSnapshotFileAndReadMetadata(const BlackBerry::Platform::WebFileInfo& fileInfo)
{
    FileMetadata fileMetadata;
    getFileMetadata(fileInfo, fileMetadata);
    m_callbacks->didCreateSnapshotFile(fileMetadata, 0);
    deleteMe();
}

void PlatformAsyncFileSystemCallbacks::notifyReadDirectory(const std::vector<BlackBerry::Platform::WebFileSystemEntry>& entries, bool hasMore)
{
    for (size_t i = 0; i < entries.size(); ++i)
        m_callbacks->didReadDirectoryEntry(String::fromUTF8(entries[i].m_name.c_str()), entries[i].m_isDirectory);
    m_callbacks->didReadDirectoryEntries(hasMore);
    deleteMe();
}

void PlatformAsyncFileSystemCallbacks::notifyCreateFileWriter(BlackBerry::Platform::WebFileWriter* platformWriter, long long length)
{
    m_callbacks->didCreateFileWriter(createAsyncFileWriter(adoptPtr(platformWriter)), length);
    deleteMe();
}

PassOwnPtr<AsyncFileSystem> PlatformAsyncFileSystemCallbacks::createAsyncFileSystem(PassOwnPtr<BlackBerry::Platform::WebFileSystem> platformFileSystem)
{
    return AsyncFileSystemBlackBerry::create(platformFileSystem);
}

PassOwnPtr<AsyncFileWriter> PlatformAsyncFileSystemCallbacks::createAsyncFileWriter(PassOwnPtr<BlackBerry::Platform::WebFileWriter> platformWriter)
{
    ASSERT(m_writerClient);
    BlackBerry::Platform::WebFileWriter* platformWriterPtr = platformWriter.get();
    OwnPtr<AsyncFileWriterBlackBerry> writer = AsyncFileWriterBlackBerry::create(platformWriter, m_writerClient);
    platformWriterPtr->setClient(writer->platformWriterClient());
    return writer.release();
}

void PlatformAsyncFileSystemCallbacks::deleteMe()
{
    BlackBerry::Platform::GuardedPointerDeleter::deleteOnThread(BlackBerry::Platform::webKitThreadMessageClient(), this);
}

} // namespace WebCore
#endif
