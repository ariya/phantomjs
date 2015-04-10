/*
 * Copyright (C) 2009, 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef WeakInlines_h
#define WeakInlines_h

#include "PassWeak.h"
#include "WeakSetInlines.h"
#include <wtf/Assertions.h>
#include <wtf/HashTraits.h>

namespace JSC {

template<typename T> inline Weak<T>::Weak(T* cell, WeakHandleOwner* weakOwner, void* context)
    : m_impl(cell ? WeakSet::allocate(cell, weakOwner, context) : 0)
{
}

template<typename T> inline bool Weak<T>::isHashTableDeletedValue() const
{
    return m_impl == hashTableDeletedValue();
}

template<typename T> inline Weak<T>::Weak(typename Weak<T>::HashTableDeletedValueTag)
    : m_impl(hashTableDeletedValue())
{
}

template<typename T> template<typename U>  inline Weak<T>::Weak(const PassWeak<U>& other)
    : m_impl(other.leakImpl())
{
}

template<class T> inline void swap(Weak<T>& a, Weak<T>& b)
{
    a.swap(b);
}

template<typename T> inline void Weak<T>::swap(Weak& other)
{
    std::swap(m_impl, other.m_impl);
}

template<typename T> inline Weak<T>& Weak<T>::operator=(const PassWeak<T>& o)
{
    clear();
    m_impl = o.leakImpl();
    return *this;
}

template<typename T> inline T* Weak<T>::operator->() const
{
    ASSERT(m_impl && m_impl->state() == WeakImpl::Live);
    return jsCast<T*>(m_impl->jsValue().asCell());
}

template<typename T> inline T& Weak<T>::operator*() const
{
    ASSERT(m_impl && m_impl->state() == WeakImpl::Live);
    return *jsCast<T*>(m_impl->jsValue().asCell());
}

template<typename T> inline T* Weak<T>::get() const
{
    if (!m_impl || m_impl->state() != WeakImpl::Live)
        return 0;
    return jsCast<T*>(m_impl->jsValue().asCell());
}

template<typename T> inline bool Weak<T>::was(T* other) const
{
    return jsCast<T*>(m_impl->jsValue().asCell()) == other;
}

template<typename T> inline bool Weak<T>::operator!() const
{
    return !m_impl || !m_impl->jsValue() || m_impl->state() != WeakImpl::Live;
}

template<typename T> inline Weak<T>::operator UnspecifiedBoolType*() const
{
    return reinterpret_cast<UnspecifiedBoolType*>(!!*this);
}

template<typename T> inline PassWeak<T> Weak<T>::release()
{
    PassWeak<T> tmp = adoptWeak<T>(m_impl);
    m_impl = 0;
    return tmp;
}

template<typename T> inline WeakImpl* Weak<T>::hashTableDeletedValue()
{
    return reinterpret_cast<WeakImpl*>(-1);
}

template <typename T> inline bool operator==(const Weak<T>& lhs, const Weak<T>& rhs)
{
    return lhs.get() == rhs.get();
}

// This function helps avoid modifying a weak table while holding an iterator into it. (Object allocation
// can run a finalizer that modifies the table. We avoid that by requiring a pre-constructed object as our value.)
template<typename Map, typename Key, typename Value> inline void weakAdd(Map& map, const Key& key, Value value)
{
    ASSERT(!map.get(key));
    map.set(key, value); // The table may still have a zombie for value.
}

template<typename Map, typename Key, typename Value> inline void weakRemove(Map& map, const Key& key, Value value)
{
    typename Map::iterator it = map.find(key);
    ASSERT_UNUSED(value, value);
    ASSERT(it != map.end());
    ASSERT(it->value.was(value));
    ASSERT(!it->value);
    map.remove(it);
}

template<typename T> inline void weakClear(Weak<T>& weak, T* cell)
{
    ASSERT_UNUSED(cell, cell);
    ASSERT(weak.was(cell));
    ASSERT(!weak);
    weak.clear();
}

} // namespace JSC

namespace WTF {

template<typename T> struct VectorTraits<JSC::Weak<T> > : SimpleClassVectorTraits {
    static const bool canCompareWithMemcmp = false;
};

template<typename T> struct HashTraits<JSC::Weak<T> > : SimpleClassHashTraits<JSC::Weak<T> > {
    typedef JSC::Weak<T> StorageType;

    typedef std::nullptr_t EmptyValueType;
    static EmptyValueType emptyValue() { return nullptr; }

    typedef JSC::PassWeak<T> PassInType;
    static void store(PassInType value, StorageType& storage) { storage = value; }

    typedef JSC::PassWeak<T> PassOutType;
    static PassOutType passOut(StorageType& value) { return value.release(); }
    static PassOutType passOut(EmptyValueType) { return PassOutType(); }

    typedef T* PeekType;
    static PeekType peek(const StorageType& value) { return value.get(); }
    static PeekType peek(EmptyValueType) { return PeekType(); }
};

} // namespace WTF

#endif // WeakInlines_h
