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

#include "IDBBackingStore.h"
#include "IDBCallbacks.h"
#include "IDBDatabaseBackendImpl.h"
#include "IDBDatabaseError.h"
#include "IDBDatabaseException.h"
#include "IDBKeyRange.h"
#include "IDBTracing.h"
#include "IDBTransactionBackendImpl.h"
#include "SharedBuffer.h"

namespace WebCore {

class IDBCursorBackendImpl::CursorIterationOperation : public IDBTransactionBackendImpl::Operation {
public:
    static PassOwnPtr<IDBTransactionBackendImpl::Operation> create(PassRefPtr<IDBCursorBackendImpl> cursor, PassRefPtr<IDBKey> key, PassRefPtr<IDBCallbacks> callbacks)
    {
        return adoptPtr(new CursorIterationOperation(cursor, key, callbacks));
    }
    virtual void perform(IDBTransactionBackendImpl*);
private:
    CursorIterationOperation(PassRefPtr<IDBCursorBackendImpl> cursor, PassRefPtr<IDBKey> key, PassRefPtr<IDBCallbacks> callbacks)
        : m_cursor(cursor)
        , m_key(key)
        , m_callbacks(callbacks)
    {
    }

    RefPtr<IDBCursorBackendImpl> m_cursor;
    RefPtr<IDBKey> m_key;
    RefPtr<IDBCallbacks> m_callbacks;
};

class IDBCursorBackendImpl::CursorAdvanceOperation : public IDBTransactionBackendImpl::Operation {
public:
    static PassOwnPtr<IDBTransactionBackendImpl::Operation> create(PassRefPtr<IDBCursorBackendImpl> cursor, unsigned long count, PassRefPtr<IDBCallbacks> callbacks)
    {
        return adoptPtr(new CursorAdvanceOperation(cursor, count, callbacks));
    }
    virtual void perform(IDBTransactionBackendImpl*);
private:
    CursorAdvanceOperation(PassRefPtr<IDBCursorBackendImpl> cursor, unsigned long count, PassRefPtr<IDBCallbacks> callbacks)
        : m_cursor(cursor)
        , m_count(count)
        , m_callbacks(callbacks)
    {
    }

    RefPtr<IDBCursorBackendImpl> m_cursor;
    unsigned long m_count;
    RefPtr<IDBCallbacks> m_callbacks;
};

class IDBCursorBackendImpl::CursorPrefetchIterationOperation : public IDBTransactionBackendImpl::Operation {
public:
    static PassOwnPtr<IDBTransactionBackendImpl::Operation> create(PassRefPtr<IDBCursorBackendImpl> cursor, int numberToFetch, PassRefPtr<IDBCallbacks> callbacks)
    {
        return adoptPtr(new CursorPrefetchIterationOperation(cursor, numberToFetch, callbacks));
    }
    virtual void perform(IDBTransactionBackendImpl*);
private:
    CursorPrefetchIterationOperation(PassRefPtr<IDBCursorBackendImpl> cursor, int numberToFetch, PassRefPtr<IDBCallbacks> callbacks)
        : m_cursor(cursor)
        , m_numberToFetch(numberToFetch)
        , m_callbacks(callbacks)
    {
    }

