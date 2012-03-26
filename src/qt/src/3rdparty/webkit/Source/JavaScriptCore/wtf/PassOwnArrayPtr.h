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

#ifndef WTF_PassOwnArrayPtr_h
#define WTF_PassOwnArrayPtr_h

#include "Assertions.h"
#include "NullPtr.h"
#include "TypeTraits.h"

namespace WTF {

template<typename T> class OwnArrayPtr;
template<typename T> class PassOwnArrayPtr;
template<typename T> PassOwnArrayPtr<T> adoptArrayPtr(T*);
template<typename T> void deleteOwnedArrayPtr(T* ptr);

template<typename T> class PassOwnArrayPtr {
public:
    typedef T* PtrType;

    PassOwnArrayPtr() : m_ptr(0) { }

#if !defined(LOOSE_PASS_OWN_PTR) || !HAVE(NULLPTR)
    PassOwnArrayPtr(std::nullptr_t) : m_ptr(0) { }
#endif

    // It somewhat breaks the type system to allow transfer of ownership out of
    // a const PassOwnArrayPtr. However, it makes it much easier to work with PassOwnArrayPtr
    // temporaries, and we don't have a need to use real const PassOwnArrayPtrs anyway.
    PassOwnArrayPtr(const PassOwnArrayPtr& o) : m_ptr(o.leakPtr()) { }
    template<typename U> PassOwnArrayPtr(const PassOwnArrayPtr<U>& o) : m_ptr(o.leakPtr()) { }

    ~PassOwnArrayPtr() { deleteOwnedArrayPtr(m_ptr); }

    PtrType get() const { return m_ptr; }

    void clear();
    PtrType leakPtr() const WARN_UNUSED_RETURN;

    T& operator*() const { ASSERT(m_ptr); return *m_ptr; }
    PtrType operator->() const { ASSERT(m_ptr); return m_ptr; }

    bool operator!() const { return !m_ptr; }

    // This conversion operator allows implicit conversion to bool but not to other integer types.
#if COMPILER(WINSCW)
    operator bool() const { return m_ptr; }
#else
    typedef PtrType PassOwnArrayPtr::*UnspecifiedBoolType;
    operator UnspecifiedBoolType() const { return m_ptr ? &PassOwnArrayPtr::m_ptr : 0; }
#endif

    PassOwnArrayPtr& operator=(const PassOwnArrayPtr<T>&);
#if !defined(LOOSE_PASS_OWN_ARRAY_PTR) || !HAVE(NULLPTR)
    PassOwnArrayPtr& operator=(std::nullptr_t) { clear(); return *this; }
#endif
    template<typename U> PassOwnArrayPtr& operator=(const PassOwnArrayPtr<U>&);

    template<typename U> friend PassOwnArrayPtr<U> adoptArrayPtr(U*);

#ifdef LOOSE_PASS_OWN_ARRAY_PTR
    PassOwnArrayPtr(PtrType ptr) : m_ptr(ptr) { }
    PassOwnArrayPtr& operator=(PtrType);
#endif

private:
#ifndef LOOSE_PASS_OWN_ARRAY_PTR
    explicit PassOwnArrayPtr(PtrType ptr) : m_ptr(ptr) { }
#endif

    mutable PtrType m_ptr;
};

template<typename T> inline void PassOwnArrayPtr<T>::clear()
{
    PtrType ptr = m_ptr;
    m_ptr = 0;
    deleteOwnedArrayPtr(ptr);
}

template<typename T> inline typename PassOwnArrayPtr<T>::PtrType PassOwnArrayPtr<T>::leakPtr() const
{
    PtrType ptr = m_ptr;
    m_ptr = 0;
    return ptr;
}

#ifdef LOOSE_PASS_OWN_ARRAY_PTR
template<typename T> inline PassOwnArrayPtr<T>& PassOwnArrayPtr<T>::operator=(PtrType optr)
{
    PtrType ptr = m_ptr;
    m_ptr = optr;
    ASSERT(!ptr || m_ptr != ptr);
    if (ptr)
        deleteOwnedArrayPtr(ptr);
    return *this;
}
#endif

template<typename T> inline PassOwnArrayPtr<T>& PassOwnArrayPtr<T>::operator=(const PassOwnArrayPtr<T>& optr)
{
    PtrType ptr = m_ptr;
    m_ptr = optr.leakPtr();
    ASSERT(!ptr || m_ptr != ptr);
    if (ptr)
        deleteOwnedArrayPtr(ptr);
    return *this;
}

template<typename T> template<typename U> inline PassOwnArrayPtr<T>& PassOwnArrayPtr<T>::operator=(const PassOwnArrayPtr<U>& optr)
{
    PtrType ptr = m_ptr;
    m_ptr = optr.leakPtr();
    ASSERT(!ptr || m_ptr != ptr);
    if (ptr)
        deleteOwnedArrayPtr(ptr);
    return *this;
}

template<typename T, typename U> inline bool operator==(const PassOwnArrayPtr<T>& a, const PassOwnArrayPtr<U>& b) 
{
    return a.get() == b.get(); 
}

template<typename T, typename U> inline bool operator==(const PassOwnArrayPtr<T>& a, const OwnArrayPtr<U>& b) 
{
    return a.get() == b.get(); 
}

template<typename T, typename U> inline bool operator==(const OwnArrayPtr<T>& a, const PassOwnArrayPtr<U>& b) 
{
    return a.get() == b.get(); 
}

template<typename T, typename U> inline bool operator==(const PassOwnArrayPtr<T>& a, U* b) 
{
    return a.get() == b; 
}

template<typename T, typename U> inline bool operator==(T* a, const PassOwnArrayPtr<U>& b) 
{
    return a == b.get(); 
}

template<typename T, typename U> inline bool operator!=(const PassOwnArrayPtr<T>& a, const PassOwnArrayPtr<U>& b) 
{
    return a.get() != b.get(); 
}

template<typename T, typename U> inline bool operator!=(const PassOwnArrayPtr<T>& a, const OwnArrayPtr<U>& b) 
{
    return a.get() != b.get(); 
}

template<typename T, typename U> inline bool operator!=(const OwnArrayPtr<T>& a, const PassOwnArrayPtr<U>& b) 
{
    return a.get() != b.get(); 
}

template<typename T, typename U> inline bool operator!=(const PassOwnArrayPtr<T>& a, U* b)
{
    return a.get() != b; 
}

template<typename T, typename U> inline bool operator!=(T* a, const PassOwnArrayPtr<U>& b) 
{
    return a != b.get(); 
}

template<typename T> inline PassOwnArrayPtr<T> adoptArrayPtr(T* ptr)
{
    return PassOwnArrayPtr<T>(ptr);
}

template<typename T> inline void deleteOwnedArrayPtr(T* ptr)
{
    typedef char known[sizeof(T) ? 1 : -1];
    if (sizeof(known))
        delete [] ptr;
}

template<typename T, typename U> inline PassOwnArrayPtr<T> static_pointer_cast(const PassOwnArrayPtr<U>& p) 
{
    return adoptArrayPtr(static_cast<T*>(p.leakPtr()));
}

template<typename T, typename U> inline PassOwnArrayPtr<T> const_pointer_cast(const PassOwnArrayPtr<U>& p) 
{
    return adoptArrayPtr(const_cast<T*>(p.leakPtr()));
}

template<typename T> inline T* getPtr(const PassOwnArrayPtr<T>& p)
{
    return p.get();
}

} // namespace WTF

using WTF::PassOwnArrayPtr;
using WTF::adoptArrayPtr;
using WTF::const_pointer_cast;
using WTF::static_pointer_cast;

#endif // WTF_PassOwnArrayPtr_h
