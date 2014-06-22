/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
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

#ifndef IDBIndex_h
#define IDBIndex_h

#include "IDBCursor.h"
#include "IDBDatabase.h"
#include "IDBKeyPath.h"
#include "IDBKeyRange.h"
#include "IDBMetadata.h"
#include "IDBObjectStore.h"
#include "IDBRequest.h"
#include "ScriptWrappable.h"
#include <wtf/Forward.h>
#include <wtf/text/WTFString.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

class IDBObjectStore;

class IDBIndex : public ScriptWrappable, public RefCounted<IDBIndex> {
public:
    static PassRefPtr<IDBIndex> create(const IDBIndexMetadata& metadata, IDBObjectStore* objectStore, IDBTransaction* transaction)
    {
        return adoptRef(new IDBIndex(metadata, objectStore, transaction));
    }
    ~IDBIndex();

    // Implement the IDL
    const String name() const { return m_metadata.name; }
    PassRefPtr<IDBObjectStore> objectStore() const { return m_objectStore; }
    PassRefPtr<IDBAny> keyPathAny() const { return IDBAny::create(m_metadata.keyPath); }
    const IDBKeyPath keyPath() const { return m_metadata.keyPath; }
    bool unique() const { return m_metadata.unique; }
    bool multiEntry() const { return m_metadata.multiEntry; }
    int64_t id() const { return m_metadata.id; }

    // FIXME: Try to modify the code generator so this is unneeded.
    PassRefPtr<IDBRequest> openCursor(ScriptExecutionContext* context, ExceptionCode& ec) { return openCursor(context, static_cast<IDBKeyRange*>(0), ec); }
    PassRefPtr<IDBRequest> openCursor(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> keyRange, ExceptionCode& ec) { return openCursor(context, keyRange, IDBCursor::directionNext(), ec); }
    PassRefPtr<IDBRequest> openCursor(ScriptExecutionContext* context, const ScriptValue& key, ExceptionCode& ec) { return openCursor(context, key, IDBCursor::directionNext(), ec); }
    PassRefPtr<IDBRequest> openCursor(ScriptExecutionContext*, PassRefPtr<IDBKeyRange>, const String& direction, ExceptionCode&);
    PassRefPtr<IDBRequest> openCursor(ScriptExecutionContext*, const ScriptValue& key, const String& direction, ExceptionCode&);
    PassRefPtr<IDBRequest> count(ScriptExecutionContext* context, ExceptionCode& ec) { return count(context, static_cast<IDBKeyRange*>(0), ec); }
    PassRefPtr<IDBRequest> count(ScriptExecutionContext*, PassRefPtr<IDBKeyRange>, ExceptionCode&);
    PassRefPtr<IDBRequest> count(ScriptExecutionContext*, const ScriptValue& key, ExceptionCode&);

    PassRefPtr<IDBRequest> openKeyCursor(ScriptExecutionContext* context, ExceptionCode& ec) { return openKeyCursor(context, static_cast<IDBKeyRange*>(0), ec); }
    PassRefPtr<IDBRequest> openKeyCursor(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> keyRange, ExceptionCode& ec) { return openKeyCursor(context, keyRange, IDBCursor::directionNext(), ec); }
    PassRefPtr<IDBRequest> openKeyCursor(ScriptExecutionContext* context, const ScriptValue& key, ExceptionCode& ec) { return openKeyCursor(context, key, IDBCursor::directionNext(), ec); }
    PassRefPtr<IDBRequest> openKeyCursor(ScriptExecutionContext*, PassRefPtr<IDBKeyRange>, const String& direction, ExceptionCode&);
    PassRefPtr<IDBRequest> openKeyCursor(ScriptExecutionContext*, const ScriptValue& key, const String& direction, ExceptionCode&);

    PassRefPtr<IDBRequest> get(ScriptExecutionContext*, PassRefPtr<IDBKeyRange>, ExceptionCode&);
    PassRefPtr<IDBRequest> get(ScriptExecutionContext*, const ScriptValue& key, ExceptionCode&);
    PassRefPtr<IDBRequest> getKey(ScriptExecutionContext*, PassRefPtr<IDBKeyRange>, ExceptionCode&);
    PassRefPtr<IDBRequest> getKey(ScriptExecutionContext*, const ScriptValue& key, ExceptionCode&);

    void markDeleted() { m_deleted = true; }

    IDBDatabaseBackendInterface* backendDB() const;

private:
    IDBIndex(const IDBIndexMetadata&, IDBObjectStore*, IDBTransaction*);

    IDBIndexMetadata m_metadata;
    RefPtr<IDBObjectStore> m_objectStore;
    RefPtr<IDBTransaction> m_transaction;
    bool m_deleted;
};

} // namespace WebCore

#endif

#endif // IDBIndex_h
