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


#ifndef IDBCursorBackendImpl_h
#define IDBCursorBackendImpl_h

#if ENABLE(INDEXED_DATABASE)

#include "IDBBackingStore.h"
#include "IDBCursor.h"
#include "IDBCursorBackendInterface.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class IDBDatabaseBackendImpl;
class IDBIndexBackendImpl;
class IDBKeyRange;
class IDBObjectStoreBackendInterface;
class IDBBackingStore;
class IDBTransactionBackendInterface;
class SerializedScriptValue;

class IDBCursorBackendImpl : public IDBCursorBackendInterface {
public:
    static PassRefPtr<IDBCursorBackendImpl> create(PassRefPtr<IDBBackingStore::Cursor> cursor, IDBCursor::Direction direction, CursorType cursorType, IDBTransactionBackendInterface* transaction, IDBObjectStoreBackendInterface* objectStore)
    {
        return adoptRef(new IDBCursorBackendImpl(cursor, direction, cursorType, transaction, objectStore));
    }
    virtual ~IDBCursorBackendImpl();

    virtual unsigned short direction() const;
    virtual PassRefPtr<IDBKey> key() const;
    virtual PassRefPtr<IDBKey> primaryKey() const;
    virtual PassRefPtr<SerializedScriptValue> value() const;
    virtual void update(PassRefPtr<SerializedScriptValue>, PassRefPtr<IDBCallbacks>, ExceptionCode&);
    virtual void continueFunction(PassRefPtr<IDBKey>, PassRefPtr<IDBCallbacks>, ExceptionCode&);
    virtual void deleteFunction(PassRefPtr<IDBCallbacks>, ExceptionCode&);

private:
    IDBCursorBackendImpl(PassRefPtr<IDBBackingStore::Cursor>, IDBCursor::Direction, CursorType, IDBTransactionBackendInterface*, IDBObjectStoreBackendInterface*);

    static void continueFunctionInternal(ScriptExecutionContext*, PassRefPtr<IDBCursorBackendImpl>, PassRefPtr<IDBKey>, PassRefPtr<IDBCallbacks>);

    RefPtr<IDBBackingStore::Cursor> m_cursor;
    IDBCursor::Direction m_direction;
    CursorType m_cursorType;
    RefPtr<IDBTransactionBackendInterface> m_transaction;
    RefPtr<IDBObjectStoreBackendInterface> m_objectStore;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBCursorBackendImpl_h
