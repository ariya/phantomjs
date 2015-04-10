/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef WeakGCMap_h
#define WeakGCMap_h

#include <heap/Weak.h>
#include <heap/WeakInlines.h>
#include <wtf/HashMap.h>

namespace JSC {

// A HashMap with Weak<JSCell> values, which automatically removes values once they're garbage collected.

template<typename KeyArg, typename RawMappedArg, typename HashArg = typename DefaultHash<KeyArg>::Hash,
    typename KeyTraitsArg = HashTraits<KeyArg> >
class WeakGCMap : public HashMap<KeyArg, Weak<RawMappedArg>, HashArg, KeyTraitsArg> {
    typedef Weak<RawMappedArg> MappedType;
    typedef HashMap<KeyArg, MappedType, HashArg, KeyTraitsArg> Base;
    typedef WeakGCMap<KeyArg, RawMappedArg, HashArg, KeyTraitsArg> Self;
    typedef HashTraits<MappedType> MappedTraits;
    typedef typename MappedTraits::PassInType MappedPassInType;

public:
    typedef typename Base::KeyType KeyType;
    typedef typename Base::AddResult AddResult;
    typedef typename Base::iterator iterator;
    typedef typename Base::const_iterator const_iterator;
    using Base::begin;
    using Base::end;
    using Base::size;
    using Base::remove;

    WeakGCMap()
        : m_gcThreshold(minGCThreshold)
    {
    }

    AddResult set(const KeyType& key, MappedPassInType value)
    {
        gcMapIfNeeded();
        return Base::set(key, value);
    }

    AddResult add(const KeyType& key, MappedPassInType value)
    {
        gcMapIfNeeded();
        AddResult addResult = Base::add(key, nullptr);
        if (!addResult.iterator->value) { // New value or found a zombie value.
            addResult.isNewEntry = true;
            addResult.iterator->value = value;
        }
        return addResult;
    }

    iterator find(const KeyType& key)
    {
        iterator it = Base::find(key);
        iterator end = Base::end();
        if (it != end && !it->value) // Found a zombie value.
            return end;
        return it;
    }

    const_iterator find(const KeyType& key) const
    {
        return const_cast<Self*>(this)->find(key);
    }

    bool contains(const KeyType& key) const
    {
        return find(key) != end();
    }

private:
    static const int minGCThreshold = 3;

    void gcMap()
    {
        Vector<KeyType, 4> zombies;
        iterator end = this->end();
        for (iterator it = begin(); it != end; ++it) {
            if (!it->value)
                zombies.append(it->key);
        }
        for (size_t i = 0; i < zombies.size(); ++i)
            remove(zombies[i]);
    }

    void gcMapIfNeeded()
    {
        if (size() < m_gcThreshold)
            return;

        gcMap();
        m_gcThreshold = std::max(minGCThreshold, size() * 2 - 1);
    }

    int m_gcThreshold;
};

template<typename KeyArg, typename RawMappedArg, typename HashArg, typename KeyTraitsArg>
const int WeakGCMap<KeyArg, RawMappedArg, HashArg, KeyTraitsArg>::minGCThreshold;

} // namespace JSC

#endif // WeakGCMap_h
