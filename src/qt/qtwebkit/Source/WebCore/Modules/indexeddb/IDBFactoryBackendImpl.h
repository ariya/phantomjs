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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#ifndef IDBFactoryBackendImpl_h
#define IDBFactoryBackendImpl_h

#if ENABLE(INDEXED_DATABASE)

#include "IDBCallbacks.h"
#include "IDBDatabaseCallbacks.h"
#include "IDBFactoryBackendInterface.h"
#include "SecurityOrigin.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/RefCounted.h>
#include <wtf/WeakPtr.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class DOMStringList;

class IDBBackingStore;
class IDBDatabaseBackendImpl;

class IDBFactoryBackendImpl : public IDBFactoryBackendInterface {
public:
    static PassRefPtr<IDBFactoryBackendImpl> create()
    {
        return adoptRef(new IDBFactoryBackendImpl());
    }
    virtual ~IDBFactoryBackendImpl();

    // Notifications from weak pointers.
    virtual void removeIDBDatabaseBackend(const String& uniqueIdentifier);

    virtual void getDatabaseNames(PassRefPtr<IDBCallbacks>, PassRefPtr<SecurityOrigin>, ScriptExecutionContext*, const String& dataDir);
    virtual void open(const String& name, int64_t version, int64_t transactionId, PassRefPtr<IDBCallbacks>, PassRefPtr<IDBDatabaseCallbacks>, PassRefPtr<SecurityOrigin>, ScriptExecutionContext*, const String& dataDir);

    virtual void deleteDatabase(const String& name, PassRefPtr<IDBCallbacks>, PassRefPtr<SecurityOrigin>, ScriptExecutionContext*, const String& dataDir);

protected:
    IDBFactoryBackendImpl();
    virtual PassRefPtr<IDBBackingStore> openBackingStore(PassRefPtr<SecurityOrigin>, const String& dataDir);

private:
    typedef HashMap<String, RefPtr<IDBDatabaseBackendImpl> > IDBDatabaseBackendMap;
    IDBDatabaseBackendMap m_databaseBackendMap;

    typedef HashMap<String, WeakPtr<IDBBackingStore> > IDBBackingStoreMap;
    IDBBackingStoreMap m_backingStoreMap;

    HashSet<RefPtr<IDBBackingStore> > m_sessionOnlyBackingStores;

    // Only one instance of the factory should exist at any given time.
    static IDBFactoryBackendImpl* idbFactoryBackendImpl;
};

} // namespace WebCore

#endif

#endif // IDBFactoryBackendImpl_h
