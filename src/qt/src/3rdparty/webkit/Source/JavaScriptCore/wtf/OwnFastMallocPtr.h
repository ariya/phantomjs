/*
 *  Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 *  Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef OwnFastMallocPtr_h
#define OwnFastMallocPtr_h

#include "FastMalloc.h"

namespace WTF {

    template<class T> class OwnFastMallocPtr {
        WTF_MAKE_NONCOPYABLE(OwnFastMallocPtr);
    public:
        explicit OwnFastMallocPtr(T* ptr) : m_ptr(ptr)
        {
        }

        ~OwnFastMallocPtr()
        {
            fastFree(const_cast<void*>(static_cast<const void*>(const_cast<const T*>(m_ptr))));
        }

        T* get() const { return m_ptr; }
        T* release() { T* ptr = m_ptr; m_ptr = 0; return ptr; }

    private:
        T* m_ptr;
    };

} // namespace WTF

using WTF::OwnFastMallocPtr;

#endif // OwnFastMallocPtr_h
