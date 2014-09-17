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

#ifndef IDBKeyRange_h
#define IDBKeyRange_h

#if ENABLE(INDEXED_DATABASE)

#include "IDBKey.h"
#include "OptionsObject.h"
#include <wtf/PassRefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

class IDBKeyRange : public ThreadSafeRefCounted<IDBKeyRange> {
public:
    static PassRefPtr<IDBKeyRange> create(PassRefPtr<IDBKey> lower, PassRefPtr<IDBKey> upper, bool lowerOpen, bool upperOpen)
    {
        return adoptRef(new IDBKeyRange(lower, upper, lowerOpen, upperOpen));
    }
    ~IDBKeyRange() { }

    PassRefPtr<IDBKey> lower() const { return m_lower; }
    PassRefPtr<IDBKey> upper() const { return m_upper; }
    bool lowerOpen() const { return m_lowerOpen; }
    bool upperOpen() const { return m_upperOpen; }

    static PassRefPtr<IDBKeyRange> only(PassRefPtr<IDBKey> value);
    static PassRefPtr<IDBKeyRange> lowerBound(PassRefPtr<IDBKey> bound, bool open = false);
    static PassRefPtr<IDBKeyRange> upperBound(PassRefPtr<IDBKey> bound, bool open = false);
    static PassRefPtr<IDBKeyRange> bound(PassRefPtr<IDBKey> lower, PassRefPtr<IDBKey> upper, bool lowerOpen = false, bool upperOpen = false);

private:
    IDBKeyRange(PassRefPtr<IDBKey> lower, PassRefPtr<IDBKey> upper, bool lowerOpen, bool upperOpen);

    RefPtr<IDBKey> m_lower;
    RefPtr<IDBKey> m_upper;
    bool m_lowerOpen;
    bool m_upperOpen;
};

} // namespace WebCore

#endif

#endif // IDBKeyRange_h
