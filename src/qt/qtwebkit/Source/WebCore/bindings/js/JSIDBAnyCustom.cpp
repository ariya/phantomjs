/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INDEXED_DATABASE)

#include "JSIDBAny.h"

#include "IDBAny.h"
#include "IDBCursor.h"
#include "IDBCursorWithValue.h"
#include "IDBDatabase.h"
#include "IDBFactory.h"
#include "IDBIndex.h"
#include "IDBObjectStore.h"
#include "JSDOMStringList.h"
#include "JSIDBCursor.h"
#include "JSIDBCursorWithValue.h"
#include "JSIDBDatabase.h"
#include "JSIDBFactory.h"
#include "JSIDBIndex.h"
#include "JSIDBObjectStore.h"
#include "JSIDBTransaction.h"
#include "SerializedScriptValue.h"

using namespace JSC;

namespace WebCore {

JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, const IDBKeyPath& value)
{
    switch (value.type()) {
    case IDBKeyPath::NullType:
        return jsNull();
    case IDBKeyPath::StringType:
        return jsStringWithCache(exec, value.string());
    case IDBKeyPath::ArrayType:
        RefPtr<DOMStringList> keyPaths = DOMStringList::create();
        for (Vector<String>::const_iterator it = value.array().begin(); it != value.array().end(); ++it)
            keyPaths->append(*it);
        return toJS(exec, globalObject, keyPaths.release());
    }

    ASSERT_NOT_REACHED();
    return jsUndefined();
}

JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, IDBAny* idbAny)
{
    if (!idbAny)
        return jsNull();

    switch (idbAny->type()) {
    case IDBAny::UndefinedType:
        return jsUndefined();
    case IDBAny::NullType:
        return jsNull();
    case IDBAny::DOMStringListType:
        return toJS(exec, globalObject, idbAny->domStringList());
    case IDBAny::IDBCursorType:
        return toJS(exec, globalObject, idbAny->idbCursor());
    case IDBAny::IDBCursorWithValueType:
        return wrap<JSIDBCursorWithValue>(exec, globalObject, idbAny->idbCursorWithValue().get());
    case IDBAny::IDBDatabaseType:
        return toJS(exec, globalObject, idbAny->idbDatabase());
    case IDBAny::IDBFactoryType:
        return toJS(exec, globalObject, idbAny->idbFactory());
    case IDBAny::IDBIndexType:
        return toJS(exec, globalObject, idbAny->idbIndex());
    case IDBAny::IDBObjectStoreType:
        return toJS(exec, globalObject, idbAny->idbObjectStore());
    case IDBAny::IDBTransactionType:
        return toJS(exec, globalObject, idbAny->idbTransaction());
    case IDBAny::ScriptValueType:
        return idbAny->scriptValue().jsValue();
    case IDBAny::StringType:
        return jsStringWithCache(exec, idbAny->string());
    case IDBAny::IntegerType:
        return jsNumber(idbAny->integer());
    case IDBAny::KeyPathType:
        return toJS(exec, globalObject, idbAny->keyPath());
    }

    ASSERT_NOT_REACHED();
    return jsUndefined();
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
