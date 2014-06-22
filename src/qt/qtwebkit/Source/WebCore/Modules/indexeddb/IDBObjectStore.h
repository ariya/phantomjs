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

#ifndef IDBObjectStore_h
#define IDBObjectStore_h

#include "Dictionary.h"
#include "IDBCursor.h"
#include "IDBIndex.h"
#include "IDBKey.h"
#include "IDBKeyRange.h"
#include "IDBMetadata.h"
#include "IDBRequest.h"
#include "IDBTransaction.h"
#include "ScriptWrappable.h"
#include "SerializedScriptValue.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

class DOMStringList;
class IDBAny;

class IDBObjectStore : public ScriptWrappable, public RefCounted<IDBObjectStore> {
public:
    static PassRefPtr<IDBObjectStore> create(const IDBObjectStoreMetadata& metadata, IDBTransaction* transaction)
    {
        return adoptRef(new IDBObjectStore(metadata, transaction));
    }
    ~IDBObjectStore() { }

    // Implement the IDBObjectStore IDL
    int64_t id() const { return m_metadata.id; }
    const String name() const { return m_metadata.name; }
    PassRefPtr<IDBAny> keyPathAny() const { return IDBAny::create(m_metadata.keyPath); }
    const IDBKeyPath keyPath() const { return m_metadata.keyPath; }
    PassRefPtr<DOMStringList> indexNames() const;
    PassRefPtr<IDBTransaction> transaction() const { return m_transaction; }
    bool autoIncrement() const { return m_metadata.autoIncrement; }

    PassRefPtr<IDBRequest> add(ScriptState*, ScriptValue&, ExceptionCode&);
    PassRefPtr<IDBRequest> put(ScriptState*, ScriptValue&, ExceptionCode&);
    PassRefPtr<IDBRequest> openCursor(ScriptExecutionContext* context, ExceptionCode& ec) { return openCursor(context, static_cast<IDBKeyRange*>(0), ec); }
    PassRefPtr<IDBRequest> openCursor(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> keyRange, ExceptionCode& ec) { return openCursor(context, keyRange, IDBCursor::directionNext(), ec); }
    PassRefPtr<IDBRequest> openCursor(ScriptExecutionContext* context, const ScriptValue& key, ExceptionCode& ec) { return openCursor(context, key, IDBCursor::directionNext(), ec); }
    PassRefPtr<IDBRequest> openCursor(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> range, const String& direction, ExceptionCode& ec) { return openCursor(context, range, direction, IDBDatabaseBackendInterface::NormalTask, ec); }
    PassRefPtr<IDBRequest> openCursor(ScriptExecutionContext*, PassRefPtr<IDBKeyRange>, const String& direction, IDBDatabaseBackendInterface::TaskType, ExceptionCode&);
    PassRefPtr<IDBRequest> openCursor(ScriptExecutionContext*, const ScriptValue& key, const String& direction, ExceptionCode&);

    PassRefPtr<IDBRequest> get(ScriptExecutionContext*, const ScriptValue& key, ExceptionCode&);
    PassRefPtr<IDBRequest> get(ScriptExecutionContext*, PassRefPtr<IDBKeyRange>, ExceptionCode&);
    PassRefPtr<IDBRequest> add(ScriptState*, ScriptValue&, const ScriptValue& key, ExceptionCode&);
    PassRefPtr<IDBRequest> put(ScriptState*, ScriptValue&, const ScriptValue& key, ExceptionCode&);
    PassRefPtr<IDBRequest> deleteFunction(ScriptExecutionContext*, PassRefPtr<IDBKeyRange>, ExceptionCode&);
    PassRefPtr<IDBRequest> deleteFunction(ScriptExecutionContext*, const ScriptValue& key, ExceptionCode&);
    PassRefPtr<IDBRequest> clear(ScriptExecutionContext*, ExceptionCode&);

    PassRefPtr<IDBIndex> createIndex(ScriptExecutionContext* context, const String& name, const String& keyPath, const Dictionary& options, ExceptionCode& ec) { return createIndex(context, name, IDBKeyPath(keyPath), options, ec); }
    PassRefPtr<IDBIndex> createIndex(ScriptExecutionContext* context, const String& name, const Vector<String>& keyPath, const Dictionary& options, ExceptionCode& ec) { return createIndex(context, name, IDBKeyPath(keyPath), options, ec); }
    PassRefPtr<IDBIndex> createIndex(ScriptExecutionContext*, const String& name, const IDBKeyPath&, const Dictionary&, ExceptionCode&);
    PassRefPtr<IDBIndex> createIndex(ScriptExecutionContext*, const String& name, const IDBKeyPath&, bool unique, bool multiEntry, ExceptionCode&);

    PassRefPtr<IDBIndex> index(const String& name, ExceptionCode&);
    void deleteIndex(const String& name, ExceptionCode&);

    PassRefPtr<IDBRequest> count(ScriptExecutionContext* context, ExceptionCode& ec) { return count(context, static_cast<IDBKeyRange*>(0), ec); }
    PassRefPtr<IDBRequest> count(ScriptExecutionContext*, PassRefPtr<IDBKeyRange>, ExceptionCode&);
    PassRefPtr<IDBRequest> count(ScriptExecutionContext*, const ScriptValue& key, ExceptionCode&);

    PassRefPtr<IDBRequest> put(IDBDatabaseBackendInterface::PutMode, PassRefPtr<IDBAny> source, ScriptState*, ScriptValue&, const ScriptValue& key, ExceptionCode&);
    PassRefPtr<IDBRequest> put(IDBDatabaseBackendInterface::PutMode, PassRefPtr<IDBAny> source, ScriptState*, ScriptValue&, PassRefPtr<IDBKey>, ExceptionCode&);
    void markDeleted() { m_deleted = true; }
    void transactionFinished();

    IDBObjectStoreMetadata metadata() const { return m_metadata; }
    void setMetadata(const IDBObjectStoreMetadata& metadata) { m_metadata = metadata; }

    typedef Vector<RefPtr<IDBKey> > IndexKeys;
    typedef HashMap<String, IndexKeys> IndexKeyMap;

    IDBDatabaseBackendInterface* backendDB() const;

private:
    IDBObjectStore(const IDBObjectStoreMetadata&, IDBTransaction*);

    int64_t findIndexId(const String& name) const;
    bool containsIndex(const String& name) const
    {
        return findIndexId(name) != IDBIndexMetadata::InvalidId;
    }

    IDBObjectStoreMetadata m_metadata;
    RefPtr<IDBTransaction> m_transaction;
    bool m_deleted;

    typedef HashMap<String, RefPtr<IDBIndex> > IDBIndexMap;
    IDBIndexMap m_indexMap;
};

} // namespace WebCore

#endif

#endif // IDBObjectStore_h
