/*
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
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

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)

#include "DisplayRefreshMonitor.h"

#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>

namespace WebCore {

DisplayAnimationClient::DisplayAnimationClient(DisplayRefreshMonitor *monitor)
    : m_monitor(monitor)
{
}

void DisplayAnimationClient::animationFrameChanged()
{
    m_monitor->displayLinkFired();
}

DisplayRefreshMonitor::~DisplayRefreshMonitor()
{
    stopAnimationClient();
    cancelCallOnMainThread(DisplayRefreshMonitor::handleDisplayRefreshedNotificationOnMainThread, this);
}

void DisplayRefreshMonitor::startAnimationClient()
{
    if (m_animationClient)
        return;

    m_animationClient = new DisplayAnimationClient(this);
    BlackBerry::Platform::AnimationFrameRateController::instance()->addClient(m_animationClient);
}

void DisplayRefreshMonitor::stopAnimationClient()
{
    if (!m_animationClient)
        return;

    BlackBerry::Platform::AnimationFrameRateController::instance()->removeClient(m_animationClient);
    delete m_animationClient;
    m_animationClient = 0;
}

bool DisplayRefreshMonitor::requestRefreshCallback()
{
    MutexLocker lock(m_mutex);

    startAnimationClient();

    m_scheduled = true;
    return true;
}

void DisplayRefreshMonitor::displayLinkFired()
{
    if (!m_mutex.tryLock())
        return;

    if (!m_previousFrameDone) {
        m_mutex.unlock();
        return;
    }

    m_previousFrameDone = false;

    m_monotonicAnimationStartTime = monotonicallyIncreasingTime();

    callOnMainThread(handleDisplayRefreshedNotificationOnMainThread, this);
    m_mutex.unlock();
}

}

#endif // USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
