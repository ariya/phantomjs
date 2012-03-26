/*
 *  Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 2008 Collabora Ltd.
 *  Copyright (C) 2009 Martin Robinson
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

#ifndef WTF_GRefPtr_h
#define WTF_GRefPtr_h

#if ENABLE(GLIB_SUPPORT)

#include "AlwaysInline.h"
#include "GRefPtr.h"
#include "RefPtr.h"
#include <algorithm>

extern "C" void g_object_unref(gpointer);
extern "C" gpointer g_object_ref_sink(gpointer);

namespace WTF {

enum GRefPtrAdoptType { GRefPtrAdopt };
template <typename T> inline T* refGPtr(T*);
template <typename T> inline void derefGPtr(T*);
template <typename T> class GRefPtr;
template <typename T> GRefPtr<T> adoptGRef(T*);

template <typename T> class GRefPtr {
public:
    GRefPtr() : m_ptr(0) { }

    GRefPtr(T* ptr)
        : m_ptr(ptr)
    {
        if (ptr)
            refGPtr(ptr);
    }

    GRefPtr(const GRefPtr& o)
        : m_ptr(o.m_ptr)
    {
        if (T* ptr = m_ptr)
            refGPtr(ptr);
    }

    template <typename U> GRefPtr(const GRefPtr<U>& o)
        : m_ptr(o.get())
    {
        if (T* ptr = m_ptr)
            refGPtr(ptr);
    }

    ~GRefPtr()
    {
        if (T* ptr = m_ptr)
            derefGPtr(ptr);
    }

    void clear()
    {
        T* ptr = m_ptr;
        m_ptr = 0;
        if (ptr)
            derefGPtr(ptr);
    }

    T* leakRef() WARN_UNUSED_RETURN
    {
        T* ptr = m_ptr;
        m_ptr = 0;
        return ptr;
    }

    // Hash table deleted values, which are only constructed and never copied or destroyed.
    GRefPtr(HashTableDeletedValueType) : m_ptr(hashTableDeletedValue()) { }
    bool isHashTableDeletedValue() const { return m_ptr == hashTableDeletedValue(); }

    T* get() const { return m_ptr; }
    T& operator*() const { return *m_ptr; }
    ALWAYS_INLINE T* operator->() const { return m_ptr; }

    bool operator!() const { return !m_ptr; }

    // This conversion operator allows implicit conversion to bool but not to other integer types.
    typedef T* GRefPtr::*UnspecifiedBoolType;
    operator UnspecifiedBoolType() const { return m_ptr ? &GRefPtr::m_ptr : 0; }

    GRefPtr& operator=(const GRefPtr&);
    GRefPtr& operator=(T*);
    template <typename U> GRefPtr& operator=(const GRefPtr<U>&);

    void swap(GRefPtr&);
    friend GRefPtr adoptGRef<T>(T*);

private:
    static T* hashTableDeletedValue() { return reinterpret_cast<T*>(-1); }
    // Adopting constructor.
    GRefPtr(T* ptr, GRefPtrAdoptType) : m_ptr(ptr) {}

    T* m_ptr;
};

template <typename T> inline GRefPtr<T>& GRefPtr<T>::operator=(const GRefPtr<T>& o)
{
    T* optr = o.get();
    if (optr)
        refGPtr(optr);
    T* ptr = m_ptr;
    m_ptr = optr;
    if (ptr)
        derefGPtr(ptr);
    return *this;
}

template <typename T> inline GRefPtr<T>& GRefPtr<T>::operator=(T* optr)
{
    T* ptr = m_ptr;
    if (optr)
        refGPtr(optr);
    m_ptr = optr;
    if (ptr)
        derefGPtr(ptr);
    return *this;
}

template <class T> inline void GRefPtr<T>::swap(GRefPtr<T>& o)
{
    std::swap(m_ptr, o.m_ptr);
}

template <class T> inline void swap(GRefPtr<T>& a, GRefPtr<T>& b)
{
    a.swap(b);
}

template <typename T, typename U> inline bool operator==(const GRefPtr<T>& a, const GRefPtr<U>& b)
{
    return a.get() == b.get();
}

template <typename T, typename U> inline bool operator==(const GRefPtr<T>& a, U* b)
{
    return a.get() == b;
}

template <typename T, typename U> inline bool operator==(T* a, const GRefPtr<U>& b)
{
    return a == b.get();
}

template <typename T, typename U> inline bool operator!=(const GRefPtr<T>& a, const GRefPtr<U>& b)
{
    return a.get() != b.get();
}

template <typename T, typename U> inline bool operator!=(const GRefPtr<T>& a, U* b)
{
    return a.get() != b;
}

template <typename T, typename U> inline bool operator!=(T* a, const GRefPtr<U>& b)
{
    return a != b.get();
}

template <typename T, typename U> inline GRefPtr<T> static_pointer_cast(const GRefPtr<U>& p)
{
    return GRefPtr<T>(static_cast<T*>(p.get()));
}

template <typename T, typename U> inline GRefPtr<T> const_pointer_cast(const GRefPtr<U>& p)
{
    return GRefPtr<T>(const_cast<T*>(p.get()));
}

template <typename T> inline T* getPtr(const GRefPtr<T>& p)
{
    return p.get();
}

template <typename T> GRefPtr<T> adoptGRef(T* p)
{
    return GRefPtr<T>(p, GRefPtrAdopt);
}

template <> GHashTable* refGPtr(GHashTable* ptr);
template <> void derefGPtr(GHashTable* ptr);
template <> GVariant* refGPtr(GVariant* ptr);
template <> void derefGPtr(GVariant* ptr);
template <> GSource* refGPtr(GSource* ptr);
template <> void derefGPtr(GSource* ptr);

template <typename T> inline T* refGPtr(T* ptr)
{
    if (ptr)
        g_object_ref_sink(ptr);
    return ptr;
}

template <typename T> inline void derefGPtr(T* ptr)
{
    if (ptr)
        g_object_unref(ptr);
}

} // namespace WTF

using WTF::GRefPtr;
using WTF::adoptGRef;

#endif // ENABLE(GLIB_SUPPORT)

#endif // WTF_GRefPtr_h
