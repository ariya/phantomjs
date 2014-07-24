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
#include "IDBCursor.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBAny.h"
#include "IDBBindingUtilities.h"
#include "IDBCallbacks.h"
#include "IDBCursorBackendInterface.h"
#include "IDBKey.h"
#include "IDBObjectStore.h"
#include "IDBRequest.h"
#include "IDBTracing.h"
#include "IDBTransaction.h"
#include "ScriptCallStack.h"
#include "ScriptExecutionContext.h"
#include <limits>

namespace WebCore {

PassRefPtr<IDBCursor> IDBCursor::create(PassRefPtr<IDBCursorBackendInterface> backend, IndexedDB::CursorDirection direction, IDBRequest* request, IDBAny* source, IDBTransaction* transaction)
{
    return adoptRef(new IDBCursor(backend, direction, request, source, transaction));
}

const AtomicString& IDBCursor::directionNext()
{
    DEFINE_STATIC_LOCAL(AtomicString, next, ("next", AtomicString::ConstructFromLiteral));
    return next;
}

const AtomicString& IDBCursor::directionNextUnique()
{
    DEFINE_STATIC_LOCAL(AtomicString, nextunique, ("nextunique", AtomicString::ConstructFromLiteral));
    return nextunique;
}

const AtomicString& IDBCursor::directionPrev()
{
    DEFINE_STATIC_LOCAL(AtomicString, prev, ("prev", AtomicString::ConstructFromLiteral));
    return prev;
}

const AtomicString& IDBCursor::directionPrevUnique()
{
    DEFINE_STATIC_LOCAL(AtomicString, prevunique, ("prevunique", AtomicString::ConstructFromLiteral));
    return prevunique;
}


IDBCursor::IDBCursor(PassRefPtr<IDBCursorBackendInterface> backend, IndexedDB::CursorDirection direction, IDBRequest* request, IDBAny* source, IDBTransaction* transaction)
    : m_backend(backend)
    , m_request(request)
    , m_direction(direction)
    , m_source(source)
    , m_transaction(transaction)
    , m_transactionNotifier(transaction, this)
    , m_gotValue(false)
{
    ASSERT(m_backend);
    ASSERT(m_request);
    ASSERT(m_source->type() == IDBAny::IDBObjectStoreType || m_source->type() == IDBAny::IDBIndexType);
    ASSERT(m_transaction);
}

IDBCursor::~IDBCursor()
{
}

const String& IDBCursor::direction() const
{
    IDB_TRACE("IDBCursor::direction");
    return directionToString(m_direction);
}

const ScriptValue& IDBCursor::key() const
{
    IDB_TRACE("IDBCursor::key");
    return m_currentKeyValue;
}

const ScriptValue& IDBCursor::primaryKey() const
{
    IDB_TRACE("IDBCursor::primaryKey");
    return m_currentPrimaryKeyValue;
}

const ScriptValue& IDBCursor::value() const
{
    IDB_TRACE("IDBCursor::value");
    return m_currentValue;
}

IDBAny* IDBCursor::source() const
{
    return m_source.get();
}

PassRefPtr<IDBRequest> IDBCursor::update(ScriptState* state, ScriptValue& value, ExceptionCode& ec)
{
    IDB_TRACE("IDBCursor::update");

    if (!m_gotValue || isKeyCursor()) {
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

    RefPtr<IDBObjectStore> objectStore = effectiveObjectStore();
    const IDBKeyPath& keyPath = objectStore->metadata().keyPath;
    const bool usesInLineKeys = !keyPath.isNull();
    if (usesInLineKeys) {
        RefPtr<IDBKey> keyPathKey = createIDBKeyFromScriptValueAndKeyPath(m_request->requestState(), value, keyPath);
        if (!keyPathKey || !keyPathKey->isEqual(m_currentPrimaryKey.get())) {
            ec = IDBDatabaseException::DataError;
            return 0;
        }
    }

    return objectStore->put(IDBDatabaseBackendInterface::CursorUpdate, IDBAny::create(this), state, value, m_currentPrimaryKey, ec);
}

void IDBCursor::advance(unsigned long count, ExceptionCode& ec)
{
    ec = 0;
    IDB_TRACE("IDBCursor::advance");
    if (!m_gotValue) {
        ec = IDBDatabaseException::InvalidStateError;
        return;
    }

    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TransactionInactiveError;
        return;
    }

    if (!count) {
        ec = TypeError;
        return;
    }

    m_request->setPendingCursor(this);
    m_gotValue = false;
    m_backend->advance(count, m_request, ec);
    ASSERT(!ec);
}

void IDBCursor::continueFunction(ScriptExecutionContext* context, const ScriptValue& keyValue, ExceptionCode& ec)
{
    DOMRequestState requestState(context);
    RefPtr<IDBKey> key = scriptValueToIDBKey(&requestState, keyValue);
    continueFunction(key.release(), ec);
}

void IDBCursor::continueFunction(PassRefPtr<IDBKey> key, ExceptionCode& ec)
{
    ec = 0;
    IDB_TRACE("IDBCursor::continue");
    if (key && !key->isValid()) {
        ec = IDBDatabaseException::DataError;
        return;
    }

    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TransactionInactiveError;
        return;
    }

