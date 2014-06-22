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

#include "config.h"
#include "IDBObjectStore.h"

#if ENABLE(INDEXED_DATABASE)

#include "DOMStringList.h"
#include "IDBAny.h"
#include "IDBBindingUtilities.h"
#include "IDBCursorWithValue.h"
#include "IDBDatabase.h"
#include "IDBDatabaseException.h"
#include "IDBIndex.h"
#include "IDBKey.h"
#include "IDBKeyPath.h"
#include "IDBKeyRange.h"
#include "IDBTracing.h"
#include "IDBTransaction.h"
#include "ScriptExecutionContext.h"
#include "SerializedScriptValue.h"
#include "SharedBuffer.h"

namespace WebCore {

static const unsigned short defaultDirection = IndexedDB::CursorNext;

IDBObjectStore::IDBObjectStore(const IDBObjectStoreMetadata& metadata, IDBTransaction* transaction)
    : m_metadata(metadata)
    , m_transaction(transaction)
    , m_deleted(false)
{
    ASSERT(m_transaction);
    // We pass a reference to this object before it can be adopted.
    relaxAdoptionRequirement();
}

PassRefPtr<DOMStringList> IDBObjectStore::indexNames() const
{
    IDB_TRACE("IDBObjectStore::indexNames");
    RefPtr<DOMStringList> indexNames = DOMStringList::create();
    for (IDBObjectStoreMetadata::IndexMap::const_iterator it = m_metadata.indexes.begin(); it != m_metadata.indexes.end(); ++it)
        indexNames->append(it->value.name);
    indexNames->sort();
    return indexNames.release();
}

PassRefPtr<IDBRequest> IDBObjectStore::get(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> keyRange, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::get");
    if (m_deleted) {
        ec = IDBDatabaseException::InvalidStateError;
        return 0;
    }
    if (!keyRange) {
        ec = IDBDatabaseException::DataError;
        return 0;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TransactionInactiveError;
        return 0;
    }
    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    backendDB()->get(m_transaction->id(), id(), IDBIndexMetadata::InvalidId, keyRange, false, request);
    return request.release();
}

PassRefPtr<IDBRequest> IDBObjectStore::get(ScriptExecutionContext* context, const ScriptValue& key, ExceptionCode& ec)
{
    RefPtr<IDBKeyRange> keyRange = IDBKeyRange::only(context, key, ec);
    if (ec)
        return 0;
    return get(context, keyRange.release(), ec);
}

static void generateIndexKeysForValue(DOMRequestState* requestState, const IDBIndexMetadata& indexMetadata, const ScriptValue& objectValue, IDBObjectStore::IndexKeys* indexKeys)
{
    ASSERT(indexKeys);
    RefPtr<IDBKey> indexKey = createIDBKeyFromScriptValueAndKeyPath(requestState, objectValue, indexMetadata.keyPath);

    if (!indexKey)
        return;

    if (!indexMetadata.multiEntry || indexKey->type() != IDBKey::ArrayType) {
        if (!indexKey->isValid())
            return;

        indexKeys->append(indexKey);
    } else {
        ASSERT(indexMetadata.multiEntry);
        ASSERT(indexKey->type() == IDBKey::ArrayType);
        indexKey = IDBKey::createMultiEntryArray(indexKey->array());

        for (size_t i = 0; i < indexKey->array().size(); ++i)
            indexKeys->append(indexKey->array()[i]);
    }
}

PassRefPtr<IDBRequest> IDBObjectStore::add(ScriptState* state, ScriptValue& value, const ScriptValue& key, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::add");
    return put(IDBDatabaseBackendInterface::AddOnly, IDBAny::create(this), state, value, key, ec);
}

PassRefPtr<IDBRequest> IDBObjectStore::add(ScriptState* state, ScriptValue& value, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::add");
    return put(IDBDatabaseBackendInterface::AddOnly, IDBAny::create(this), state, value, static_cast<IDBKey*>(0), ec);
}

PassRefPtr<IDBRequest> IDBObjectStore::put(ScriptState* state, ScriptValue& value, const ScriptValue& key, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::put");
    return put(IDBDatabaseBackendInterface::AddOrUpdate, IDBAny::create(this), state, value, key, ec);
}

PassRefPtr<IDBRequest> IDBObjectStore::put(ScriptState* state, ScriptValue& value, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::put");
    return put(IDBDatabaseBackendInterface::AddOrUpdate, IDBAny::create(this), state, value, static_cast<IDBKey*>(0), ec);
}

PassRefPtr<IDBRequest> IDBObjectStore::put(IDBDatabaseBackendInterface::PutMode putMode, PassRefPtr<IDBAny> source, ScriptState* state, ScriptValue& value, const ScriptValue& keyValue, ExceptionCode& ec)
{
    ScriptExecutionContext* context = scriptExecutionContextFromScriptState(state);
    DOMRequestState requestState(context);
    RefPtr<IDBKey> key = scriptValueToIDBKey(&requestState, keyValue);
    return put(putMode, source, state, value, key.release(), ec);
}

PassRefPtr<IDBRequest> IDBObjectStore::put(IDBDatabaseBackendInterface::PutMode putMode, PassRefPtr<IDBAny> source, ScriptState* state, ScriptValue& value, PassRefPtr<IDBKey> prpKey, ExceptionCode& ec)
{
    RefPtr<IDBKey> key = prpKey;
    if (m_deleted) {
        ec = IDBDatabaseException::InvalidStateError;
        return 0;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TransactionInactiveError;
        return 0;
    }
    if (m_transaction->isReadOnly()) {
        ec = IDBDatabaseException::ReadOnlyError;
        return 0;
    }

    // FIXME: Expose the JS engine exception state through ScriptState.
    bool didThrow = false;
    RefPtr<SerializedScriptValue> serializedValue = value.serialize(state, 0, 0, didThrow);
    if (didThrow) {
        // Setting an explicit ExceptionCode here would defer handling the already thrown exception.
        return 0;
    }

    if (serializedValue->blobURLs().size() > 0) {
        // FIXME: Add Blob/File/FileList support
        ec = IDBDatabaseException::DataCloneError;
        return 0;
    }

    const IDBKeyPath& keyPath = m_metadata.keyPath;
    const bool usesInLineKeys = !keyPath.isNull();
    const bool hasKeyGenerator = autoIncrement();

    ScriptExecutionContext* context = scriptExecutionContextFromScriptState(state);
    DOMRequestState requestState(context);

    if (putMode != IDBDatabaseBackendInterface::CursorUpdate && usesInLineKeys && key) {
        ec = IDBDatabaseException::DataError;
        return 0;
    }
    if (!usesInLineKeys && !hasKeyGenerator && !key) {
        ec = IDBDatabaseException::DataError;
        return 0;
    }
    if (usesInLineKeys) {
        RefPtr<IDBKey> keyPathKey = createIDBKeyFromScriptValueAndKeyPath(&requestState, value, keyPath);
        if (keyPathKey && !keyPathKey->isValid()) {
            ec = IDBDatabaseException::DataError;
            return 0;
        }
        if (!hasKeyGenerator && !keyPathKey) {
            ec = IDBDatabaseException::DataError;
            return 0;
        }
        if (hasKeyGenerator && !keyPathKey) {
            if (!canInjectIDBKeyIntoScriptValue(&requestState, value, keyPath)) {
                ec = IDBDatabaseException::DataError;
                return 0;
            }
        }
        if (keyPathKey)
            key = keyPathKey;
    }
    if (key && !key->isValid()) {
        ec = IDBDatabaseException::DataError;
        return 0;
    }

    Vector<int64_t> indexIds;
    Vector<IndexKeys> indexKeys;
    for (IDBObjectStoreMetadata::IndexMap::const_iterator it = m_metadata.indexes.begin(); it != m_metadata.indexes.end(); ++it) {
        IndexKeys keys;
        generateIndexKeysForValue(&requestState, it->value, value, &keys);
        indexIds.append(it->key);
        indexKeys.append(keys);
    }

    RefPtr<IDBRequest> request = IDBRequest::create(context, source, m_transaction.get());
    Vector<uint8_t> valueBytes = serializedValue->toWireBytes();
    // This is a hack to account for disagreements about whether SerializedScriptValue should deal in Vector<uint8_t> or Vector<char>.
    // See https://lists.webkit.org/pipermail/webkit-dev/2013-February/023682.html
    Vector<char>* valueBytesSigned = reinterpret_cast<Vector<char>*>(&valueBytes);
    RefPtr<SharedBuffer> valueBuffer = SharedBuffer::adoptVector(*valueBytesSigned);
    backendDB()->put(m_transaction->id(), id(), valueBuffer, key.release(), static_cast<IDBDatabaseBackendInterface::PutMode>(putMode), request, indexIds, indexKeys);
    return request.release();
}

PassRefPtr<IDBRequest> IDBObjectStore::deleteFunction(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> keyRange, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::delete");
    if (m_deleted) {
        ec = IDBDatabaseException::InvalidStateError;
        return 0;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TransactionInactiveError;
        return 0;
    }
    if (m_transaction->isReadOnly()) {
        ec = IDBDatabaseException::ReadOnlyError;
        return 0;
    }
    if (!keyRange) {
        ec = IDBDatabaseException::DataError;
        return 0;
    }

    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    backendDB()->deleteRange(m_transaction->id(), id(), keyRange, request);
    return request.release();
}

PassRefPtr<IDBRequest> IDBObjectStore::deleteFunction(ScriptExecutionContext* context, const ScriptValue& key, ExceptionCode& ec)
{
    RefPtr<IDBKeyRange> keyRange = IDBKeyRange::only(context, key, ec);
    if (ec)
        return 0;
    return deleteFunction(context, keyRange.release(), ec);
}

PassRefPtr<IDBRequest> IDBObjectStore::clear(ScriptExecutionContext* context, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::clear");
    if (m_deleted) {
        ec = IDBDatabaseException::InvalidStateError;
        return 0;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TransactionInactiveError;
        return 0;
    }
    if (m_transaction->isReadOnly()) {
        ec = IDBDatabaseException::ReadOnlyError;
        return 0;
    }

    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    backendDB()->clear(m_transaction->id(), id(), request);
    return request.release();
}

namespace {
// This class creates the index keys for a given index by extracting
// them from the SerializedScriptValue, for all the existing values in
// the objectStore. It only needs to be kept alive by virtue of being
// a listener on an IDBRequest object, in the same way that JavaScript
// cursor success handlers are kept alive.
class IndexPopulator : public EventListener {
public:
    static PassRefPtr<IndexPopulator> create(PassRefPtr<IDBDatabaseBackendInterface> backend, int64_t transactionId, int64_t objectStoreId, const IDBIndexMetadata& indexMetadata)
    {
        return adoptRef(new IndexPopulator(backend, transactionId, objectStoreId, indexMetadata));
    }