    RefPtr<IDBCursorBackendImpl> m_cursor;
    int m_numberToFetch;
    RefPtr<IDBCallbacks> m_callbacks;
};

IDBCursorBackendImpl::IDBCursorBackendImpl(PassRefPtr<IDBBackingStore::Cursor> cursor, IndexedDB::CursorType cursorType, IDBDatabaseBackendInterface::TaskType taskType, IDBTransactionBackendImpl* transaction, int64_t objectStoreId)
    : m_taskType(taskType)
    , m_cursorType(cursorType)
    , m_database(transaction->database())
    , m_transaction(transaction)
    , m_objectStoreId(objectStoreId)
    , m_cursor(cursor)
    , m_closed(false)
{
    m_transaction->registerOpenCursor(this);
}

IDBCursorBackendImpl::~IDBCursorBackendImpl()
{
    m_transaction->unregisterOpenCursor(this);
}


void IDBCursorBackendImpl::continueFunction(PassRefPtr<IDBKey> key, PassRefPtr<IDBCallbacks> prpCallbacks, ExceptionCode&)
{
    IDB_TRACE("IDBCursorBackendImpl::continue");
    RefPtr<IDBCallbacks> callbacks = prpCallbacks;
    m_transaction->scheduleTask(m_taskType, CursorIterationOperation::create(this, key, callbacks));
}

void IDBCursorBackendImpl::advance(unsigned long count, PassRefPtr<IDBCallbacks> prpCallbacks, ExceptionCode&)
{
    IDB_TRACE("IDBCursorBackendImpl::advance");
    RefPtr<IDBCallbacks> callbacks = prpCallbacks;
    m_transaction->scheduleTask(CursorAdvanceOperation::create(this, count, callbacks));
}

void IDBCursorBackendImpl::CursorAdvanceOperation::perform(IDBTransactionBackendImpl*)
{
    IDB_TRACE("CursorAdvanceOperation");
    if (!m_cursor->m_cursor || !m_cursor->m_cursor->advance(m_count)) {
        m_cursor->m_cursor = 0;
        m_callbacks->onSuccess(static_cast<SharedBuffer*>(0));
        return;
    }

    m_callbacks->onSuccess(m_cursor->key(), m_cursor->primaryKey(), m_cursor->value());
}

void IDBCursorBackendImpl::CursorIterationOperation::perform(IDBTransactionBackendImpl*)
{
    IDB_TRACE("CursorIterationOperation");
    if (!m_cursor->m_cursor || !m_cursor->m_cursor->continueFunction(m_key.get())) {
        m_cursor->m_cursor = 0;
        m_callbacks->onSuccess(static_cast<SharedBuffer*>(0));
        return;
    }

    m_callbacks->onSuccess(m_cursor->key(), m_cursor->primaryKey(), m_cursor->value());
}

void IDBCursorBackendImpl::deleteFunction(PassRefPtr<IDBCallbacks> prpCallbacks, ExceptionCode&)
{
    IDB_TRACE("IDBCursorBackendImpl::delete");
    ASSERT(m_transaction->mode() != IndexedDB::TransactionReadOnly);
    RefPtr<IDBKeyRange> keyRange = IDBKeyRange::create(m_cursor->primaryKey());
    m_database->deleteRange(m_transaction->id(), m_objectStoreId, keyRange.release(), prpCallbacks);
}

void IDBCursorBackendImpl::prefetchContinue(int numberToFetch, PassRefPtr<IDBCallbacks> prpCallbacks, ExceptionCode&)
{
    IDB_TRACE("IDBCursorBackendImpl::prefetchContinue");
    RefPtr<IDBCallbacks> callbacks = prpCallbacks;
    m_transaction->scheduleTask(m_taskType, CursorPrefetchIterationOperation::create(this, numberToFetch, callbacks));
}

void IDBCursorBackendImpl::CursorPrefetchIterationOperation::perform(IDBTransactionBackendImpl*)
{
    IDB_TRACE("CursorPrefetchIterationOperation");

    Vector<RefPtr<IDBKey> > foundKeys;
    Vector<RefPtr<IDBKey> > foundPrimaryKeys;
    Vector<RefPtr<SharedBuffer> > foundValues;

    if (m_cursor->m_cursor)
        m_cursor->m_savedCursor = m_cursor->m_cursor->clone();

    const size_t maxSizeEstimate = 10 * 1024 * 1024;
    size_t sizeEstimate = 0;

    for (int i = 0; i < m_numberToFetch; ++i) {
        if (!m_cursor->m_cursor || !m_cursor->m_cursor->continueFunction(0)) {
            m_cursor->m_cursor = 0;
            break;
        }

        foundKeys.append(m_cursor->m_cursor->key());
        foundPrimaryKeys.append(m_cursor->m_cursor->primaryKey());

        switch (m_cursor->m_cursorType) {
        case IndexedDB::CursorKeyOnly:
            foundValues.append(SharedBuffer::create());
            break;
        case IndexedDB::CursorKeyAndValue:
            sizeEstimate += m_cursor->m_cursor->value()->size();
            foundValues.append(m_cursor->m_cursor->value());
            break;
        default:
            ASSERT_NOT_REACHED();
        }
        sizeEstimate += m_cursor->m_cursor->key()->sizeEstimate();
        sizeEstimate += m_cursor->m_cursor->primaryKey()->sizeEstimate();

        if (sizeEstimate > maxSizeEstimate)
            break;
    }

    if (!foundKeys.size()) {
        m_callbacks->onSuccess(static_cast<SharedBuffer*>(0));
        return;
    }

    m_callbacks->onSuccessWithPrefetch(foundKeys, foundPrimaryKeys, foundValues);
}

void IDBCursorBackendImpl::prefetchReset(int usedPrefetches, int)
{
    IDB_TRACE("IDBCursorBackendImpl::prefetchReset");
    m_cursor = m_savedCursor;
    m_savedCursor = 0;

    if (m_closed)
        return;
    if (m_cursor) {
        for (int i = 0; i < usedPrefetches; ++i) {
            bool ok = m_cursor->continueFunction();
            ASSERT_UNUSED(ok, ok);
        }
    }
}

void IDBCursorBackendImpl::close()
{
    IDB_TRACE("IDBCursorBackendImpl::close");
    m_closed = true;
    m_cursor.clear();
    m_savedCursor.clear();
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
