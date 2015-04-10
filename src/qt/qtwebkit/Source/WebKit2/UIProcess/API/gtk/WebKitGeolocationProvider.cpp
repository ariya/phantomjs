/*
 * Copyright (C) 2012 Igalia S.L.
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
#include "WebKitGeolocationProvider.h"

#include "WebGeolocationManagerProxy.h"
#include "WebGeolocationPosition.h"

using namespace WebKit;

#if ENABLE(GEOLOCATION)

static inline WebKitGeolocationProvider* toGeolocationProvider(const void* clientInfo)
{
    return static_cast<WebKitGeolocationProvider*>(const_cast<void*>(clientInfo));
}

static void startUpdatingCallback(WKGeolocationManagerRef geolocationManager, const void* clientInfo)
{
    toGeolocationProvider(clientInfo)->startUpdating();
}

static void stopUpdatingCallback(WKGeolocationManagerRef geolocationManager, const void* clientInfo)
{
    toGeolocationProvider(clientInfo)->stopUpdating();
}

WebKitGeolocationProvider::~WebKitGeolocationProvider()
{
    m_provider.stopUpdating();
}

PassRefPtr<WebKitGeolocationProvider> WebKitGeolocationProvider::create(WebGeolocationManagerProxy* geolocationManager)
{
    return adoptRef(new WebKitGeolocationProvider(geolocationManager));
}

WebKitGeolocationProvider::WebKitGeolocationProvider(WebGeolocationManagerProxy* geolocationManager)
    : m_geolocationManager(geolocationManager)
    , m_provider(this)
{
    ASSERT(geolocationManager);

    WKGeolocationProvider wkGeolocationProvider = {
        kWKGeolocationProviderCurrentVersion,
        this, // clientInfo
        startUpdatingCallback,
        stopUpdatingCallback
    };
    WKGeolocationManagerSetProvider(toAPI(geolocationManager), &wkGeolocationProvider);
}

void WebKitGeolocationProvider::startUpdating()
{
    m_provider.startUpdating();
}

void WebKitGeolocationProvider::stopUpdating()
{
    m_provider.stopUpdating();
}

void WebKitGeolocationProvider::notifyPositionChanged(int timestamp, double latitude, double longitude, double altitude, double accuracy, double altitudeAccuracy)
{
    RefPtr<WebGeolocationPosition> position = WebGeolocationPosition::create(timestamp, latitude, longitude, accuracy, true, altitude, true, altitudeAccuracy, false, 0, false, 0);
    m_geolocationManager->providerDidChangePosition(position.get());
}

void WebKitGeolocationProvider::notifyErrorOccurred(const char* message)
{
    m_geolocationManager->providerDidFailToDeterminePosition();
}

#endif // ENABLE(GEOLOCATION)
