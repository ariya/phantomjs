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

#ifndef IDBCursorBackendInterface_h
#define IDBCursorBackendInterface_h

#if ENABLE(INDEXED_DATABASE)

#include "ExceptionCode.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Threading.h>

namespace WebCore {

class IDBAny;
class IDBCallbacks;
class IDBKey;
class IDBRequest;
class SerializedScriptValue;

class IDBCursorBackendInterface : public ThreadSafeRefCounted<IDBCursorBackendInterface> {
public:
    virtual ~IDBCursorBackendInterface() {}

    enum CursorType {
        InvalidCursorType = 0,
        IndexCursor,
        IndexKeyCursor,
        ObjectStoreCursor
    };

    virtual unsigned short direction() const = 0;
    virtual PassRefPtr<IDBKey> key() const = 0;
    virtual PassRefPtr<IDBKey> primaryKey() const = 0;
    virtual PassRefPtr<SerializedScriptValue> value() const = 0;

    virtual void update(PassRefPtr<SerializedScriptValue>, PassRefPtr<IDBCallbacks>, ExceptionCode&) = 0;
    virtual void continueFunction(PassRefPtr<IDBKey> key, PassRefPtr<IDBCallbacks>, ExceptionCode&) = 0;
    virtual void deleteFunction(PassRefPtr<IDBCallbacks>, ExceptionCode&) = 0;
};

} // namespace WebCore

#endif

#endif // IDBCursorBackendInterface_h
