/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef PassWeak_h
#define PassWeak_h

#include "JSCell.h"
#include "WeakSetInlines.h"
#include <wtf/Assertions.h>
#include <wtf/NullPtr.h>
#include <wtf/TypeTraits.h>

namespace JSC {

template<typename T> class PassWeak;
template<typename T> PassWeak<T> adoptWeak(WeakImpl*);

template<typename T> class PassWeak {
public:
    PassWeak();
    PassWeak(std::nullptr_t);
    PassWeak(T*, WeakHandleOwner* = 0, void* context = 0);

    // It somewhat breaks the type system to allow transfer of ownership out of
    // a const PassWeak. However, it makes it much easier to work with PassWeak
    // temporaries, and we don't have a need to use real const PassWeaks anyway.
    PassWeak(const PassWeak&);
    template<typename U> PassWeak(const PassWeak<U>&);

    ~PassWeak();

    T* operator->() const;
    T& operator*() const;
    T* get() const;

    bool operator!() const;

    // This conversion operator allows implicit conversion to bool but not to other integer types.
    typedef void* (PassWeak::*UnspecifiedBoolType);
    operator UnspecifiedBoolType*() const;

    WeakImpl* leakImpl() const WARN_UNUSED_RETURN;

private:
    friend PassWeak adoptWeak<T>(WeakImpl*);
    explicit PassWeak(WeakImpl*);

    WeakImpl* m_impl;
};

template<typename T> inline PassWeak<T>::PassWeak()
    : m_impl(0)
{
}

template<typename T> inline PassWeak<T>::PassWeak(std::nullptr_t)
    : m_impl(0)
{
}

template<typename T> inline PassWeak<T>::PassWeak(T* cell, WeakHandleOwner* weakOwner, void* context)
    : m_impl(cell ? WeakSet::allocate(cell, weakOwner, context) : 0)
{
}

template<typename T> inline PassWeak<T>::PassWeak(const PassWeak& o)
    : m_impl(o.leakImpl())
{
}

template<typename T> template<typename U> inline PassWeak<T>::PassWeak(const PassWeak<U>& o)
    : m_impl(o.leakImpl())
{
}

template<typename T> inline PassWeak<T>::~PassWeak()
{
    if (!m_impl)
        return;
    WeakSet::deallocate(m_impl);
}

template<typename T> inline T* PassWeak<T>::operator->() const
{
    ASSERT(m_impl && m_impl->state() == WeakImpl::Live);
    return jsCast<T*>(m_impl->jsValue().asCell());
}

template<typename T> inline T& PassWeak<T>::operator*() const
{
    ASSERT(m_impl && m_impl->state() == WeakImpl::Live);
    return *jsCast<T*>(m_impl->jsValue().asCell());
}

template<typename T> inline T* PassWeak<T>::get() const
{
    if (!m_impl || m_impl->state() != WeakImpl::Live)
        return 0;
    return jsCast<T*>(m_impl->jsValue().asCell());
}

template<typename T> inline bool PassWeak<T>::operator!() const
{
    return !m_impl || m_impl->state() != WeakImpl::Live || !m_impl->jsValue();
}

template<typename T> inline PassWeak<T>::operator UnspecifiedBoolType*() const
{
    return reinterpret_cast<UnspecifiedBoolType*>(!!*this);
}

template<typename T> inline PassWeak<T>::PassWeak(WeakImpl* impl)
: m_impl(impl)
{
}

template<typename T> inline WeakImpl* PassWeak<T>::leakImpl() const
{
    WeakImpl* tmp = 0;
    std::swap(tmp, const_cast<WeakImpl*&>(m_impl));
    return tmp;
}

template<typename T> PassWeak<T> inline adoptWeak(WeakImpl* impl)
{
    return PassWeak<T>(impl);
}

template<typename T, typename U> inline bool operator==(const PassWeak<T>& a, const PassWeak<U>& b) 
{
    return a.get() == b.get(); 
}

template<typename T, typename U> inline bool operator==(const PassWeak<T>& a, const Weak<U>& b) 
{
    return a.get() == b.get(); 
}

template<typename T, typename U> inline bool operator==(const Weak<T>& a, const PassWeak<U>& b) 
{
    return a.get() == b.get(); 
}

template<typename T, typename U> inline bool operator==(const PassWeak<T>& a, U* b) 
{
    return a.get() == b; 
}

template<typename T, typename U> inline bool operator==(T* a, const PassWeak<U>& b) 
{
    return a == b.get(); 
}

template<typename T, typename U> inline bool operator!=(const PassWeak<T>& a, const PassWeak<U>& b) 
{
    return a.get() != b.get(); 
}

template<typename T, typename U> inline bool operator!=(const PassWeak<T>& a, const Weak<U>& b) 
{
    return a.get() != b.get(); 
}

template<typename T, typename U> inline bool operator!=(const Weak<T>& a, const PassWeak<U>& b) 
{
    return a.get() != b.get(); 
}

template<typename T, typename U> inline bool operator!=(const PassWeak<T>& a, U* b)
{
    return a.get() != b; 
}

template<typename T, typename U> inline bool operator!=(T* a, const PassWeak<U>& b) 
{
    return a != b.get(); 
}

} // namespace JSC

#endif // PassWeak_h
