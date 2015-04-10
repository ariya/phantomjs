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

#ifndef IDBAny_h
#define IDBAny_h

#if ENABLE(INDEXED_DATABASE)

#include "IDBKeyPath.h"
#include "ScriptValue.h"
#include "ScriptWrappable.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class DOMStringList;
class IDBCursor;
class IDBCursorWithValue;
class IDBDatabase;
class IDBFactory;
class IDBIndex;
class IDBKeyPath;
class IDBObjectStore;
class IDBTransaction;

class IDBAny : public ScriptWrappable, public RefCounted<IDBAny> {
public:
    static PassRefPtr<IDBAny> createInvalid();
    static PassRefPtr<IDBAny> createNull();
    static PassRefPtr<IDBAny> createString(const String&);
    template<typename T>
    static PassRefPtr<IDBAny> create(T* idbObject)
    {
        return adoptRef(new IDBAny(idbObject));
    }
    template<typename T>
    static PassRefPtr<IDBAny> create(const T& idbObject)
    {
        return adoptRef(new IDBAny(idbObject));
    }
    template<typename T>
    static PassRefPtr<IDBAny> create(PassRefPtr<T> idbObject)
    {
        return adoptRef(new IDBAny(idbObject));
    }
    static PassRefPtr<IDBAny> create(int64_t value)
    {
        return adoptRef(new IDBAny(value));
    }
    ~IDBAny();

    enum Type {
        UndefinedType = 0,
        NullType,
        DOMStringListType,
        IDBCursorType,
        IDBCursorWithValueType,
        IDBDatabaseType,
        IDBFactoryType,
        IDBIndexType,
        IDBObjectStoreType,
        IDBTransactionType,
        ScriptValueType,
        IntegerType,
        StringType,
        KeyPathType,
    };

    Type type() const { return m_type; }
    // Use type() to figure out which one of these you're allowed to call.
    PassRefPtr<DOMStringList> domStringList();
    PassRefPtr<IDBCursor> idbCursor();
    PassRefPtr<IDBCursorWithValue> idbCursorWithValue();
    PassRefPtr<IDBDatabase> idbDatabase();
    PassRefPtr<IDBFactory> idbFactory();
    PassRefPtr<IDBIndex> idbIndex();
    PassRefPtr<IDBObjectStore> idbObjectStore();
    PassRefPtr<IDBTransaction> idbTransaction();
    const ScriptValue& scriptValue();
    int64_t integer();
    const String& string();
    const IDBKeyPath& keyPath() { return m_idbKeyPath; };

private:
    explicit IDBAny(Type);
    explicit IDBAny(PassRefPtr<DOMStringList>);
    explicit IDBAny(PassRefPtr<IDBCursor>);
    explicit IDBAny(PassRefPtr<IDBCursorWithValue>);
    explicit IDBAny(PassRefPtr<IDBDatabase>);
    explicit IDBAny(PassRefPtr<IDBFactory>);
    explicit IDBAny(PassRefPtr<IDBIndex>);
    explicit IDBAny(PassRefPtr<IDBObjectStore>);
    explicit IDBAny(PassRefPtr<IDBTransaction>);
    explicit IDBAny(const IDBKeyPath&);
    explicit IDBAny(const String&);
    explicit IDBAny(const ScriptValue&);
    explicit IDBAny(int64_t);

    const Type m_type;

    // Only one of the following should ever be in use at any given time.
    const RefPtr<DOMStringList> m_domStringList;
    const RefPtr<IDBCursor> m_idbCursor;
    const RefPtr<IDBCursorWithValue> m_idbCursorWithValue;
    const RefPtr<IDBDatabase> m_idbDatabase;
    const RefPtr<IDBFactory> m_idbFactory;
    const RefPtr<IDBIndex> m_idbIndex;
    const RefPtr<IDBObjectStore> m_idbObjectStore;
    const RefPtr<IDBTransaction> m_idbTransaction;
    const IDBKeyPath m_idbKeyPath;
    const ScriptValue m_scriptValue;
    const String m_string;
    const int64_t m_integer;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBAny_h
