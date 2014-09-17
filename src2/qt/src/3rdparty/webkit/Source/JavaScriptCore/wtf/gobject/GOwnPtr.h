/*
 *  Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 *  Copyright (C) 2008 Collabora Ltd.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef GOwnPtr_h
#define GOwnPtr_h

#if ENABLE(GLIB_SUPPORT)

#include <algorithm>
#include <wtf/Assertions.h>
#include <wtf/Noncopyable.h>

extern "C" void g_free(void*);

namespace WTF {

template <typename T> inline void freeOwnedGPtr(T* ptr);
template<> void freeOwnedGPtr<GError>(GError*);
template<> void freeOwnedGPtr<GList>(GList*);
template<> void freeOwnedGPtr<GPatternSpec>(GPatternSpec*);
template<> void freeOwnedGPtr<GDir>(GDir*);

template <typename T> class GOwnPtr {
    WTF_MAKE_NONCOPYABLE(GOwnPtr);
public:
    explicit GOwnPtr(T* ptr = 0) : m_ptr(ptr) { }
    ~GOwnPtr() { freeOwnedGPtr(m_ptr); }

    T* get() const { return m_ptr; }
    T* release()
    {
        T* ptr = m_ptr;
        m_ptr = 0;
        return ptr;
    }

    T*& outPtr()
    {
        ASSERT(!m_ptr);
        return m_ptr;
    }

    void set(T* ptr)
    {
        ASSERT(!ptr || m_ptr != ptr);
        freeOwnedGPtr(m_ptr);
        m_ptr = ptr;
    }

    void clear()
    {
        T* ptr = m_ptr;
        m_ptr = 0;
        freeOwnedGPtr(ptr);
    }

    T& operator*() const
    {
        ASSERT(m_ptr);
        return *m_ptr;
    }

    T* operator->() const
    {
        ASSERT(m_ptr);
        return m_ptr;
    }

    bool operator!() const { return !m_ptr; }

    // This conversion operator allows implicit conversion to bool but not to other integer types.
    typedef T* GOwnPtr::*UnspecifiedBoolType;
    operator UnspecifiedBoolType() const { return m_ptr ? &GOwnPtr::m_ptr : 0; }

    void swap(GOwnPtr& o) { std::swap(m_ptr, o.m_ptr); }

private:
    T* m_ptr;
};

template <typename T> inline void swap(GOwnPtr<T>& a, GOwnPtr<T>& b)
{
    a.swap(b);
}

template <typename T, typename U> inline bool operator==(const GOwnPtr<T>& a, U* b)
{ 
    return a.get() == b; 
}

template <typename T, typename U> inline bool operator==(T* a, const GOwnPtr<U>& b) 
{
    return a == b.get(); 
}

template <typename T, typename U> inline bool operator!=(const GOwnPtr<T>& a, U* b)
{
    return a.get() != b; 
}

template <typename T, typename U> inline bool operator!=(T* a, const GOwnPtr<U>& b)
{ 
    return a != b.get(); 
}

template <typename T> inline typename GOwnPtr<T>::PtrType getPtr(const GOwnPtr<T>& p)
{
    return p.get();
}

template <typename T> inline void freeOwnedGPtr(T* ptr)
{
    g_free(ptr);
}

} // namespace WTF

using WTF::GOwnPtr;

#endif // ENABLE(GLIB_SUPPORT)

#endif // GOwnPtr_h

