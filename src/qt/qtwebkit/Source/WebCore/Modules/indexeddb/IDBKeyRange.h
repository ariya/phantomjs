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

#include "Dictionary.h"
#include "IDBKey.h"
#include "ScriptWrappable.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

typedef int ExceptionCode;

class IDBKeyRange : public ScriptWrappable, public RefCounted<IDBKeyRange> {
public:
    enum LowerBoundType {
        LowerBoundOpen,
        LowerBoundClosed
    };
    enum UpperBoundType {
        UpperBoundOpen,
        UpperBoundClosed
    };

    static PassRefPtr<IDBKeyRange> create(PassRefPtr<IDBKey> lower, PassRefPtr<IDBKey> upper, LowerBoundType lowerType, UpperBoundType upperType)
    {
        return adoptRef(new IDBKeyRange(lower, upper, lowerType, upperType));
    }
    static PassRefPtr<IDBKeyRange> create(PassRefPtr<IDBKey> prpKey);
    ~IDBKeyRange() { }

    PassRefPtr<IDBKey> lower() const { return m_lower; }
    PassRefPtr<IDBKey> upper() const { return m_upper; }

    ScriptValue lowerValue(ScriptExecutionContext*) const;
    ScriptValue upperValue(ScriptExecutionContext*) const;
    bool lowerOpen() const { return m_lowerType == LowerBoundOpen; }
    bool upperOpen() const { return m_upperType == UpperBoundOpen; }

    static PassRefPtr<IDBKeyRange> only(PassRefPtr<IDBKey> value, ExceptionCode&);
    static PassRefPtr<IDBKeyRange> only(ScriptExecutionContext*, const ScriptValue& key, ExceptionCode&);

    static PassRefPtr<IDBKeyRange> lowerBound(ScriptExecutionContext* context, const ScriptValue& bound, ExceptionCode& ec) { return lowerBound(context, bound, false, ec); }
    static PassRefPtr<IDBKeyRange> lowerBound(ScriptExecutionContext*, const ScriptValue& bound, bool open, ExceptionCode&);

    static PassRefPtr<IDBKeyRange> upperBound(ScriptExecutionContext* context, const ScriptValue& bound, ExceptionCode& ec) { return upperBound(context, bound, false, ec); }
    static PassRefPtr<IDBKeyRange> upperBound(ScriptExecutionContext*, const ScriptValue& bound, bool open, ExceptionCode&);

    static PassRefPtr<IDBKeyRange> bound(ScriptExecutionContext* context, const ScriptValue& lower, const ScriptValue& upper, ExceptionCode& ec) { return bound(context, lower, upper, false, false, ec); }
    static PassRefPtr<IDBKeyRange> bound(ScriptExecutionContext* context, const ScriptValue& lower, const ScriptValue& upper, bool lowerOpen, ExceptionCode& ec) { return bound(context, lower, upper, lowerOpen, false, ec); }
    static PassRefPtr<IDBKeyRange> bound(ScriptExecutionContext*, const ScriptValue& lower, const ScriptValue& upper, bool lowerOpen, bool upperOpen, ExceptionCode&);

    bool isOnlyKey() const;

private:
    IDBKeyRange(PassRefPtr<IDBKey> lower, PassRefPtr<IDBKey> upper, LowerBoundType lowerType, UpperBoundType upperType);

    RefPtr<IDBKey> m_lower;
    RefPtr<IDBKey> m_upper;
    LowerBoundType m_lowerType;
    UpperBoundType m_upperType;
};

} // namespace WebCore

#endif

#endif // IDBKeyRange_h
