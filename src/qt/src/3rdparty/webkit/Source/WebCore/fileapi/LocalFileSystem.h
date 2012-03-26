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

#ifndef LocalFileSystem_h
#define LocalFileSystem_h

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystem.h"
#include "PlatformString.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class ErrorCallback;
class FileSystemCallback;
class ScriptExecutionContext;

// Keeps per-process information and provides an entry point to open a file system.
class LocalFileSystem {
    WTF_MAKE_NONCOPYABLE(LocalFileSystem);
public:
    // Returns a per-process instance of LocalFileSystem.
    // Note that LocalFileSystem::initializeLocalFileSystem must be called before
    // calling this one.
    static LocalFileSystem& localFileSystem();

    // Does not create the root path for file system, just reads it if available.
    void readFileSystem(ScriptExecutionContext*, AsyncFileSystem::Type, PassOwnPtr<AsyncFileSystemCallbacks>, bool synchronous = false);

    void requestFileSystem(ScriptExecutionContext*, AsyncFileSystem::Type, long long size, PassOwnPtr<AsyncFileSystemCallbacks>, bool synchronous = false);

#if !PLATFORM(CHROMIUM)
    // This call is not thread-safe; must be called before any worker threads are created.
    void initializeLocalFileSystem(const String&);

    String fileSystemBasePath() const;
#endif

private:
    LocalFileSystem(const String& basePath)
        : m_basePath(basePath)
    {
    }

    static LocalFileSystem* s_instance;

    // An inner class that enforces thread-safe string access.
    class SystemBasePath {
    public:
        explicit SystemBasePath(const String& path) : m_value(path) { }
        operator String() const
        {
            return m_value.threadsafeCopy();
        }
    private:
        String m_value;
    };

    SystemBasePath m_basePath;
};

} // namespace

#endif // ENABLE(FILE_SYSTEM)

#endif // LocalFileSystem_h
