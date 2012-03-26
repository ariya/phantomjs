/*
 * Copyright 2010, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DeviceOrientationController.h"

#include "DeviceOrientation.h"
#include "DeviceOrientationClient.h"
#include "DeviceOrientationEvent.h"

namespace WebCore {

DeviceOrientationController::DeviceOrientationController(Page* page, DeviceOrientationClient* client)
    : m_page(page)
    , m_client(client)
    , m_timer(this, &DeviceOrientationController::timerFired)
{
    ASSERT(m_client);
    m_client->setController(this);
}

DeviceOrientationController::~DeviceOrientationController()
{
    m_client->deviceOrientationControllerDestroyed();
}

void DeviceOrientationController::timerFired(Timer<DeviceOrientationController>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_timer);
    ASSERT(m_client->lastOrientation());

    RefPtr<DeviceOrientation> orientation = m_client->lastOrientation();
    RefPtr<DeviceOrientationEvent> event = DeviceOrientationEvent::create(eventNames().deviceorientationEvent, orientation.get());

    Vector<RefPtr<DOMWindow> > listenersVector;
    copyToVector(m_newListeners, listenersVector);
    m_newListeners.clear();
    for (size_t i = 0; i < listenersVector.size(); ++i)
        listenersVector[i]->dispatchEvent(event);
}

void DeviceOrientationController::addListener(DOMWindow* window)
{
    // If the client already has an orientation, we should fire an event with that
    // orientation. The event is fired asynchronously, but without
    // waiting for the client to get a new orientation.
    if (m_client->lastOrientation()) {
        m_newListeners.add(window);
        if (!m_timer.isActive())
            m_timer.startOneShot(0);
    }

    // The client must not call back synchronously.
    bool wasEmpty = m_listeners.isEmpty();
    m_listeners.add(window);
    if (wasEmpty)
        m_client->startUpdating();
}

void DeviceOrientationController::removeListener(DOMWindow* window)
{
    m_listeners.remove(window);
    m_newListeners.remove(window);
    if (m_listeners.isEmpty())
        m_client->stopUpdating();
}

void DeviceOrientationController::removeAllListeners(DOMWindow* window)
{
    // May be called with a DOMWindow that's not a listener.
    if (!m_listeners.contains(window))
        return;

    m_listeners.removeAll(window);
    m_newListeners.remove(window);
    if (m_listeners.isEmpty())
        m_client->stopUpdating();
}

void DeviceOrientationController::didChangeDeviceOrientation(DeviceOrientation* orientation)
{
    RefPtr<DeviceOrientationEvent> event = DeviceOrientationEvent::create(eventNames().deviceorientationEvent, orientation);
    Vector<RefPtr<DOMWindow> > listenersVector;
    copyToVector(m_listeners, listenersVector);
    for (size_t i = 0; i < listenersVector.size(); ++i)
        listenersVector[i]->dispatchEvent(event);
}

} // namespace WebCore
