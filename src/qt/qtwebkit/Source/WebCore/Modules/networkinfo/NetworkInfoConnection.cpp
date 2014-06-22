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
#include "NetworkInfoConnection.h"

#if ENABLE(NETWORK_INFO)
#include "Document.h"
#include "Event.h"
#include "Frame.h"
#include "NetworkInfoClient.h"

namespace WebCore {

PassRefPtr<NetworkInfoConnection> NetworkInfoConnection::create(Navigator* navigator)
{
    RefPtr<NetworkInfoConnection> networkInfoConnection(adoptRef(new NetworkInfoConnection(navigator)));
    networkInfoConnection->suspendIfNeeded();
    return networkInfoConnection.release();
}

NetworkInfoConnection::NetworkInfoConnection(Navigator* navigator)
    : ActiveDOMObject(navigator->frame()->document())
    , m_controller(NetworkInfoController::from(navigator->frame()->page()))
    , m_networkInfo(0)
{
    m_controller->addListener(this);
}

NetworkInfoConnection::~NetworkInfoConnection()
{
}

double NetworkInfoConnection::bandwidth() const
{
    return m_controller->client()->bandwidth();
}

bool NetworkInfoConnection::metered() const
{
    return m_controller->client()->metered();
}

void NetworkInfoConnection::didChangeNetworkInformation(PassRefPtr<Event> event, PassRefPtr<NetworkInfo> networkInfo)
{
    m_networkInfo = networkInfo;
    dispatchEvent(event);
}

EventTargetData* NetworkInfoConnection::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* NetworkInfoConnection::ensureEventTargetData()
{
    return &m_eventTargetData;
}

const AtomicString& NetworkInfoConnection::interfaceName() const
{
    return eventNames().interfaceForNetworkInfoConnection;
}

void NetworkInfoConnection::suspend(ReasonForSuspension)
{
    if (m_controller)
        m_controller->removeListener(this);
}

void NetworkInfoConnection::resume()
{
    if (m_controller)
        m_controller->addListener(this);
}

void NetworkInfoConnection::stop()
{
    if (m_controller)
        m_controller->removeListener(this);
}

} // namespace WebCore
#endif
