/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ArrayAllocationProfile_h
#define ArrayAllocationProfile_h

#include "IndexingType.h"
#include "JSArray.h"

namespace JSC {

class ArrayAllocationProfile {
public:
    ArrayAllocationProfile()
        : m_currentIndexingType(ArrayWithUndecided)
        , m_lastArray(0)
    {
    }
    
    IndexingType selectIndexingType()
    {
        if (m_lastArray && UNLIKELY(m_lastArray->structure()->indexingType() != m_currentIndexingType))
            updateIndexingType();
        return m_currentIndexingType;
    }
    
    JSArray* updateLastAllocation(JSArray* lastArray)
    {
        m_lastArray = lastArray;
        return lastArray;
    }
    
    JS_EXPORT_PRIVATE void updateIndexingType();
    
    static IndexingType selectIndexingTypeFor(ArrayAllocationProfile* profile)
    {
        if (!profile)
            return ArrayWithUndecided;
        return profile->selectIndexingType();
    }
    
    static JSArray* updateLastAllocationFor(ArrayAllocationProfile* profile, JSArray* lastArray)
    {
        if (profile)
            profile->updateLastAllocation(lastArray);
        return lastArray;
    }

private:
    
    IndexingType m_currentIndexingType;
    JSArray* m_lastArray;
};

} // namespace JSC

#endif // ArrayAllocationProfile_h

