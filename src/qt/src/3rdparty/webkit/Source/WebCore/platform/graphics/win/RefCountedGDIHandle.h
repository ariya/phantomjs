/*
 * Copyright (C) 2006, 2007, 2008, 2010 Apple Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef RefCountedGDIHandle_h
#define RefCountedGDIHandle_h

#include <windows.h>
#include <wtf/HashFunctions.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

template <typename T> class RefCountedGDIHandle : public RefCounted<RefCountedGDIHandle<T> > {
public:
    static PassRefPtr<RefCountedGDIHandle> create(T handle)
    {
        return adoptRef(new RefCountedGDIHandle<T>(handle));
    }

    static PassRefPtr<RefCountedGDIHandle<T> > createDeleted()
    {
        return adoptRef(new RefCountedGDIHandle<T>(reinterpret_cast<T>(-1)));
    }

    ~RefCountedGDIHandle()
    {
        if (m_handle != reinterpret_cast<T>(-1))
            WTF::deleteOwnedPtr(m_handle);
    }

    T handle() const
    {
        return m_handle;
    }

    unsigned hash() const
    {
        return WTF::PtrHash<T>::hash(m_handle);
    }

private:
    RefCountedGDIHandle(T handle)
        : m_handle(handle)
    {
    }

    T m_handle;
};

} // namespace WebCore

#endif // RefCountedGDIHandle_h
