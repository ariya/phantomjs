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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)

#include "DisplayRefreshMonitor.h"

#include <wtf/CurrentTime.h>

namespace WebCore {

DisplayRefreshMonitorClient::DisplayRefreshMonitorClient()
    : m_scheduled(false)
    , m_displayIDIsSet(false)
{
}

DisplayRefreshMonitorClient::~DisplayRefreshMonitorClient()
{
    DisplayRefreshMonitorManager::sharedManager()->unregisterClient(this);
}

void DisplayRefreshMonitorClient::fireDisplayRefreshIfNeeded(double timestamp)
{
    if (m_scheduled) {
        m_scheduled = false;
        displayRefreshFired(timestamp);
    }
}

DisplayRefreshMonitor::DisplayRefreshMonitor(PlatformDisplayID displayID)
    : m_monotonicAnimationStartTime(0)
    , m_active(true)
    , m_scheduled(false)
    , m_previousFrameDone(true)
    , m_unscheduledFireCount(0)
    , m_displayID(displayID)
#if PLATFORM(MAC)
    , m_displayLink(0)
#endif
#if PLATFORM(BLACKBERRY)
    , m_animationClient(0)
#endif
{
}

void DisplayRefreshMonitor::handleDisplayRefreshedNotificationOnMainThread(void* data)
{
    DisplayRefreshMonitor* monitor = static_cast<DisplayRefreshMonitor*>(data);
    monitor->displayDidRefresh();
}

void DisplayRefreshMonitor::addClient(DisplayRefreshMonitorClient* client)
{
    m_clients.add(client);
}

bool DisplayRefreshMonitor::removeClient(DisplayRefreshMonitorClient* client)
{
    DisplayRefreshMonitorClientSet::iterator it = m_clients.find(client);
    if (it != m_clients.end()) {
        m_clients.remove(it);
        return true;
    }
    return false;
}

void DisplayRefreshMonitor::displayDidRefresh()
{
    double monotonicAnimationStartTime;
    {
        MutexLocker lock(m_mutex);
         if (!m_scheduled)
            ++m_unscheduledFireCount;
        else
            m_unscheduledFireCount = 0;

        m_scheduled = false;
        monotonicAnimationStartTime = m_monotonicAnimationStartTime;
    }

    // The call back can cause all our clients to be unregistered, so we need to protect
    // against deletion until the end of the method.
    RefPtr<DisplayRefreshMonitor> protector(this);
    
    Vector<DisplayRefreshMonitorClient*> clients;
    copyToVector(m_clients, clients);
    for (size_t i = 0; i < clients.size(); ++i)
        clients[i]->fireDisplayRefreshIfNeeded(monotonicAnimationStartTime);

    {
        MutexLocker lock(m_mutex);
        m_previousFrameDone = true;
    }
    
    DisplayRefreshMonitorManager::sharedManager()->displayDidRefresh(this);
}

DisplayRefreshMonitorManager* DisplayRefreshMonitorManager::sharedManager()
{
    DEFINE_STATIC_LOCAL(DisplayRefreshMonitorManager, manager, ());
    return &manager;
}

DisplayRefreshMonitor* DisplayRefreshMonitorManager::ensureMonitorForClient(DisplayRefreshMonitorClient* client)
{
    DisplayRefreshMonitorMap::iterator it = m_monitors.find(client->m_displayID);
    if (it == m_monitors.end()) {
        RefPtr<DisplayRefreshMonitor> monitor = DisplayRefreshMonitor::create(client->m_displayID);
        monitor->addClient(client);
        DisplayRefreshMonitor* result = monitor.get();
        m_monitors.add(client->m_displayID, monitor.release());
        return result;
    }
    it->value->addClient(client);
    return it->value.get();
}

void DisplayRefreshMonitorManager::registerClient(DisplayRefreshMonitorClient* client)
{
    if (!client->m_displayIDIsSet)
        return;
        
    ensureMonitorForClient(client);
}

void DisplayRefreshMonitorManager::unregisterClient(DisplayRefreshMonitorClient* client)
{
    if (!client->m_displayIDIsSet)
        return;

    DisplayRefreshMonitorMap::iterator it = m_monitors.find(client->m_displayID);
    if (it == m_monitors.end())
        return;
    
    DisplayRefreshMonitor* monitor = it->value.get();
    if (monitor->removeClient(client)) {
        if (!monitor->hasClients())
            m_monitors.remove(it);
    }
}

bool DisplayRefreshMonitorManager::scheduleAnimation(DisplayRefreshMonitorClient* client)
{
    if (!client->m_displayIDIsSet)
        return false;
        
    DisplayRefreshMonitor* monitor = ensureMonitorForClient(client);

    client->m_scheduled = true;
    return monitor->requestRefreshCallback();
}

void DisplayRefreshMonitorManager::displayDidRefresh(DisplayRefreshMonitor* monitor)
{
    if (monitor->shouldBeTerminated()) {
        DisplayRefreshMonitorMap::iterator it = m_monitors.find(monitor->displayID());
        ASSERT(it != m_monitors.end());
        m_monitors.remove(it);
    }
}

void DisplayRefreshMonitorManager::windowScreenDidChange(PlatformDisplayID displayID, DisplayRefreshMonitorClient* client)
{
    if (client->m_displayIDIsSet && client->m_displayID == displayID)
        return;
    
    unregisterClient(client);
    client->setDisplayID(displayID);
    registerClient(client);
    if (client->m_scheduled)
        scheduleAnimation(client);
}

}

#endif // USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
