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
#include "IDBKeyPath.h"
#include "IDBObjectStore.h"
#include "DOMStringList.h"

namespace WebCore {

PassRefPtr<IDBAny> IDBAny::createInvalid()
{
    return adoptRef(new IDBAny(UndefinedType));
}

PassRefPtr<IDBAny> IDBAny::createNull()
{
    return adoptRef(new IDBAny(NullType));
}

PassRefPtr<IDBAny> IDBAny::createString(const String& value)
{
    return adoptRef(new IDBAny(value));
}

IDBAny::IDBAny(Type type)
    : m_type(type)
    , m_integer(0)
{
    ASSERT(type == UndefinedType || type == NullType);
}

IDBAny::~IDBAny()
{
}

PassRefPtr<DOMStringList> IDBAny::domStringList()
{
    ASSERT(m_type == DOMStringListType);
    return m_domStringList;
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

const ScriptValue& IDBAny::scriptValue()
{
    ASSERT(m_type == ScriptValueType);
    return m_scriptValue;
}

const String& IDBAny::string()
{
    ASSERT(m_type == StringType);
    return m_string;
}

int64_t IDBAny::integer()
{
    ASSERT(m_type == IntegerType);
    return m_integer;
}

IDBAny::IDBAny(PassRefPtr<DOMStringList> value)
    : m_type(DOMStringListType)
    , m_domStringList(value)
    , m_integer(0)
{
}

IDBAny::IDBAny(PassRefPtr<IDBCursorWithValue> value)
    : m_type(IDBCursorWithValueType)
    , m_idbCursorWithValue(value)
    , m_integer(0)
{
}

IDBAny::IDBAny(PassRefPtr<IDBCursor> value)
    : m_type(IDBCursorType)
    , m_idbCursor(value)
    , m_integer(0)
{
}

IDBAny::IDBAny(PassRefPtr<IDBDatabase> value)
    : m_type(IDBDatabaseType)
    , m_idbDatabase(value)
    , m_integer(0)
{
}

IDBAny::IDBAny(PassRefPtr<IDBFactory> value)
    : m_type(IDBFactoryType)
    , m_idbFactory(value)
    , m_integer(0)
{
}

IDBAny::IDBAny(PassRefPtr<IDBIndex> value)
    : m_type(IDBIndexType)
    , m_idbIndex(value)
    , m_integer(0)
{
}

IDBAny::IDBAny(PassRefPtr<IDBTransaction> value)
    : m_type(IDBTransactionType)
    , m_idbTransaction(value)
    , m_integer(0)
{
}

IDBAny::IDBAny(PassRefPtr<IDBObjectStore> value)
    : m_type(IDBObjectStoreType)
    , m_idbObjectStore(value)
    , m_integer(0)
{
}

IDBAny::IDBAny(const ScriptValue& value)
    : m_type(ScriptValueType)
    , m_scriptValue(value)
    , m_integer(0)
{
}

IDBAny::IDBAny(const IDBKeyPath& value)
    : m_type(KeyPathType)
    , m_idbKeyPath(value)
    , m_integer(0)
{
}

IDBAny::IDBAny(const String& value)
    : m_type(StringType)
    , m_string(value)
    , m_integer(0)
{
}

IDBAny::IDBAny(int64_t value)
    : m_type(IntegerType)
    , m_integer(value)
{
}

} // namespace WebCore

#endif
