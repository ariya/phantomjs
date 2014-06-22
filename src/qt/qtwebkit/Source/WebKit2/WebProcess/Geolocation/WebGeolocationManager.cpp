/*
 * Copyright (C) 2011, 2012, 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebGeolocationManager.h"

#include "WebGeolocationManagerMessages.h"
#include "WebGeolocationManagerProxyMessages.h"
#include "WebPage.h"
#include "WebProcess.h"
#include <WebCore/Geolocation.h>
#include <WebCore/GeolocationController.h>
#include <WebCore/GeolocationError.h>
#include <WebCore/GeolocationPosition.h>
#include <WebCore/Page.h>

using namespace WebCore;

namespace WebKit {

const char* WebGeolocationManager::supplementName()
{
    return "WebGeolocationManager";
}

WebGeolocationManager::WebGeolocationManager(WebProcess* process)
    : m_process(process)
{
    m_process->addMessageReceiver(Messages::WebGeolocationManager::messageReceiverName(), this);
}

WebGeolocationManager::~WebGeolocationManager()
{
}

void WebGeolocationManager::registerWebPage(WebPage* page)
{
    bool wasEmpty = m_pageSet.isEmpty();

    m_pageSet.add(page);
    
    if (wasEmpty)
        m_process->parentProcessConnection()->send(Messages::WebGeolocationManagerProxy::StartUpdating(), 0);
}

void WebGeolocationManager::unregisterWebPage(WebPage* page)
{
    m_pageSet.remove(page);

    if (m_pageSet.isEmpty())
        m_process->parentProcessConnection()->send(Messages::WebGeolocationManagerProxy::StopUpdating(), 0);
}

void WebGeolocationManager::didChangePosition(const WebGeolocationPosition::Data& data)
{
#if ENABLE(GEOLOCATION)
    RefPtr<GeolocationPosition> position = GeolocationPosition::create(data.timestamp, data.latitude, data.longitude, data.accuracy, data.canProvideAltitude, data.altitude, data.canProvideAltitudeAccuracy, data.altitudeAccuracy, data.canProvideHeading, data.heading, data.canProvideSpeed, data.speed);

    Vector<RefPtr<WebPage> > webPageCopy;
    copyToVector(m_pageSet, webPageCopy);
    for (size_t i = 0; i < webPageCopy.size(); ++i) {
        WebPage* page = webPageCopy[i].get();
        if (page->corePage())
            GeolocationController::from(page->corePage())->positionChanged(position.get());
    }
#else
    UNUSED_PARAM(data);
#endif // ENABLE(GEOLOCATION)
}

void WebGeolocationManager::didFailToDeterminePosition(const String& errorMessage)
{
#if ENABLE(GEOLOCATION)
    // FIXME: Add localized error string.
    RefPtr<GeolocationError> error = GeolocationError::create(GeolocationError::PositionUnavailable, errorMessage);

    Vector<RefPtr<WebPage> > webPageCopy;
    copyToVector(m_pageSet, webPageCopy);
    for (size_t i = 0; i < webPageCopy.size(); ++i) {
        WebPage* page = webPageCopy[i].get();
        if (page->corePage())
            GeolocationController::from(page->corePage())->errorOccurred(error.get());
    }
#else
    UNUSED_PARAM(errorMessage);
#endif // ENABLE(GEOLOCATION)
}

} // namespace WebKit
