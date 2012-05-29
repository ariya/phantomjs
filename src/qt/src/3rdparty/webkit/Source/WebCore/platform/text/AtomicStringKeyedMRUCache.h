/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AtomicStringKeyedMRUCache_h
#define AtomicStringKeyedMRUCache_h

#include <wtf/text/AtomicString.h>

namespace WebCore {

template<typename T, size_t capacity = 4>
class AtomicStringKeyedMRUCache {
public:
    T get(const AtomicString& key)
    {
        if (key.isNull()) {
            DEFINE_STATIC_LOCAL(T, valueForNull, (createValueForNullKey()));
            return valueForNull;
        }

        for (size_t i = 0; i < m_cache.size(); ++i) {
            if (m_cache[i].first == key) {
                size_t foundIndex = i;
                if (foundIndex + 1 < m_cache.size()) {
                    Entry entry = m_cache[foundIndex];
                    m_cache.remove(foundIndex);
                    foundIndex = m_cache.size();
                    m_cache.append(entry);
                }
                return m_cache[foundIndex].second;
            }
        }
        if (m_cache.size() == capacity)
            m_cache.remove(0);

        m_cache.append(std::make_pair(key, createValueForKey(key)));
        return m_cache.last().second;
    }

private:
    T createValueForNullKey();
    T createValueForKey(const AtomicString&);

    typedef pair<AtomicString, T> Entry;
    typedef Vector<Entry, capacity> Cache;
    Cache m_cache;
};

}

#endif // AtomicStringKeyedMRUCache_h
