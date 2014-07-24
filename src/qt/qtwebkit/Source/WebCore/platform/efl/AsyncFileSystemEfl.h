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

#ifndef  AsyncFileSystemEfl_h
#define  AsyncFileSystemEfl_h

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystem.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class AsyncFileSystemCallbacks;

class AsyncFileSystemEfl : public AsyncFileSystem {
public:
    AsyncFileSystemEfl();
    virtual ~AsyncFileSystemEfl();

    virtual void move(const KURL&, const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void copy(const KURL&, const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void remove(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void removeRecursively(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void readMetadata(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void createFile(const KURL&, bool, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void createDirectory(const KURL&, bool, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void fileExists(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void directoryExists(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void readDirectory(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void createWriter(AsyncFileWriterClient*, const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void createSnapshotFileAndReadMetadata(const KURL&, PassOwnPtr<AsyncFileSystemCallbacks>);
};

} // namespace WebCore

#endif

#endif //  AsyncFileSystemEfl_h
