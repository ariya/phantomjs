/*
 * Copyright (C) 2012 ChangSeok Oh <shivamidow@gmail.com>
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

#include "AsyncFileSystemGtk.h"

#include "AsyncFileSystemCallbacks.h"
#include "ExceptionCode.h"
#include "NotImplemented.h"

namespace WebCore {

bool AsyncFileSystem::isAvailable()
{
    notImplemented();
    return false;
}

PassOwnPtr<AsyncFileSystem> AsyncFileSystem::create()
{
    return adoptPtr(new AsyncFileSystemGtk());
}

void AsyncFileSystem::openFileSystem(const String& basePath, const String& storageIdentifier, FileSystemType, bool, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    notImplemented();
    callbacks->didFail(NOT_SUPPORTED_ERR);
}

void AsyncFileSystem::deleteFileSystem(const String& basePath, const String& storageIdentifier, FileSystemType type, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    notImplemented();
    callbacks->didFail(NOT_SUPPORTED_ERR);
}

AsyncFileSystemGtk::AsyncFileSystemGtk()
{
    notImplemented();
}

AsyncFileSystemGtk::~AsyncFileSystemGtk()
{
    notImplemented();
}

void AsyncFileSystemGtk::move(const KURL& sourcePath, const KURL& destinationPath, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    notImplemented();
}

void AsyncFileSystemGtk::copy(const KURL& sourcePath, const KURL& destinationPath, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    notImplemented();
}

void AsyncFileSystemGtk::remove(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    notImplemented();
}

void AsyncFileSystemGtk::removeRecursively(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    notImplemented();
}

void AsyncFileSystemGtk::readMetadata(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    notImplemented();
}

void AsyncFileSystemGtk::createFile(const KURL& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    notImplemented();
}

void AsyncFileSystemGtk::createDirectory(const KURL& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    notImplemented();
}

void AsyncFileSystemGtk::fileExists(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    notImplemented();
}

void AsyncFileSystemGtk::directoryExists(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    notImplemented();
}

void AsyncFileSystemGtk::readDirectory(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    notImplemented();
}


void AsyncFileSystemGtk::createWriter(AsyncFileWriterClient* client, const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    notImplemented();
}

void AsyncFileSystemGtk::createSnapshotFileAndReadMetadata(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

} // namespace WebCore

#endif
