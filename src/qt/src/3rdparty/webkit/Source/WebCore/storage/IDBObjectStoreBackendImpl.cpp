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

#include "config.h"
#include "IDBObjectStoreBackendImpl.h"

#if ENABLE(INDEXED_DATABASE)

#include "CrossThreadTask.h"
#include "DOMStringList.h"
#include "IDBBackingStore.h"
#include "IDBBindingUtilities.h"
#include "IDBCallbacks.h"
#include "IDBCursorBackendImpl.h"
#include "IDBDatabaseBackendImpl.h"
#include "IDBDatabaseException.h"
#include "IDBIndexBackendImpl.h"
#include "IDBKey.h"
#include "IDBKeyPath.h"
#include "IDBKeyPathBackendImpl.h"
#include "IDBKeyRange.h"
#include "IDBTransactionBackendInterface.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

IDBObjectStoreBackendImpl::~IDBObjectStoreBackendImpl()
{
}

IDBObjectStoreBackendImpl::IDBObjectStoreBackendImpl(IDBBackingStore* backingStore, int64_t databaseId, int64_t id, const String& name, const String& keyPath, bool autoIncrement)
    : m_backingStore(backingStore)
    , m_databaseId(databaseId)
    , m_id(id)
    , m_name(name)
    , m_keyPath(keyPath)
    , m_autoIncrement(autoIncrement)
    , m_autoIncrementNumber(-1)
{
    loadIndexes();
}

IDBObjectStoreBackendImpl::IDBObjectStoreBackendImpl(IDBBackingStore* backingStore, int64_t databaseId, const String& name, const String& keyPath, bool autoIncrement)
    : m_backingStore(backingStore)
    , m_databaseId(databaseId)
    , m_id(InvalidId)
    , m_name(name)
    , m_keyPath(keyPath)
    , m_autoIncrement(autoIncrement)
    , m_autoIncrementNumber(-1)
{
}

PassRefPtr<DOMStringList> IDBObjectStoreBackendImpl::indexNames() const
{
    RefPtr<DOMStringList> indexNames = DOMStringList::create();
    for (IndexMap::const_iterator it = m_indexes.begin(); it != m_indexes.end(); ++it)
        indexNames->append(it->first);
    return indexNames.release();
}

void IDBObjectStoreBackendImpl::get(PassRefPtr<IDBKey> prpKey, PassRefPtr<IDBCallbacks> prpCallbacks, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    RefPtr<IDBObjectStoreBackendImpl> objectStore = this;
    RefPtr<IDBKey> key = prpKey;
    RefPtr<IDBCallbacks> callbacks = prpCallbacks;
    if (!transaction->scheduleTask(createCallbackTask(&IDBObjectStoreBackendImpl::getInternal, objectStore, key, callbacks)))
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
}

void IDBObjectStoreBackendImpl::getInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl> objectStore, PassRefPtr<IDBKey> key, PassRefPtr<IDBCallbacks> callbacks)
{
    String wireData = objectStore->m_backingStore->getObjectStoreRecord(objectStore->m_databaseId, objectStore->id(), *key);
    if (wireData.isNull()) {
        callbacks->onSuccess(SerializedScriptValue::undefinedValue());
        return;
    }

    callbacks->onSuccess(SerializedScriptValue::createFromWire(wireData));
}

static PassRefPtr<IDBKey> fetchKeyFromKeyPath(SerializedScriptValue* value, const String& keyPath)
{
    Vector<RefPtr<SerializedScriptValue> > values;
    values.append(value);
    Vector<RefPtr<IDBKey> > keys;
    IDBKeyPathBackendImpl::createIDBKeysFromSerializedValuesAndKeyPath(values, keyPath, keys);
    if (keys.isEmpty())
        return 0;
    ASSERT(keys.size() == 1);
    return keys[0].release();
}

static PassRefPtr<SerializedScriptValue> injectKeyIntoKeyPath(PassRefPtr<IDBKey> key, PassRefPtr<SerializedScriptValue> value, const String& keyPath)
{
    return IDBKeyPathBackendImpl::injectIDBKeyIntoSerializedValue(key, value, keyPath);
}

