/*
 * Copyright (C) 2012, 2013 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "config.h"
#include "DocumentSharedObjectPool.h"

#include "Element.h"

namespace WebCore {

class ShareableElementDataCacheKey {
public:
    ShareableElementDataCacheKey(const Attribute* attributes, unsigned attributeCount)
        : m_attributes(attributes)
        , m_attributeCount(attributeCount)
    { }

    bool operator!=(const ShareableElementDataCacheKey& other) const
    {
        if (m_attributeCount != other.m_attributeCount)
            return true;
        return memcmp(m_attributes, other.m_attributes, sizeof(Attribute) * m_attributeCount);
    }

    unsigned hash() const
    {
        return StringHasher::hashMemory(m_attributes, m_attributeCount * sizeof(Attribute));
    }

private:
    const Attribute* m_attributes;
    unsigned m_attributeCount;
};

class ShareableElementDataCacheEntry {
public:
    ShareableElementDataCacheEntry(const ShareableElementDataCacheKey& k, PassRefPtr<ShareableElementData> v)
        : key(k)
        , value(v)
    { }

    ShareableElementDataCacheKey key;
    RefPtr<ShareableElementData> value;
};

PassRefPtr<ShareableElementData> DocumentSharedObjectPool::cachedShareableElementDataWithAttributes(const Vector<Attribute>& attributes)
{
    ASSERT(!attributes.isEmpty());

    ShareableElementDataCacheKey cacheKey(attributes.data(), attributes.size());
    unsigned cacheHash = cacheKey.hash();

    ShareableElementDataCache::iterator cacheIterator = m_shareableElementDataCache.add(cacheHash, nullptr).iterator;
    if (cacheIterator->value && cacheIterator->value->key != cacheKey)
        cacheHash = 0;

    RefPtr<ShareableElementData> elementData;
    if (cacheHash && cacheIterator->value)
        elementData = cacheIterator->value->value;
    else
        elementData = ShareableElementData::createWithAttributes(attributes);

    if (!cacheHash || cacheIterator->value)
        return elementData.release();

    cacheIterator->value = adoptPtr(new ShareableElementDataCacheEntry(ShareableElementDataCacheKey(elementData->m_attributeArray, elementData->length()), elementData));

    return elementData.release();
}

DocumentSharedObjectPool::DocumentSharedObjectPool()
{
}

DocumentSharedObjectPool::~DocumentSharedObjectPool()
{
}

}
