/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "GeolocationProviderMock.h"

#include <WebKit2/WKGeolocationManager.h>
#include <wtf/Assertions.h>
#include <wtf/CurrentTime.h>

namespace WTR {

static void startUpdatingCallback(WKGeolocationManagerRef geolocationManager, const void* clientInfo)
{
    GeolocationProviderMock* geolocationProvider = static_cast<GeolocationProviderMock*>(const_cast<void*>(clientInfo));
    geolocationProvider->startUpdating(geolocationManager);
}

static void stopUpdatingCallback(WKGeolocationManagerRef geolocationManager, const void* clientInfo)
{
    GeolocationProviderMock* geolocationProvider = static_cast<GeolocationProviderMock*>(const_cast<void*>(clientInfo));
    geolocationProvider->stopUpdating(geolocationManager);
}

GeolocationProviderMock::GeolocationProviderMock(WKContextRef context)
    : m_isActive(false)
    , m_hasError(false)
{
    m_geolocationManager = WKContextGetGeolocationManager(context);

    WKGeolocationProvider providerCallback = {
        kWKGeolocationProviderCurrentVersion,
        this,
        startUpdatingCallback,
        stopUpdatingCallback };
    WKGeolocationManagerSetProvider(m_geolocationManager, &providerCallback);
}

GeolocationProviderMock::~GeolocationProviderMock()
{
    WKGeolocationManagerSetProvider(m_geolocationManager, 0);
}

void GeolocationProviderMock::setPosition(double latitude, double longitude, double accuracy, bool providesAltitude, double altitude, bool providesAltitudeAccuracy, double altitudeAccuracy, bool providesHeading, double heading, bool providesSpeed, double speed)
{
    m_position.adopt(WKGeolocationPositionCreate_b(currentTime(), latitude, longitude, accuracy, providesAltitude, altitude, providesAltitudeAccuracy, altitudeAccuracy, providesHeading, heading, providesSpeed, speed));

    m_hasError = false;
    m_errorMessage.clear();

    sendPositionIfNeeded();
}

void GeolocationProviderMock::setPositionUnavailableError(WKStringRef errorMessage)
{
    m_errorMessage = errorMessage;
    m_hasError = true;

    m_position.clear();

    sendErrorIfNeeded();
}

void GeolocationProviderMock::startUpdating(WKGeolocationManagerRef geolocationManager)
{
    ASSERT_UNUSED(geolocationManager, geolocationManager == m_geolocationManager);

    m_isActive = true;
    sendPositionIfNeeded();
    sendErrorIfNeeded();
}

void GeolocationProviderMock::stopUpdating(WKGeolocationManagerRef geolocationManager)
{
    ASSERT_UNUSED(geolocationManager, geolocationManager == m_geolocationManager);

    m_isActive = false;
}

void GeolocationProviderMock::sendPositionIfNeeded()
{
    if (m_isActive && m_position)
        WKGeolocationManagerProviderDidChangePosition(m_geolocationManager, m_position.get());
}

void GeolocationProviderMock::sendErrorIfNeeded()
{
    if (m_isActive && m_hasError)
        WKGeolocationManagerProviderDidFailToDeterminePositionWithErrorMessage(m_geolocationManager, m_errorMessage.get());
}

} // namespace WTR