void IDBObjectStoreBackendImpl::put(PassRefPtr<SerializedScriptValue> prpValue, PassRefPtr<IDBKey> prpKey, PutMode putMode, PassRefPtr<IDBCallbacks> prpCallbacks, IDBTransactionBackendInterface* transactionPtr, ExceptionCode& ec)
{
    if (transactionPtr->mode() == IDBTransaction::READ_ONLY) {
        ec = IDBDatabaseException::READ_ONLY_ERR;
        return;
    }

    RefPtr<IDBObjectStoreBackendImpl> objectStore = this;
    RefPtr<SerializedScriptValue> value = prpValue;
    RefPtr<IDBKey> key = prpKey;
    RefPtr<IDBCallbacks> callbacks = prpCallbacks;
    RefPtr<IDBTransactionBackendInterface> transaction = transactionPtr;

    if (key && (key->type() == IDBKey::NullType)) {
        ec = IDBDatabaseException::DATA_ERR;
        return;
    }
    // FIXME: This should throw a SERIAL_ERR on structured clone problems.
    // FIXME: This should throw a DATA_ERR when the wrong key/keyPath data is supplied.
    if (!transaction->scheduleTask(createCallbackTask(&IDBObjectStoreBackendImpl::putInternal, objectStore, value, key, putMode, callbacks, transaction)))
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
}

PassRefPtr<IDBKey> IDBObjectStoreBackendImpl::selectKeyForPut(IDBObjectStoreBackendImpl* objectStore, IDBKey* key, PutMode putMode, IDBCallbacks* callbacks, RefPtr<SerializedScriptValue>& value)
{
    if (putMode == CursorUpdate)
        ASSERT(key);

    const bool autoIncrement = objectStore->autoIncrement();
    const bool hasKeyPath = !objectStore->m_keyPath.isNull();

    if (hasKeyPath && key && putMode != CursorUpdate) {
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::DATA_ERR, "A key was supplied for an objectStore that has a keyPath."));
        return 0;
    }

    if (autoIncrement && key) {
        objectStore->resetAutoIncrementKeyCache();
        return key;
    }

    if (autoIncrement) {
        ASSERT(!key);
        if (!hasKeyPath)
            return objectStore->genAutoIncrementKey();

        RefPtr<IDBKey> keyPathKey = fetchKeyFromKeyPath(value.get(), objectStore->m_keyPath);
        if (keyPathKey) {
            objectStore->resetAutoIncrementKeyCache();
            return keyPathKey;
        }

        RefPtr<IDBKey> autoIncKey = objectStore->genAutoIncrementKey();
        RefPtr<SerializedScriptValue> valueAfterInjection = injectKeyIntoKeyPath(autoIncKey, value, objectStore->m_keyPath);
        if (!valueAfterInjection) {
            callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::DATA_ERR, "The generated key could not be inserted into the object using the keyPath."));
            return 0;
        }
        value = valueAfterInjection;
        return autoIncKey.release();
    }

    if (hasKeyPath) {
        RefPtr<IDBKey> keyPathKey = fetchKeyFromKeyPath(value.get(), objectStore->m_keyPath);

        if (!keyPathKey) {
            callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::DATA_ERR, "The key could not be fetched from the keyPath."));
            return 0;
        }

        if (putMode == CursorUpdate && !keyPathKey->isEqual(key)) {
            callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::DATA_ERR, "The key fetched from the keyPath does not match the key of the cursor."));
            return 0;
        }

        return keyPathKey.release();
    }

    if (!key) {
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::DATA_ERR, "No key supplied"));
        return 0;
    }

    return key;
}