    virtual bool operator==(const EventListener& other)
    {
        return this == &other;
    }

private:
    IndexPopulator(PassRefPtr<IDBDatabaseBackendInterface> backend, int64_t transactionId, int64_t objectStoreId, const IDBIndexMetadata& indexMetadata)
        : EventListener(CPPEventListenerType)
        , m_databaseBackend(backend)
        , m_transactionId(transactionId)
        , m_objectStoreId(objectStoreId)
        , m_indexMetadata(indexMetadata)
    {
    }

    virtual void handleEvent(ScriptExecutionContext*, Event* event)
    {
        ASSERT(event->type() == eventNames().successEvent);
        EventTarget* target = event->target();
        IDBRequest* request = static_cast<IDBRequest*>(target);

        RefPtr<IDBAny> cursorAny = request->result(ASSERT_NO_EXCEPTION);
        RefPtr<IDBCursorWithValue> cursor;
        if (cursorAny->type() == IDBAny::IDBCursorWithValueType)
            cursor = cursorAny->idbCursorWithValue();

        Vector<int64_t, 1> indexIds;
        indexIds.append(m_indexMetadata.id);
        if (cursor) {
            cursor->continueFunction(static_cast<IDBKey*>(0), ASSERT_NO_EXCEPTION);

            RefPtr<IDBKey> primaryKey = cursor->idbPrimaryKey();
            ScriptValue value = cursor->value();

            IDBObjectStore::IndexKeys indexKeys;
            generateIndexKeysForValue(request->requestState(), m_indexMetadata, value, &indexKeys);

            Vector<IDBObjectStore::IndexKeys, 1> indexKeysList;
            indexKeysList.append(indexKeys);

            m_databaseBackend->setIndexKeys(m_transactionId, m_objectStoreId, primaryKey, indexIds, indexKeysList);
        } else {
            // Now that we are done indexing, tell the backend to go
            // back to processing tasks of type NormalTask.
            m_databaseBackend->setIndexesReady(m_transactionId, m_objectStoreId, indexIds);
            m_databaseBackend.clear();
        }

    }

