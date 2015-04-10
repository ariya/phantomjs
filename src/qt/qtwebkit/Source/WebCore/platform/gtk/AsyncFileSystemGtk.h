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

#ifndef  AsyncFileSystemGtk_h
#define  AsyncFileSystemGtk_h

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystem.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class AsyncFileSystemCallbacks;

class AsyncFileSystemGtk : public AsyncFileSystem {
public:
    AsyncFileSystemGtk();
    virtual ~AsyncFileSystemGtk();

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
};

} // namespace WebCore

#endif

#endif //  AsyncFileSystemGtk_h
