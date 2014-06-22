/*
 * Copyright (C) 2012 Samsung Electronics. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "NetworkInfoController.h"

#if ENABLE(NETWORK_INFO)
#include "Event.h"
#include "NetworkInfo.h"
#include "NetworkInfoClient.h"

namespace WebCore {

NetworkInfoController::NetworkInfoController(Page* page, NetworkInfoClient* client)
    : m_page(page)
    , m_client(client)
{
}

NetworkInfoController::~NetworkInfoController()
{
    for (NetworkInfoListenerList::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
        (*it)->networkInfoControllerDestroyed();

    m_client->networkInfoControllerDestroyed();
}

PassOwnPtr<NetworkInfoController> NetworkInfoController::create(Page* page, NetworkInfoClient* client)
{
    return adoptPtr(new NetworkInfoController(page, client));
}

void NetworkInfoController::addListener(NetworkInfoConnection* networkInfoConnection)
{
     m_listeners.append(networkInfoConnection);
     m_client->startUpdating();
}

void NetworkInfoController::removeListener(NetworkInfoConnection* networkInfoConnection)
{
     size_t position = m_listeners.find(networkInfoConnection);
     if (position == WTF::notFound)
         return;
     m_listeners.remove(position);
     if (m_listeners.isEmpty())
         m_client->stopUpdating();
}

void NetworkInfoController::didChangeNetworkInformation(const AtomicString& eventType, PassRefPtr<NetworkInfo> networkInfo)
{
     RefPtr<Event> event = Event::create(eventType, false, false);
     RefPtr<NetworkInfo> networkInformation = networkInfo;
     for (NetworkInfoListenerList::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
         (*it)->didChangeNetworkInformation(event, networkInformation);
}

const char* NetworkInfoController::supplementName()
{
    return "NetworkInfoController";
}

void provideNetworkInfoTo(Page* page, NetworkInfoClient* client)
{
    NetworkInfoController::provideTo(page, NetworkInfoController::supplementName(), NetworkInfoController::create(page, client));
}

} // namespace WebCore

#endif