void IDBObjectStoreBackendImpl::putInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl> objectStore, PassRefPtr<SerializedScriptValue> prpValue, PassRefPtr<IDBKey> prpKey, PutMode putMode, PassRefPtr<IDBCallbacks> callbacks, PassRefPtr<IDBTransactionBackendInterface> transaction)
{
    RefPtr<SerializedScriptValue> value = prpValue;
    RefPtr<IDBKey> key = selectKeyForPut(objectStore.get(), prpKey.get(), putMode, callbacks.get(), value);
    if (!key)
        return;

    if (key->type() == IDBKey::NullType) {
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::DATA_ERR, "NULL key is not allowed."));
        return;
    }

    Vector<RefPtr<IDBKey> > indexKeys;
    for (IndexMap::iterator it = objectStore->m_indexes.begin(); it != objectStore->m_indexes.end(); ++it) {
        RefPtr<IDBKey> key = fetchKeyFromKeyPath(value.get(), it->second->keyPath());
        if (!key) {
            callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UNKNOWN_ERR, "The key could not be fetched from an index's keyPath."));
            return;
        }
        if (key->type() == IDBKey::NullType) {
            callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::DATA_ERR, "One of the derived (from a keyPath) keys for an index is NULL."));
            return;
        }
        if (!it->second->addingKeyAllowed(key.get())) {
            callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UNKNOWN_ERR, "One of the derived (from a keyPath) keys for an index does not satisfy its uniqueness requirements."));
            return;
        }
        indexKeys.append(key.release());
    }

    RefPtr<IDBBackingStore::ObjectStoreRecordIdentifier> recordIdentifier = objectStore->m_backingStore->createInvalidRecordIdentifier();
    bool isExistingValue = objectStore->m_backingStore->keyExistsInObjectStore(objectStore->m_databaseId, objectStore->id(), *key, recordIdentifier.get());

    if (putMode == AddOnly && isExistingValue) {
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::CONSTRAINT_ERR, "Key already exists in the object store."));
        return;
    }

    // Before this point, don't do any mutation.  After this point, rollback the transaction in case of error.

    if (!objectStore->m_backingStore->putObjectStoreRecord(objectStore->m_databaseId, objectStore->id(), *key, value->toWireString(), recordIdentifier.get())) {
        // FIXME: The Indexed Database specification does not have an error code dedicated to I/O errors.
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UNKNOWN_ERR, "Error writing data to stable storage."));
        transaction->abort();
        return;
    }

    int i = 0;
    for (IndexMap::iterator it = objectStore->m_indexes.begin(); it != objectStore->m_indexes.end(); ++it, ++i) {
        if (!it->second->hasValidId())
            continue; // The index object has been created, but does not exist in the database yet.

        if (!objectStore->m_backingStore->deleteIndexDataForRecord(objectStore->m_databaseId, objectStore->id(), it->second->id(), recordIdentifier.get())) {
            // FIXME: The Indexed Database specification does not have an error code dedicated to I/O errors.
            callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UNKNOWN_ERR, "Error writing data to stable storage."));
            transaction->abort();
            return;
        }

        if (!objectStore->m_backingStore->putIndexDataForRecord(objectStore->m_databaseId, objectStore->id(), it->second->id(), *indexKeys[i], recordIdentifier.get())) {
            // FIXME: The Indexed Database specification does not have an error code dedicated to I/O errors.
            callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UNKNOWN_ERR, "Error writing data to stable storage."));
            transaction->abort();
            return;
        }
    }

    callbacks->onSuccess(key.get());
}

void IDBObjectStoreBackendImpl::deleteFunction(PassRefPtr<IDBKey> prpKey, PassRefPtr<IDBCallbacks> prpCallbacks, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    if (transaction->mode() == IDBTransaction::READ_ONLY) {
        ec = IDBDatabaseException::READ_ONLY_ERR;
        return;
    }

    RefPtr<IDBObjectStoreBackendImpl> objectStore = this;
    RefPtr<IDBKey> key = prpKey;
    RefPtr<IDBCallbacks> callbacks = prpCallbacks;
    if (key->type() == IDBKey::NullType) {
        ec = IDBDatabaseException::DATA_ERR;
        return;
    }

    if (!transaction->scheduleTask(createCallbackTask(&IDBObjectStoreBackendImpl::deleteInternal, objectStore, key, callbacks)))
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
}

void IDBObjectStoreBackendImpl::deleteInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl> objectStore, PassRefPtr<IDBKey> key, PassRefPtr<IDBCallbacks> callbacks)
{
    RefPtr<IDBBackingStore::ObjectStoreRecordIdentifier> recordIdentifier = objectStore->m_backingStore->createInvalidRecordIdentifier();
    if (!objectStore->m_backingStore->keyExistsInObjectStore(objectStore->m_databaseId, objectStore->id(), *key, recordIdentifier.get())) {
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::NOT_FOUND_ERR, "Key does not exist in the object store."));
        return;
    }

    for (IndexMap::iterator it = objectStore->m_indexes.begin(); it != objectStore->m_indexes.end(); ++it) {
        if (!it->second->hasValidId())
            continue; // The index object has been created, but does not exist in the database yet.

        if (!objectStore->m_backingStore->deleteIndexDataForRecord(objectStore->m_databaseId, objectStore->id(), it->second->id(), recordIdentifier.get()))
            ASSERT_NOT_REACHED();
    }

    objectStore->m_backingStore->deleteObjectStoreRecord(objectStore->m_databaseId, objectStore->id(), recordIdentifier.get());
    callbacks->onSuccess(SerializedScriptValue::nullValue());
}

void IDBObjectStoreBackendImpl::clear(PassRefPtr<IDBCallbacks> prpCallbacks, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    if (transaction->mode() == IDBTransaction::READ_ONLY) {
        ec = IDBDatabaseException::READ_ONLY_ERR;
        return;
    }

    RefPtr<IDBObjectStoreBackendImpl> objectStore = this;
    RefPtr<IDBCallbacks> callbacks = prpCallbacks;

    if (!transaction->scheduleTask(createCallbackTask(&IDBObjectStoreBackendImpl::clearInternal, objectStore, callbacks)))
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
}

