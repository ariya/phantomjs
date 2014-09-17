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

#ifndef DOMFileSystemBase_h
#define DOMFileSystemBase_h

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystem.h"
#include "WebKitFlags.h"
#include "PlatformString.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class DirectoryEntry;
class DirectoryReaderBase;
class EntriesCallback;
class EntryBase;
class EntryCallback;
class ErrorCallback;
class KURL;
class MetadataCallback;
class ScriptExecutionContext;
class SecurityOrigin;
class VoidCallback;

// A common base class for DOMFileSystem and DOMFileSystemSync.
class DOMFileSystemBase : public RefCounted<DOMFileSystemBase> {
public:
    static PassRefPtr<DOMFileSystemBase> create(ScriptExecutionContext* context, const String& name, PassOwnPtr<AsyncFileSystem> asyncFileSystem)
    {
        return adoptRef(new DOMFileSystemBase(context, name, asyncFileSystem));
    }
    virtual ~DOMFileSystemBase();

    static const char kPersistentPathPrefix[];
    static const size_t kPersistentPathPrefixLength;
    static const char kTemporaryPathPrefix[];
    static const size_t kTemporaryPathPrefixLength;
    static const char kExternalPathPrefix[];
    static const size_t kExternalPathPrefixLength;
    static bool crackFileSystemURL(const KURL&, AsyncFileSystem::Type&, String& filePath);

    const String& name() const { return m_name; }
    AsyncFileSystem* asyncFileSystem() const { return m_asyncFileSystem.get(); }
    SecurityOrigin* securityOrigin() const;

    // Actual FileSystem API implementations.  All the validity checks on virtual paths are done at this level.
    // They return false for immediate errors that don't involve lower AsyncFileSystem layer (e.g. for name validation errors).  Otherwise they return true (but later may call back with an runtime error).
    bool getMetadata(const EntryBase*, PassRefPtr<MetadataCallback>, PassRefPtr<ErrorCallback>);
    bool move(const EntryBase* source, EntryBase* parent, const String& name, PassRefPtr<EntryCallback>, PassRefPtr<ErrorCallback>);
    bool copy(const EntryBase* source, EntryBase* parent, const String& name, PassRefPtr<EntryCallback>, PassRefPtr<ErrorCallback>);
    bool remove(const EntryBase*, PassRefPtr<VoidCallback>, PassRefPtr<ErrorCallback>);
    bool removeRecursively(const EntryBase*, PassRefPtr<VoidCallback>, PassRefPtr<ErrorCallback>);
    bool getParent(const EntryBase*, PassRefPtr<EntryCallback>, PassRefPtr<ErrorCallback>);
    bool getFile(const EntryBase*, const String& path, PassRefPtr<WebKitFlags>, PassRefPtr<EntryCallback>, PassRefPtr<ErrorCallback>);
    bool getDirectory(const EntryBase*, const String& path, PassRefPtr<WebKitFlags>, PassRefPtr<EntryCallback>, PassRefPtr<ErrorCallback>);
    bool readDirectory(PassRefPtr<DirectoryReaderBase>, const String& path, PassRefPtr<EntriesCallback>, PassRefPtr<ErrorCallback>);

protected:
    DOMFileSystemBase(ScriptExecutionContext*, const String& name, PassOwnPtr<AsyncFileSystem>);
    friend class DOMFileSystemSync;

    ScriptExecutionContext* m_context;
    String m_name;
    mutable OwnPtr<AsyncFileSystem> m_asyncFileSystem;
};

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)

#endif // DOMFileSystemBase_h
