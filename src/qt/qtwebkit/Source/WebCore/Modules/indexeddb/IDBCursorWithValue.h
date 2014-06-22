/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef IDBCursorWithValue_h
#define IDBCursorWithValue_h

#if ENABLE(INDEXED_DATABASE)

#include "IDBCursor.h"

namespace WebCore {

class IDBCursorWithValue : public IDBCursor {
public:
    static PassRefPtr<IDBCursorWithValue> create(PassRefPtr<IDBCursorBackendInterface>, IndexedDB::CursorDirection, IDBRequest*, IDBAny* source, IDBTransaction*);
    static PassRefPtr<IDBCursorWithValue> fromCursor(PassRefPtr<IDBCursor>);
    virtual ~IDBCursorWithValue();

    // The value attribute defined in the IDL is simply implemented in IDBCursor (but not exposed via
    // its IDL). This is to make the implementation more simple while matching what the spec says.

protected:
    virtual bool isKeyCursor() const OVERRIDE { return false; }

private:
    IDBCursorWithValue(PassRefPtr<IDBCursorBackendInterface>, IndexedDB::CursorDirection, IDBRequest*, IDBAny* source, IDBTransaction*);
};

} // namespace WebCore

#endif

#endif // IDBCursorWithValue_h
