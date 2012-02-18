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
#include "IDBKeyRange.h"

#include "IDBKey.h"

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

IDBKeyRange::IDBKeyRange(PassRefPtr<IDBKey> lower, PassRefPtr<IDBKey> upper, bool lowerOpen, bool upperOpen)
    : m_lower(lower)
    , m_upper(upper)
    , m_lowerOpen(lowerOpen)
    , m_upperOpen(upperOpen)
{
}

PassRefPtr<IDBKeyRange> IDBKeyRange::only(PassRefPtr<IDBKey> prpValue)
{
    RefPtr<IDBKey> value = prpValue;
    return IDBKeyRange::create(value, value, false, false);
}

PassRefPtr<IDBKeyRange> IDBKeyRange::lowerBound(PassRefPtr<IDBKey> bound, bool open)
{
    return IDBKeyRange::create(bound, 0, open, false);
}

PassRefPtr<IDBKeyRange> IDBKeyRange::upperBound(PassRefPtr<IDBKey> bound, bool open)
{
    return IDBKeyRange::create(0, bound, false, open);
}

PassRefPtr<IDBKeyRange> IDBKeyRange::bound(PassRefPtr<IDBKey> lower, PassRefPtr<IDBKey> upper, bool lowerOpen, bool upperOpen)
{
    return IDBKeyRange::create(lower, upper, lowerOpen, upperOpen);
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
