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

#ifndef AsyncFileSystemBlackBerry_h
#define AsyncFileSystemBlackBerry_h

#if ENABLE(FILE_SYSTEM)
#include "AsyncFileSystem.h"
#include "FileSystemType.h"
#include "KURL.h"

#include <BlackBerryPlatformWebFileSystem.h>
#include <wtf/OwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WTF {

template <> inline void deleteOwnedPtr<BlackBerry::Platform::WebFileSystem>(BlackBerry::Platform::WebFileSystem* ptr)
{
    BlackBerry::Platform::deleteGuardedObject(ptr);
}

}

namespace WebCore {

class AsyncFileSystemBlackBerry : public AsyncFileSystem {
public:
    AsyncFileSystemBlackBerry();
    static PassOwnPtr<AsyncFileSystemBlackBerry> create(PassOwnPtr<BlackBerry::Platform::WebFileSystem> platformFileSystem)
    {
        return adoptPtr(new AsyncFileSystemBlackBerry(platformFileSystem));
    }

    static void openFileSystem(const KURL& rootURL, const String& basePath, const String& storageIdentifier, FileSystemType, long long size, bool create, int playerId, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual ~AsyncFileSystemBlackBerry();
    virtual void move(const KURL& sourcePath, const KURL& destinationPath, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void copy(const KURL& sourcePath, const KURL& destinationPath, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void remove(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void removeRecursively(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void readMetadata(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void createFile(const KURL& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void createDirectory(const KURL& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void fileExists(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void directoryExists(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void readDirectory(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void createWriter(AsyncFileWriterClient*, const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void createSnapshotFileAndReadMetadata(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);

protected:
    AsyncFileSystemBlackBerry(PassOwnPtr<BlackBerry::Platform::WebFileSystem> platformFileSystem);
    static String fileSystemURLToPath(const KURL&);

    OwnPtr<BlackBerry::Platform::WebFileSystem> m_platformFileSystem;
};

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)

#endif // AsyncFileSystemBlackBerry_h
