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

#ifndef IDBObjectStore_h
#define IDBObjectStore_h

#include "IDBCursor.h"
#include "IDBIndex.h"
#include "IDBKey.h"
#include "IDBKeyRange.h"
#include "IDBObjectStoreBackendInterface.h"
#include "IDBRequest.h"
#include "OptionsObject.h"
#include "PlatformString.h"
#include "SerializedScriptValue.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

class DOMStringList;
class IDBAny;

class IDBObjectStore : public RefCounted<IDBObjectStore> {
public:
    static PassRefPtr<IDBObjectStore> create(PassRefPtr<IDBObjectStoreBackendInterface> idbObjectStore, IDBTransaction* transaction)
    {
        return adoptRef(new IDBObjectStore(idbObjectStore, transaction));
    }
    ~IDBObjectStore() { }

    String name() const;
    String keyPath() const;
    PassRefPtr<DOMStringList> indexNames() const;

    // FIXME: Try to modify the code generator so this is unneeded.
    PassRefPtr<IDBRequest> add(ScriptExecutionContext* context, PassRefPtr<SerializedScriptValue> value, ExceptionCode& ec) { return add(context, value, 0, ec);  }
    PassRefPtr<IDBRequest> put(ScriptExecutionContext* context, PassRefPtr<SerializedScriptValue> value, ExceptionCode& ec) { return put(context, value, 0, ec);  }
    PassRefPtr<IDBIndex> createIndex(const String& name, const String& keyPath, ExceptionCode& ec) { return createIndex(name, keyPath, OptionsObject(), ec); }
    PassRefPtr<IDBRequest> openCursor(ScriptExecutionContext* context, ExceptionCode& ec) { return openCursor(context, 0, ec); } 
    PassRefPtr<IDBRequest> openCursor(ScriptExecutionContext* context, PassRefPtr<IDBKeyRange> keyRange, ExceptionCode& ec) { return openCursor(context, keyRange, IDBCursor::NEXT, ec); } 

    PassRefPtr<IDBRequest> get(ScriptExecutionContext*, PassRefPtr<IDBKey>, ExceptionCode&);
    PassRefPtr<IDBRequest> add(ScriptExecutionContext*, PassRefPtr<SerializedScriptValue>, PassRefPtr<IDBKey>, ExceptionCode&);
    PassRefPtr<IDBRequest> put(ScriptExecutionContext*, PassRefPtr<SerializedScriptValue>, PassRefPtr<IDBKey>, ExceptionCode&);
    PassRefPtr<IDBRequest> deleteFunction(ScriptExecutionContext*, PassRefPtr<IDBKey> key, ExceptionCode&);
    PassRefPtr<IDBRequest> clear(ScriptExecutionContext*, ExceptionCode&);

    PassRefPtr<IDBIndex> createIndex(const String& name, const String& keyPath, const OptionsObject&, ExceptionCode&);
    PassRefPtr<IDBIndex> index(const String& name, ExceptionCode&);
    void deleteIndex(const String& name, ExceptionCode&);

    PassRefPtr<IDBRequest> openCursor(ScriptExecutionContext*, PassRefPtr<IDBKeyRange>, unsigned short direction, ExceptionCode&); 

private:
    IDBObjectStore(PassRefPtr<IDBObjectStoreBackendInterface>, IDBTransaction*);
    void removeTransactionFromPendingList();

    RefPtr<IDBObjectStoreBackendInterface> m_objectStore;
    RefPtr<IDBTransaction> m_transaction;
};

} // namespace WebCore

#endif

#endif // IDBObjectStore_h

