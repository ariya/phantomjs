/*
 *  Copyright (C) 2013 Collabora Ltd.
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
 */

#ifndef GMutexLocker_h
#define GMutexLocker_h

#if USE(GLIB)

#include <glib.h>

#include <wtf/FastAllocBase.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

class GMutexLocker {
    WTF_MAKE_NONCOPYABLE(GMutexLocker); WTF_MAKE_FAST_ALLOCATED;

public:
    inline explicit GMutexLocker(GMutex* mutex)
        : m_mutex(mutex)
        , m_val(0)
    {
        lock();
    }

    inline ~GMutexLocker() { unlock(); }

    inline void lock()
    {
        if (m_mutex && !m_val) {
            g_mutex_lock(m_mutex);
            m_val = 1;
        }
    }

    inline void unlock()
    {
        if (m_mutex && m_val) {
            m_val = 0;
            g_mutex_unlock(m_mutex);
        }
    }

    inline GMutex* mutex() const { return m_mutex; }

private:
    GMutex* m_mutex;
    uint8_t m_val;
};

}

#endif // USE(GLIB)

#endif // GMutexLocker_h