    RefPtr<IDBDatabaseBackendInterface> m_databaseBackend;
    const int64_t m_transactionId;
    const int64_t m_objectStoreId;
    const IDBIndexMetadata m_indexMetadata;
};
}

PassRefPtr<IDBIndex> IDBObjectStore::createIndex(ScriptExecutionContext* context, const String& name, const IDBKeyPath& keyPath, const Dictionary& options, ExceptionCode& ec)
{
    bool unique = false;
    options.get("unique", unique);

    bool multiEntry = false;
    options.get("multiEntry", multiEntry);

    return createIndex(context, name, keyPath, unique, multiEntry, ec);
}

PassRefPtr<IDBIndex> IDBObjectStore::createIndex(ScriptExecutionContext* context, const String& name, const IDBKeyPath& keyPath, bool unique, bool multiEntry, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::createIndex");
    if (!m_transaction->isVersionChange() || m_deleted) {
        ec = IDBDatabaseException::InvalidStateError;
        return 0;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TransactionInactiveError;
        return 0;
    }
    if (!keyPath.isValid()) {
        ec = IDBDatabaseException::SyntaxError;
        return 0;
    }
    if (name.isNull()) {
        ec = TypeError;
        return 0;
    }
    if (containsIndex(name)) {
        ec = IDBDatabaseException::ConstraintError;
        return 0;
    }

    if (keyPath.type() == IDBKeyPath::ArrayType && multiEntry) {
        ec = IDBDatabaseException::InvalidAccessError;
        return 0;
    }

    int64_t indexId = m_metadata.maxIndexId + 1;
    backendDB()->createIndex(m_transaction->id(), id(), indexId, name, keyPath, unique, multiEntry);

    ++m_metadata.maxIndexId;

    IDBIndexMetadata metadata(name, indexId, keyPath, unique, multiEntry);
    RefPtr<IDBIndex> index = IDBIndex::create(metadata, this, m_transaction.get());
    m_indexMap.set(name, index);
    m_metadata.indexes.set(indexId, metadata);

    ASSERT(!ec);
    if (ec)
        return 0;

    RefPtr<IDBRequest> indexRequest = openCursor(context, static_cast<IDBKeyRange*>(0), IDBCursor::directionNext(), IDBDatabaseBackendInterface::PreemptiveTask, ec);
    ASSERT(!ec);
    if (ec)
        return 0;
    indexRequest->preventPropagation();

    // This is kept alive by being the success handler of the request, which is in turn kept alive by the owning transaction.
    RefPtr<IndexPopulator> indexPopulator = IndexPopulator::create(backendDB(), m_transaction->id(), id(), metadata);
    indexRequest->setOnsuccess(indexPopulator);

    return index.release();
}

PassRefPtr<IDBIndex> IDBObjectStore::index(const String& name, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::index");
    if (m_deleted) {
        ec = IDBDatabaseException::InvalidStateError;
        return 0;
    }
    if (m_transaction->isFinished()) {
        ec = IDBDatabaseException::InvalidStateError;
        return 0;
    }

    IDBIndexMap::iterator it = m_indexMap.find(name);
    if (it != m_indexMap.end())
        return it->value;

    int64_t indexId = findIndexId(name);
    if (indexId == IDBIndexMetadata::InvalidId) {
        ec = IDBDatabaseException::NotFoundError;
        return 0;
    }

    const IDBIndexMetadata* indexMetadata(0);
    for (IDBObjectStoreMetadata::IndexMap::const_iterator it = m_metadata.indexes.begin(); it != m_metadata.indexes.end(); ++it) {
        if (it->value.name == name) {
            indexMetadata = &it->value;
            break;
        }
    }
    ASSERT(indexMetadata);
    ASSERT(indexMetadata->id != IDBIndexMetadata::InvalidId);

    RefPtr<IDBIndex> index = IDBIndex::create(*indexMetadata, this, m_transaction.get());
    m_indexMap.set(name, index);
    return index.release();
}

void IDBObjectStore::deleteIndex(const String& name, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::deleteIndex");
    if (!m_transaction->isVersionChange() || m_deleted) {
        ec = IDBDatabaseException::InvalidStateError;
        return;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TransactionInactiveError;
        return;
    }
    int64_t indexId = findIndexId(name);
    if (indexId == IDBIndexMetadata::InvalidId) {
        ec = IDBDatabaseException::NotFoundError;
        return;
    }

    backendDB()->deleteIndex(m_transaction->id(), id(), indexId);

    m_metadata.indexes.remove(indexId);
    IDBIndexMap::iterator it = m_indexMap.find(name);
    if (it != m_indexMap.end()) {
        it->value->markDeleted();
        m_indexMap.remove(name);
    }
}

PassRefPtr<IDBRequest> IDBObjectStore::openCursor(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> range, const String& directionString, IDBDatabaseBackendInterface::TaskType taskType, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::openCursor");
    if (m_deleted) {
        ec = IDBDatabaseException::InvalidStateError;
        return 0;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TransactionInactiveError;
        return 0;
    }
    IndexedDB::CursorDirection direction = IDBCursor::stringToDirection(directionString, ec);
    if (ec)
        return 0;

    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    request->setCursorDetails(IndexedDB::CursorKeyAndValue, direction);

    backendDB()->openCursor(m_transaction->id(), id(), IDBIndexMetadata::InvalidId, range, direction, false, static_cast<IDBDatabaseBackendInterface::TaskType>(taskType), request);
    return request.release();
}

PassRefPtr<IDBRequest> IDBObjectStore::openCursor(ScriptExecutionContext* context, const ScriptValue& key, const String& direction, ExceptionCode& ec)
{
    RefPtr<IDBKeyRange> keyRange = IDBKeyRange::only(context, key, ec);
    if (ec)
        return 0;
    return openCursor(context, keyRange.release(), direction, ec);
}

PassRefPtr<IDBRequest> IDBObjectStore::count(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> range, ExceptionCode& ec)
{
    IDB_TRACE("IDBObjectStore::count");
    if (m_deleted) {
        ec = IDBDatabaseException::InvalidStateError;
        return 0;
    }
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TransactionInactiveError;
        return 0;
    }
    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    backendDB()->count(m_transaction->id(), id(), IDBIndexMetadata::InvalidId, range, request);
    return request.release();
}

PassRefPtr<IDBRequest> IDBObjectStore::count(ScriptExecutionContext* context, const ScriptValue& key, ExceptionCode& ec)
{
    RefPtr<IDBKeyRange> keyRange = IDBKeyRange::only(context, key, ec);
    if (ec)
        return 0;
    return count(context, keyRange.release(), ec);
}

void IDBObjectStore::transactionFinished()
{
    ASSERT(m_transaction->isFinished());

    // Break reference cycles.
    m_indexMap.clear();
}

int64_t IDBObjectStore::findIndexId(const String& name) const
{
    for (IDBObjectStoreMetadata::IndexMap::const_iterator it = m_metadata.indexes.begin(); it != m_metadata.indexes.end(); ++it) {
        if (it->value.name == name) {
            ASSERT(it->key != IDBIndexMetadata::InvalidId);
            return it->key;
        }
    }
    return IDBIndexMetadata::InvalidId;
}

IDBDatabaseBackendInterface* IDBObjectStore::backendDB() const
{
    return m_transaction->backendDB();
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
