/*
 * Copyright (C) 2010 University of Szeged
 * Copyright (C) 2010 Renata Hodovan (hodovan@inf.u-szeged.hu)
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "RegExp.h"
#include "RegExpKey.h"
#include "UString.h"
#include <wtf/FixedArray.h>
#include <wtf/HashMap.h>

#ifndef RegExpCache_h
#define RegExpCache_h

namespace JSC {

class RegExpCache {

typedef HashMap<RegExpKey, RefPtr<RegExp> > RegExpCacheMap;

public:
    PassRefPtr<RegExp> lookupOrCreate(const UString& patternString, RegExpFlags);
    PassRefPtr<RegExp> create(const UString& patternString, RegExpFlags, RegExpCacheMap::iterator);
    RegExpCache(JSGlobalData* globalData);

private:
    static const unsigned maxCacheablePatternLength = 256;

#if PLATFORM(IOS)
    // The RegExpCache can currently hold onto multiple Mb of memory;
    // as a short-term fix some embedded platforms may wish to reduce the cache size.
    static const int maxCacheableEntries = 32;
#else
    static const int maxCacheableEntries = 256;
#endif

    FixedArray<RegExpKey, maxCacheableEntries> patternKeyArray;
    RegExpCacheMap m_cacheMap;
    JSGlobalData* m_globalData;
    int m_nextKeyToEvict;
    bool m_isFull;
};

} // namespace JSC

#endif // RegExpCache_h
