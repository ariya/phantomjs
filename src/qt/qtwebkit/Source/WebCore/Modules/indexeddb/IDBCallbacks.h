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

#ifndef IDBCallbacks_h
#define IDBCallbacks_h

#include "IDBDatabaseBackendInterface.h"
#include "IDBDatabaseError.h"
#include "IDBKey.h"
#include "IDBKeyPath.h"
#include "SharedBuffer.h"
#include <wtf/RefCounted.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {
class DOMStringList;
class IDBCursorBackendInterface;

class IDBCallbacks : public RefCounted<IDBCallbacks> {
public:
    virtual ~IDBCallbacks() { }

    virtual void onError(PassRefPtr<IDBDatabaseError>) = 0;
    // From IDBFactory.webkitGetDatabaseNames()
    virtual void onSuccess(PassRefPtr<DOMStringList>) = 0;
    // From IDBObjectStore/IDBIndex.openCursor(), IDBIndex.openKeyCursor()
    virtual void onSuccess(PassRefPtr<IDBCursorBackendInterface>, PassRefPtr<IDBKey>, PassRefPtr<IDBKey> primaryKey, PassRefPtr<SharedBuffer>) = 0;
    // From IDBObjectStore.add()/put(), IDBIndex.getKey()
    virtual void onSuccess(PassRefPtr<IDBKey>) = 0;
    // From IDBObjectStore/IDBIndex.get()/count(), and various methods that yield null/undefined.
    virtual void onSuccess(PassRefPtr<SharedBuffer>) = 0;
    // From IDBObjectStore/IDBIndex.get() (with key injection)
    virtual void onSuccess(PassRefPtr<SharedBuffer>, PassRefPtr<IDBKey>, const IDBKeyPath&) = 0;
    // From IDBObjectStore/IDBIndex.count()
    virtual void onSuccess(int64_t value) = 0;

    // From IDBFactor.deleteDatabase(), IDBObjectStore/IDBIndex.get(), IDBObjectStore.delete(), IDBObjectStore.clear()
    virtual void onSuccess() = 0;

    // From IDBCursor.advance()/continue()
    virtual void onSuccess(PassRefPtr<IDBKey>, PassRefPtr<IDBKey> primaryKey, PassRefPtr<SharedBuffer>) = 0;
    // From IDBCursor.advance()/continue()
    virtual void onSuccessWithPrefetch(const Vector<RefPtr<IDBKey> >& keys, const Vector<RefPtr<IDBKey> >& primaryKeys, const Vector<RefPtr<SharedBuffer> >& values) = 0;
    // From IDBFactory.open()/deleteDatabase()
    virtual void onBlocked(int64_t /* existingVersion */) { ASSERT_NOT_REACHED(); }
    // From IDBFactory.open()
    virtual void onUpgradeNeeded(int64_t /* oldVersion */, PassRefPtr<IDBDatabaseBackendInterface>, const IDBDatabaseMetadata&) { ASSERT_NOT_REACHED(); }
    virtual void onSuccess(PassRefPtr<IDBDatabaseBackendInterface>, const IDBDatabaseMetadata&) { ASSERT_NOT_REACHED(); }
};

} // namespace WebCore

#endif

#endif // IDBCallbacks_h
