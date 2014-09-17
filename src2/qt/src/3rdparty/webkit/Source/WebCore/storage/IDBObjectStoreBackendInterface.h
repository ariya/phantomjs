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

#ifndef IDBObjectStoreBackendInterface_h
#define IDBObjectStoreBackendInterface_h

#include "ExceptionCode.h"
#include "PlatformString.h"
#include <wtf/Threading.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

class DOMStringList;
class IDBCallbacks;
class IDBIndexBackendInterface;
class IDBKey;
class IDBKeyRange;
class IDBTransactionBackendInterface;
class SerializedScriptValue;

class IDBObjectStoreBackendInterface : public ThreadSafeRefCounted<IDBObjectStoreBackendInterface> {
public:
    virtual ~IDBObjectStoreBackendInterface() { }

    virtual String name() const = 0;
    virtual String keyPath() const = 0;
    virtual PassRefPtr<DOMStringList> indexNames() const = 0;

    virtual void get(PassRefPtr<IDBKey>, PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, ExceptionCode&) = 0;

    enum PutMode {
        AddOrUpdate,
        AddOnly,
        CursorUpdate
    };
    virtual void put(PassRefPtr<SerializedScriptValue>, PassRefPtr<IDBKey>, PutMode, PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, ExceptionCode&) = 0;
    virtual void deleteFunction(PassRefPtr<IDBKey>, PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, ExceptionCode&) = 0;

    virtual void clear(PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, ExceptionCode&) = 0;

    virtual PassRefPtr<IDBIndexBackendInterface> createIndex(const String& name, const String& keyPath, bool unique, IDBTransactionBackendInterface*, ExceptionCode&) = 0;
    virtual PassRefPtr<IDBIndexBackendInterface> index(const String& name, ExceptionCode&) = 0;
    virtual void deleteIndex(const String& name, IDBTransactionBackendInterface*, ExceptionCode&) = 0;

    virtual void openCursor(PassRefPtr<IDBKeyRange>, unsigned short direction, PassRefPtr<IDBCallbacks>, IDBTransactionBackendInterface*, ExceptionCode&) = 0;
};

} // namespace WebCore

#endif

#endif // IDBObjectStoreBackendInterface_h
