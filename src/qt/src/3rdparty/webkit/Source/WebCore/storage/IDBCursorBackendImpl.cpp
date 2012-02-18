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
#include "IDBCursorBackendImpl.h"

#if ENABLE(INDEXED_DATABASE)

#include "CrossThreadTask.h"
#include "IDBBackingStore.h"
#include "IDBCallbacks.h"
#include "IDBDatabaseError.h"
#include "IDBDatabaseException.h"
#include "IDBKeyRange.h"
#include "IDBObjectStoreBackendImpl.h"
#include "IDBRequest.h"
#include "IDBTransactionBackendInterface.h"
#include "SerializedScriptValue.h"

namespace WebCore {

IDBCursorBackendImpl::IDBCursorBackendImpl(PassRefPtr<IDBBackingStore::Cursor> cursor, IDBCursor::Direction direction, CursorType cursorType, IDBTransactionBackendInterface* transaction, IDBObjectStoreBackendInterface* objectStore)
    : m_cursor(cursor)
    , m_direction(direction)
    , m_cursorType(cursorType)
    , m_transaction(transaction)
    , m_objectStore(objectStore)
{
}

IDBCursorBackendImpl::~IDBCursorBackendImpl()
{
}

unsigned short IDBCursorBackendImpl::direction() const
{
    return m_direction;
}

PassRefPtr<IDBKey> IDBCursorBackendImpl::key() const
{
    return m_cursor->key();
}

PassRefPtr<IDBKey> IDBCursorBackendImpl::primaryKey() const
{
    return m_cursor->primaryKey();
}

PassRefPtr<SerializedScriptValue> IDBCursorBackendImpl::value() const
{
    ASSERT(m_cursorType != IndexKeyCursor);
    return SerializedScriptValue::createFromWire(m_cursor->value());
}

void IDBCursorBackendImpl::update(PassRefPtr<SerializedScriptValue> value, PassRefPtr<IDBCallbacks> callbacks, ExceptionCode& ec)
{
    if (!m_cursor || m_cursorType == IndexKeyCursor) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
        return;
    }

    m_objectStore->put(value, m_cursor->primaryKey(), IDBObjectStoreBackendInterface::CursorUpdate, callbacks, m_transaction.get(), ec);
}

void IDBCursorBackendImpl::continueFunction(PassRefPtr<IDBKey> prpKey, PassRefPtr<IDBCallbacks> prpCallbacks, ExceptionCode& ec)
{
    RefPtr<IDBCursorBackendImpl> cursor = this;
    RefPtr<IDBKey> key = prpKey;
    RefPtr<IDBCallbacks> callbacks = prpCallbacks;
    if (!m_transaction->scheduleTask(createCallbackTask(&IDBCursorBackendImpl::continueFunctionInternal, cursor, key, callbacks)))
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
}

// IMPORTANT: If this ever 1) fires an 'error' event and 2) it's possible to fire another event afterwards,
//            IDBRequest::hasPendingActivity() will need to be modified to handle this!!!
void IDBCursorBackendImpl::continueFunctionInternal(ScriptExecutionContext*, PassRefPtr<IDBCursorBackendImpl> prpCursor, PassRefPtr<IDBKey> prpKey, PassRefPtr<IDBCallbacks> callbacks)
{
    RefPtr<IDBCursorBackendImpl> cursor = prpCursor;
    RefPtr<IDBKey> key = prpKey;

    if (!cursor->m_cursor || !cursor->m_cursor->continueFunction(key.get())) {
        cursor->m_cursor = 0;
        callbacks->onSuccess(SerializedScriptValue::nullValue());
        return;
    }

    callbacks->onSuccess(cursor.get());
}

void IDBCursorBackendImpl::deleteFunction(PassRefPtr<IDBCallbacks> prpCallbacks, ExceptionCode& ec)
{
    if (!m_cursor || m_cursorType == IndexKeyCursor) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
        return;
    }

    m_objectStore->deleteFunction(m_cursor->primaryKey(), prpCallbacks, m_transaction.get(), ec);
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
