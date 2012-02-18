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

#include "config.h"

#include "RegExpCache.h"

namespace JSC {

PassRefPtr<RegExp> RegExpCache::lookupOrCreate(const UString& patternString, RegExpFlags flags)
{
    if (patternString.length() < maxCacheablePatternLength) {
        pair<RegExpCacheMap::iterator, bool> result = m_cacheMap.add(RegExpKey(flags, patternString), 0);
        if (!result.second)
            return result.first->second;
        else
            return create(patternString, flags, result.first);
    }
    return create(patternString, flags, m_cacheMap.end());
}

PassRefPtr<RegExp> RegExpCache::create(const UString& patternString, RegExpFlags flags, RegExpCacheMap::iterator iterator) 
{
    RefPtr<RegExp> regExp = RegExp::create(m_globalData, patternString, flags);

    if (patternString.length() >= maxCacheablePatternLength)
        return regExp;

    RegExpKey key = RegExpKey(flags, patternString);
    iterator->first = key;
    iterator->second = regExp;

    ++m_nextKeyToEvict;
    if (m_nextKeyToEvict == maxCacheableEntries) {
        m_nextKeyToEvict = 0;
        m_isFull = true;
    }
    if (m_isFull)
        m_cacheMap.remove(RegExpKey(patternKeyArray[m_nextKeyToEvict].flagsValue, patternKeyArray[m_nextKeyToEvict].pattern));

    patternKeyArray[m_nextKeyToEvict].flagsValue = key.flagsValue;
    patternKeyArray[m_nextKeyToEvict].pattern = patternString.impl();
    return regExp;
}

RegExpCache::RegExpCache(JSGlobalData* globalData)
    : m_globalData(globalData)
    , m_nextKeyToEvict(-1)
    , m_isFull(false)
{
}

}
