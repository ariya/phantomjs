/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IDBMetadata_h
#define IDBMetadata_h

#include "IDBKeyPath.h"
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

struct IDBIndexMetadata {
    IDBIndexMetadata() { }
    IDBIndexMetadata(const String& name, int64_t id, const IDBKeyPath& keyPath, bool unique, bool multiEntry)
        : name(name)
        , id(id)
        , keyPath(keyPath)
        , unique(unique)
        , multiEntry(multiEntry) { }
    String name;
    int64_t id;
    IDBKeyPath keyPath;
    bool unique;
    bool multiEntry;

    static const int64_t InvalidId = -1;
};

struct IDBObjectStoreMetadata {
    IDBObjectStoreMetadata() { }
    IDBObjectStoreMetadata(const String& name, int64_t id, const IDBKeyPath& keyPath, bool autoIncrement, int64_t maxIndexId)
        : name(name)
        , id(id)
        , keyPath(keyPath)
        , autoIncrement(autoIncrement)
        , maxIndexId(maxIndexId)
    {
    }
    String name;
    int64_t id;
    IDBKeyPath keyPath;
    bool autoIncrement;
    int64_t maxIndexId;

    static const int64_t InvalidId = -1;

    typedef HashMap<int64_t, IDBIndexMetadata> IndexMap;
    IndexMap indexes;

};

struct IDBDatabaseMetadata {
    // FIXME: These can probably be collapsed into 0.
    enum {
        NoIntVersion = -1,
        DefaultIntVersion = 0
    };

    typedef HashMap<int64_t, IDBObjectStoreMetadata> ObjectStoreMap;

    IDBDatabaseMetadata()
        : intVersion(NoIntVersion)
    {
    }
    IDBDatabaseMetadata(const String& name, int64_t id, const String& version, int64_t intVersion, int64_t maxObjectStoreId)
        : name(name)
        , id(id)
        , version(version)
        , intVersion(intVersion)
        , maxObjectStoreId(maxObjectStoreId)
    {
    }

    String name;
    int64_t id;
    String version;
    int64_t intVersion;
    int64_t maxObjectStoreId;

    ObjectStoreMap objectStores;
};

}

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBMetadata_h