void IDBObjectStoreBackendImpl::clearInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl> objectStore, PassRefPtr<IDBCallbacks> callbacks)
{
    objectStore->m_backingStore->clearObjectStore(objectStore->m_databaseId, objectStore->id());
    callbacks->onSuccess(SerializedScriptValue::undefinedValue());
}

namespace {
class PopulateIndexCallback : public IDBBackingStore::ObjectStoreRecordCallback {
public:
    PopulateIndexCallback(IDBBackingStore& backingStore, const String& indexKeyPath, int64_t databaseId, int64_t objectStoreId, int64_t indexId)
        : m_backingStore(backingStore)
        , m_indexKeyPath(indexKeyPath)
        , m_databaseId(databaseId)
        , m_objectStoreId(objectStoreId)
        , m_indexId(indexId)
    {
    }

    virtual bool callback(const IDBBackingStore::ObjectStoreRecordIdentifier* recordIdentifier, const String& value)
    {
        RefPtr<SerializedScriptValue> objectValue = SerializedScriptValue::createFromWire(value);
        RefPtr<IDBKey> indexKey = fetchKeyFromKeyPath(objectValue.get(), m_indexKeyPath);

        if (!m_backingStore.putIndexDataForRecord(m_databaseId, m_objectStoreId, m_indexId, *indexKey, recordIdentifier))
            return false;

        return true;
    }

private:
    IDBBackingStore& m_backingStore;
    const String& m_indexKeyPath;
    int64_t m_databaseId;
    int64_t m_objectStoreId;
    int64_t m_indexId;
};
}

static bool populateIndex(IDBBackingStore& backingStore, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const String& indexKeyPath)
{
    PopulateIndexCallback callback(backingStore, indexKeyPath, databaseId, objectStoreId, indexId);
    if (!backingStore.forEachObjectStoreRecord(databaseId, objectStoreId, callback))
        return false;
    return true;
}

PassRefPtr<IDBIndexBackendInterface> IDBObjectStoreBackendImpl::createIndex(const String& name, const String& keyPath, bool unique, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    if (name.isNull()) {
        ec = IDBDatabaseException::NON_TRANSIENT_ERR;
        return 0;
    }
    if (m_indexes.contains(name)) {
        ec = IDBDatabaseException::CONSTRAINT_ERR;
        return 0;
    }
    if (transaction->mode() != IDBTransaction::VERSION_CHANGE) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
        return 0;
    }

    RefPtr<IDBIndexBackendImpl> index = IDBIndexBackendImpl::create(m_backingStore.get(), m_databaseId, this, name, m_name, keyPath, unique);
    ASSERT(index->name() == name);

    RefPtr<IDBObjectStoreBackendImpl> objectStore = this;
    RefPtr<IDBTransactionBackendInterface> transactionPtr = transaction;
    if (!transaction->scheduleTask(
              createCallbackTask(&IDBObjectStoreBackendImpl::createIndexInternal,
                                 objectStore, index, transactionPtr),
              createCallbackTask(&IDBObjectStoreBackendImpl::removeIndexFromMap,
                                 objectStore, index))) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
        return 0;
    }

    m_indexes.set(name, index);
    return index.release();
}

void IDBObjectStoreBackendImpl::createIndexInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl> objectStore, PassRefPtr<IDBIndexBackendImpl> index, PassRefPtr<IDBTransactionBackendInterface> transaction)
{
    int64_t id;
    if (!objectStore->m_backingStore->createIndex(objectStore->m_databaseId, objectStore->id(), index->name(), index->keyPath(), index->unique(), id)) {
        transaction->abort();
        return;
    }

    index->setId(id);

    if (!populateIndex(*objectStore->m_backingStore, objectStore->m_databaseId, objectStore->m_id, id, index->keyPath())) {
        transaction->abort();
        return;
    }

    transaction->didCompleteTaskEvents();
}

PassRefPtr<IDBIndexBackendInterface> IDBObjectStoreBackendImpl::index(const String& name, ExceptionCode& ec)
{
    RefPtr<IDBIndexBackendInterface> index = m_indexes.get(name);
    if (!index) {
        ec = IDBDatabaseException::NOT_FOUND_ERR;
        return 0;
    }
    return index.release();
}

