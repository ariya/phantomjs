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
#include "IDBIndex.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBCursorBackendInterface.h"
#include "IDBDatabaseException.h"
#include "IDBIndexBackendInterface.h"
#include "IDBKey.h"
#include "IDBKeyRange.h"
#include "IDBObjectStore.h"
#include "IDBRequest.h"
#include "IDBTransaction.h"

namespace WebCore {

static const unsigned short defaultDirection = IDBCursor::NEXT;

IDBIndex::IDBIndex(PassRefPtr<IDBIndexBackendInterface> backend, IDBObjectStore* objectStore, IDBTransaction* transaction)
    : m_backend(backend)
    , m_objectStore(objectStore)
    , m_transaction(transaction)
{
    ASSERT(m_backend);
    ASSERT(m_objectStore);
    ASSERT(m_transaction);
}

IDBIndex::~IDBIndex()
{
}

PassRefPtr<IDBRequest> IDBIndex::openCursor(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> keyRange, unsigned short direction, ExceptionCode& ec)
{
    if (direction != IDBCursor::NEXT && direction != IDBCursor::NEXT_NO_DUPLICATE && direction != IDBCursor::PREV && direction != IDBCursor::PREV_NO_DUPLICATE) {
        // FIXME: May need to change when specced: http://www.w3.org/Bugs/Public/show_bug.cgi?id=11406
        ec = IDBDatabaseException::CONSTRAINT_ERR;
        return 0;
    }

    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    request->setCursorType(IDBCursorBackendInterface::IndexCursor);
    m_backend->openCursor(keyRange, direction, request, m_transaction->backend(), ec);
    if (ec) {
        request->markEarlyDeath();
        return 0;
    }
    return request;
}

PassRefPtr<IDBRequest> IDBIndex::openKeyCursor(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> keyRange, unsigned short direction, ExceptionCode& ec)
{
    if (direction != IDBCursor::NEXT && direction != IDBCursor::NEXT_NO_DUPLICATE && direction != IDBCursor::PREV && direction != IDBCursor::PREV_NO_DUPLICATE) {
        // FIXME: May need to change when specced: http://www.w3.org/Bugs/Public/show_bug.cgi?id=11406
        ec = IDBDatabaseException::CONSTRAINT_ERR;
        return 0;
    }

    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    request->setCursorType(IDBCursorBackendInterface::IndexKeyCursor);
    m_backend->openKeyCursor(keyRange, direction, request, m_transaction->backend(), ec);
    if (ec) {
        request->markEarlyDeath();
        return 0;
    }
    return request;
}

PassRefPtr<IDBRequest> IDBIndex::get(ScriptExecutionContext* context, PassRefPtr<IDBKey> key, ExceptionCode& ec)
{
    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    m_backend->get(key, request, m_transaction->backend(), ec);
    if (ec) {
        request->markEarlyDeath();
        return 0;
    }
    return request;
}

PassRefPtr<IDBRequest> IDBIndex::getKey(ScriptExecutionContext* context, PassRefPtr<IDBKey> key, ExceptionCode& ec)
{
    RefPtr<IDBRequest> request = IDBRequest::create(context, IDBAny::create(this), m_transaction.get());
    m_backend->getKey(key, request, m_transaction->backend(), ec);
    if (ec) {
        request->markEarlyDeath();
        return 0;
    }
    return request;
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