    if (!m_gotValue) {
        ec = IDBDatabaseException::InvalidStateError;
        return;
    }

    if (key) {
        ASSERT(m_currentKey);
        if (m_direction == IndexedDB::CursorNext || m_direction == IndexedDB::CursorNextNoDuplicate) {
            if (!m_currentKey->isLessThan(key.get())) {
                ec = IDBDatabaseException::DataError;
                return;
            }
        } else {
            if (!key->isLessThan(m_currentKey.get())) {
                ec = IDBDatabaseException::DataError;
                return;
            }
        }
    }

    // FIXME: We're not using the context from when continue was called, which means the callback
    //        will be on the original context openCursor was called on. Is this right?
    m_request->setPendingCursor(this);
    m_gotValue = false;
    m_backend->continueFunction(key, m_request, ec);
    ASSERT(!ec);
}

PassRefPtr<IDBRequest> IDBCursor::deleteFunction(ScriptExecutionContext* context, ExceptionCode& ec)
{
    ec = 0;
    IDB_TRACE("IDBCursor::delete");
    if (!m_transaction->isActive()) {
        ec = IDBDatabaseException::TransactionInactiveError;
        return 0;
    }
    if (m_transaction->isReadOnly()) {
        ec = IDBDatabaseException::ReadOnlyError;
        return 0;
    }

    if (!m_gotValue || isKeyCursor()) {
        ec = IDBDatabaseException::InvalidStateError;
        return 0;
    }
    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    m_backend->deleteFunction(request, ec);
    ASSERT(!ec);
    return request.release();
}

void IDBCursor::postSuccessHandlerCallback()
{
    m_backend->postSuccessHandlerCallback();
}

void IDBCursor::close()
{
    m_transactionNotifier.cursorFinished();
    if (m_request) {
        m_request->finishCursor();
        m_request.clear();
    }
}

void IDBCursor::setValueReady(DOMRequestState* state, PassRefPtr<IDBKey> key, PassRefPtr<IDBKey> primaryKey, ScriptValue& value)
{
    m_currentKey = key;
    m_currentKeyValue = idbKeyToScriptValue(state, m_currentKey);

    m_currentPrimaryKey = primaryKey;
    m_currentPrimaryKeyValue = idbKeyToScriptValue(state, m_currentPrimaryKey);

    if (!isKeyCursor()) {
        RefPtr<IDBObjectStore> objectStore = effectiveObjectStore();
        const IDBObjectStoreMetadata metadata = objectStore->metadata();
        if (metadata.autoIncrement && !metadata.keyPath.isNull()) {
#ifndef NDEBUG
            RefPtr<IDBKey> expectedKey = createIDBKeyFromScriptValueAndKeyPath(m_request->requestState(), value, metadata.keyPath);
            ASSERT(!expectedKey || expectedKey->isEqual(m_currentPrimaryKey.get()));
#endif
            bool injected = injectIDBKeyIntoScriptValue(m_request->requestState(), m_currentPrimaryKey, value, metadata.keyPath);
            // FIXME: There is no way to report errors here. Move this into onSuccessWithContinuation so that we can abort the transaction there. See: https://bugs.webkit.org/show_bug.cgi?id=92278
            ASSERT_UNUSED(injected, injected);
        }
    }
    m_currentValue = value;

    m_gotValue = true;
}

PassRefPtr<IDBObjectStore> IDBCursor::effectiveObjectStore()
{
    if (m_source->type() == IDBAny::IDBObjectStoreType)
        return m_source->idbObjectStore();
    RefPtr<IDBIndex> index = m_source->idbIndex();
    return index->objectStore();
}

IndexedDB::CursorDirection IDBCursor::stringToDirection(const String& directionString, ExceptionCode& ec)
{
    if (directionString == IDBCursor::directionNext())
        return IndexedDB::CursorNext;
    if (directionString == IDBCursor::directionNextUnique())
        return IndexedDB::CursorNextNoDuplicate;
    if (directionString == IDBCursor::directionPrev())
        return IndexedDB::CursorPrev;
    if (directionString == IDBCursor::directionPrevUnique())
        return IndexedDB::CursorPrevNoDuplicate;

    ec = TypeError;
    return IndexedDB::CursorNext;
}

const AtomicString& IDBCursor::directionToString(unsigned short direction)
{
    switch (direction) {
    case IndexedDB::CursorNext:
        return IDBCursor::directionNext();

    case IndexedDB::CursorNextNoDuplicate:
        return IDBCursor::directionNextUnique();

    case IndexedDB::CursorPrev:
        return IDBCursor::directionPrev();

    case IndexedDB::CursorPrevNoDuplicate:
        return IDBCursor::directionPrevUnique();

    default:
        ASSERT_NOT_REACHED();
        return IDBCursor::directionNext();
    }
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
