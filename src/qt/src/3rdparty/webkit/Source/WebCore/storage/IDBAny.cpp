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
#include "IDBAny.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBCursorWithValue.h"
#include "IDBDatabase.h"
#include "IDBFactory.h"
#include "IDBIndex.h"
#include "IDBObjectStore.h"
#include "SerializedScriptValue.h"

namespace WebCore {

PassRefPtr<IDBAny> IDBAny::createInvalid()
{
    return adoptRef(new IDBAny());
}

PassRefPtr<IDBAny> IDBAny::createNull()
{
    RefPtr<IDBAny> idbAny = adoptRef(new IDBAny());
    idbAny->setNull();
    return idbAny.release();
}

IDBAny::IDBAny()
    : m_type(UndefinedType)
{
}

IDBAny::~IDBAny()
{
}

PassRefPtr<IDBCursor> IDBAny::idbCursor()
{
    ASSERT(m_type == IDBCursorType);
    return m_idbCursor;
}


PassRefPtr<IDBCursorWithValue> IDBAny::idbCursorWithValue()
{
    ASSERT(m_type == IDBCursorWithValueType);
    return m_idbCursorWithValue;
}

PassRefPtr<IDBDatabase> IDBAny::idbDatabase()
{
    ASSERT(m_type == IDBDatabaseType);
    return m_idbDatabase;
}

PassRefPtr<IDBFactory> IDBAny::idbFactory()
{
    ASSERT(m_type == IDBFactoryType);
    return m_idbFactory;
}

PassRefPtr<IDBIndex> IDBAny::idbIndex()
{
    ASSERT(m_type == IDBIndexType);
    return m_idbIndex;
}

PassRefPtr<IDBKey> IDBAny::idbKey()
{
    ASSERT(m_type == IDBKeyType);
    return m_idbKey;
}

PassRefPtr<IDBObjectStore> IDBAny::idbObjectStore()
{
    ASSERT(m_type == IDBObjectStoreType);
    return m_idbObjectStore;
}

PassRefPtr<IDBTransaction> IDBAny::idbTransaction()
{
    ASSERT(m_type == IDBTransactionType);
    return m_idbTransaction;
}

PassRefPtr<SerializedScriptValue> IDBAny::serializedScriptValue()
{
    ASSERT(m_type == SerializedScriptValueType);
    return m_serializedScriptValue;
}

void IDBAny::setNull()
{
    ASSERT(m_type == UndefinedType);
    m_type = NullType;
}

void IDBAny::set(PassRefPtr<IDBCursorWithValue> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBCursorWithValueType;
    m_idbCursorWithValue = value;
}

void IDBAny::set(PassRefPtr<IDBCursor> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBCursorType;
    m_idbCursor = value;
}

void IDBAny::set(PassRefPtr<IDBDatabase> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBDatabaseType;
    m_idbDatabase = value;
}

void IDBAny::set(PassRefPtr<IDBFactory> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBFactoryType;
    m_idbFactory = value;
}

void IDBAny::set(PassRefPtr<IDBIndex> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBIndexType;
    m_idbIndex = value;
}

void IDBAny::set(PassRefPtr<IDBKey> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBKeyType;
    m_idbKey = value;
}

void IDBAny::set(PassRefPtr<IDBTransaction> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBTransactionType;
    m_idbTransaction = value;
}

void IDBAny::set(PassRefPtr<IDBObjectStore> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = IDBObjectStoreType;
    m_idbObjectStore = value;
}

void IDBAny::set(PassRefPtr<SerializedScriptValue> value)
{
    ASSERT(m_type == UndefinedType);
    m_type = SerializedScriptValueType;
    m_serializedScriptValue = value;
}

} // namespace WebCore

#endif
