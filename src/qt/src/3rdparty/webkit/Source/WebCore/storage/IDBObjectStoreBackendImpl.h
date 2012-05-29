/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef IDBObjectStoreBackendImpl_h
#define IDBObjectStoreBackendImpl_h

#include "IDBObjectStoreBackendInterface.h"
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

class IDBBackingStore;
class IDBDatabaseBackendImpl;
class IDBIndexBackendImpl;
class IDBTransactionBackendInterface;
class ScriptExecutionContext;

class IDBObjectStoreBackendImpl : public IDBObjectStoreBackendInterface {
public:
    static PassRefPtr<IDBObjectStoreBackendImpl> create(IDBBackingStore* backingStore, int64_t databaseId, int64_t id, const String& name, const String& keyPath, bool autoIncrement)
    {
        return adoptRef(new IDBObjectStoreBackendImpl(backingStore, databaseId, id, name, keyPath, autoIncrement));
    }
    static PassRefPtr<IDBObjectStoreBackendImpl> create(IDBBackingStore* backingStore, int64_t databaseId, const String& name, const String& keyPath, bool autoIncrement)
    {
        return adoptRef(new IDBObjectStoreBackendImpl(backingStore, databaseId, name, keyPath, autoIncrement));
    }
    virtual ~IDBObjectStoreBackendImpl();

    static const int64_t InvalidId = 0;
    int64_t id() const
    {
        ASSERT(m_id != InvalidId);
        return m_id;
    }
    void setId(int64_t id) { m_id = id; }
    int64_t databaseId() const { return m_databaseId; }

    virtual String name() const { return m_name; }
    virtual String keyPath() const { return m_keyPath; }
    virtual PassRefPtr<DOMStringList> indexNames() const;
    virtual bool autoIncrement() const { return m_autoIncrement; }

    virtual void get(PassRefPtr<IDBKey> key, PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, ExceptionCode&);
    virtual void put(PassRefPtr<SerializedScriptValue>, PassRefPtr<IDBKey>, PutMode, PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, ExceptionCode&);
    virtual void deleteFunction(PassRefPtr<IDBKey> key, PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, ExceptionCode&);
    virtual void clear(PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, ExceptionCode&);

    virtual PassRefPtr<IDBIndexBackendInterface> createIndex(const String& name, const String& keyPath, bool unique, IDBTransactionBackendInterface*, ExceptionCode&);
    virtual PassRefPtr<IDBIndexBackendInterface> index(const String& name, ExceptionCode&);
    virtual void deleteIndex(const String& name, IDBTransactionBackendInterface*, ExceptionCode&);

    virtual void openCursor(PassRefPtr<IDBKeyRange> range, unsigned short direction, PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, ExceptionCode&);

private:
    IDBObjectStoreBackendImpl(IDBBackingStore*, int64_t databaseId, int64_t id, const String& name, const String& keyPath, bool autoIncrement);
    IDBObjectStoreBackendImpl(IDBBackingStore*, int64_t databaseId, const String& name, const String& keyPath, bool autoIncrement);

    void loadIndexes();
    PassRefPtr<IDBKey> genAutoIncrementKey();
    void resetAutoIncrementKeyCache() { m_autoIncrementNumber = -1; }
    static PassRefPtr<IDBKey> selectKeyForPut(IDBObjectStoreBackendImpl*, IDBKey*, PutMode, IDBCallbacks*, RefPtr<SerializedScriptValue>&);

    static void getInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBKey> key, PassRefPtr<IDBCallbacks>);
    static void putInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<SerializedScriptValue>, PassRefPtr<IDBKey>, PutMode, PassRefPtr<IDBCallbacks>, PassRefPtr<IDBTransactionBackendInterface>);
    static void deleteInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBKey> key, PassRefPtr<IDBCallbacks>);
    static void clearInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBCallbacks>);
    static void createIndexInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBIndexBackendImpl>, PassRefPtr<IDBTransactionBackendInterface>);
    static void deleteIndexInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBIndexBackendImpl>, PassRefPtr<IDBTransactionBackendInterface>);
    static void openCursorInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBKeyRange> range, unsigned short direction, PassRefPtr<IDBCallbacks>, PassRefPtr<IDBTransactionBackendInterface>);

    // These are used as setVersion transaction abort tasks.
    static void removeIndexFromMap(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBIndexBackendImpl>);
    static void addIndexToMap(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBIndexBackendImpl>);

    RefPtr<IDBBackingStore> m_backingStore;

    int64_t m_databaseId;
    int64_t m_id;
    String m_name;
    String m_keyPath;
    bool m_autoIncrement;

    typedef HashMap<String, RefPtr<IDBIndexBackendImpl> > IndexMap;
    IndexMap m_indexes;
    int m_autoIncrementNumber;
};

} // namespace WebCore

#endif

#endif // IDBObjectStoreBackendImpl_h