void IDBObjectStoreBackendImpl::deleteIndex(const String& name, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    if (transaction->mode() != IDBTransaction::VERSION_CHANGE) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
        return;
    }

    RefPtr<IDBIndexBackendImpl> index = m_indexes.get(name);
    if (!index) {
        ec = IDBDatabaseException::NOT_FOUND_ERR;
        return;
    }

    RefPtr<IDBObjectStoreBackendImpl> objectStore = this;
    RefPtr<IDBTransactionBackendInterface> transactionPtr = transaction;
    if (!transaction->scheduleTask(
              createCallbackTask(&IDBObjectStoreBackendImpl::deleteIndexInternal,
                                 objectStore, index, transactionPtr),
              createCallbackTask(&IDBObjectStoreBackendImpl::addIndexToMap,
                                 objectStore, index))) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
        return;
    }
    m_indexes.remove(name);
}

void IDBObjectStoreBackendImpl::deleteIndexInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl> objectStore, PassRefPtr<IDBIndexBackendImpl> index, PassRefPtr<IDBTransactionBackendInterface> transaction)
{
    objectStore->m_backingStore->deleteIndex(objectStore->m_databaseId, objectStore->id(), index->id());
    transaction->didCompleteTaskEvents();
}

void IDBObjectStoreBackendImpl::openCursor(PassRefPtr<IDBKeyRange> prpRange, unsigned short direction, PassRefPtr<IDBCallbacks> prpCallbacks, IDBTransactionBackendInterface* transaction, ExceptionCode& ec)
{
    RefPtr<IDBObjectStoreBackendImpl> objectStore = this;
    RefPtr<IDBKeyRange> range = prpRange;
    RefPtr<IDBCallbacks> callbacks = prpCallbacks;
    RefPtr<IDBTransactionBackendInterface> transactionPtr = transaction;
    if (!transaction->scheduleTask(
            createCallbackTask(&IDBObjectStoreBackendImpl::openCursorInternal,
                               objectStore, range, direction, callbacks, transactionPtr))) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
    }
}

void IDBObjectStoreBackendImpl::openCursorInternal(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl> objectStore, PassRefPtr<IDBKeyRange> range, unsigned short tmpDirection, PassRefPtr<IDBCallbacks> callbacks, PassRefPtr<IDBTransactionBackendInterface> transaction)
{
    IDBCursor::Direction direction = static_cast<IDBCursor::Direction>(tmpDirection);

    RefPtr<IDBBackingStore::Cursor> backingStoreCursor = objectStore->m_backingStore->openObjectStoreCursor(objectStore->m_databaseId, objectStore->id(), range.get(), direction);
    if (!backingStoreCursor) {
        callbacks->onSuccess(SerializedScriptValue::nullValue());
        return;
    }

    RefPtr<IDBCursorBackendInterface> cursor = IDBCursorBackendImpl::create(backingStoreCursor.release(), direction, IDBCursorBackendInterface::ObjectStoreCursor, transaction.get(), objectStore.get());
    callbacks->onSuccess(cursor.release());
}

void IDBObjectStoreBackendImpl::loadIndexes()
{
    Vector<int64_t> ids;
    Vector<String> names;
    Vector<String> keyPaths;
    Vector<bool> uniqueFlags;
    m_backingStore->getIndexes(m_databaseId, m_id, ids, names, keyPaths, uniqueFlags);

    ASSERT(names.size() == ids.size());
    ASSERT(keyPaths.size() == ids.size());
    ASSERT(uniqueFlags.size() == ids.size());

    for (size_t i = 0; i < ids.size(); i++)
        m_indexes.set(names[i], IDBIndexBackendImpl::create(m_backingStore.get(), m_databaseId, this, ids[i], names[i], m_name, keyPaths[i], uniqueFlags[i]));
}

void IDBObjectStoreBackendImpl::removeIndexFromMap(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl> objectStore, PassRefPtr<IDBIndexBackendImpl> index)
{
    ASSERT(objectStore->m_indexes.contains(index->name()));
    objectStore->m_indexes.remove(index->name());
}

void IDBObjectStoreBackendImpl::addIndexToMap(ScriptExecutionContext*, PassRefPtr<IDBObjectStoreBackendImpl> objectStore, PassRefPtr<IDBIndexBackendImpl> index)
{
    RefPtr<IDBIndexBackendImpl> indexPtr = index;
    ASSERT(!objectStore->m_indexes.contains(indexPtr->name()));
    objectStore->m_indexes.set(indexPtr->name(), indexPtr);
}

PassRefPtr<IDBKey> IDBObjectStoreBackendImpl::genAutoIncrementKey()
{
    if (m_autoIncrementNumber > 0)
        return IDBKey::createNumber(m_autoIncrementNumber++);

    m_autoIncrementNumber = static_cast<int>(m_backingStore->nextAutoIncrementNumber(m_databaseId, id()));
    return IDBKey::createNumber(m_autoIncrementNumber++);
}


} // namespace WebCore

#endif
