/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "SharedTimer.h"

#include <BlackBerryPlatformSingleton.h>
#include <BlackBerryPlatformTimer.h>
#include <wtf/CurrentTime.h>

namespace WebCore {

class SharedTimerBlackBerry : public BlackBerry::Platform::ThreadUnsafeSingleton<SharedTimerBlackBerry> {
    friend void setSharedTimerFiredFunction(void (*f)());
    SINGLETON_DEFINITION_THREADUNSAFE(SharedTimerBlackBerry)

public:
    void start(double);
    void stop();

private:
    SharedTimerBlackBerry();
    ~SharedTimerBlackBerry();

    bool m_started;
    void (*m_timerFunction)();
};

SINGLETON_INITIALIZER_THREADUNSAFE(SharedTimerBlackBerry)

SharedTimerBlackBerry::SharedTimerBlackBerry()
    : m_started(false)
    , m_timerFunction(0)
{
}

SharedTimerBlackBerry::~SharedTimerBlackBerry()
{
    if (m_started)
        stop();
}

void SharedTimerBlackBerry::start(double interval)
{
    BlackBerry::Platform::timerStart(interval, m_timerFunction);
    m_started = true;
}

void SharedTimerBlackBerry::stop()
{
    if (!m_started)
        return;
    BlackBerry::Platform::timerStop();
    m_started = false;
}

void setSharedTimerFiredFunction(void (*f)())
{
    SharedTimerBlackBerry::instance()->m_timerFunction = f;
}

void setSharedTimerFireInterval(double interval)
{
    SharedTimerBlackBerry::instance()->start(interval);
}

void stopSharedTimer()
{
    SharedTimerBlackBerry::instance()->stop();
}

} // namespace WebCore

