/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AsyncFileSystemEfl.h"

#if ENABLE(FILE_SYSTEM)

#include "ExceptionCode.h"
#include "LocalFileSystem.h"
#include "NotImplemented.h"

namespace WebCore {

bool AsyncFileSystem::isAvailable()
{
    return false;
}

PassOwnPtr<AsyncFileSystem> AsyncFileSystem::create()
{
    return adoptPtr(new AsyncFileSystemEfl());
}

void AsyncFileSystem::openFileSystem(const String&, const String&, FileSystemType, bool, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

void AsyncFileSystem::deleteFileSystem(const String&, const String&, FileSystemType, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

AsyncFileSystemEfl::AsyncFileSystemEfl()
    : AsyncFileSystem()
{
}

AsyncFileSystemEfl::~AsyncFileSystemEfl()
{
}

void AsyncFileSystemEfl::move(const KURL&, const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

void AsyncFileSystemEfl::copy(const KURL&, const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

void AsyncFileSystemEfl::remove(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

void AsyncFileSystemEfl::removeRecursively(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

void AsyncFileSystemEfl::readMetadata(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

void AsyncFileSystemEfl::createFile(const KURL&, bool, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

void AsyncFileSystemEfl::createDirectory(const KURL&, bool, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

void AsyncFileSystemEfl::fileExists(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

void AsyncFileSystemEfl::directoryExists(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

void AsyncFileSystemEfl::readDirectory(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

void AsyncFileSystemEfl::createWriter(AsyncFileWriterClient*, const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

void AsyncFileSystemEfl::createSnapshotFileAndReadMetadata(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

} // namespace WebCore

#endif
