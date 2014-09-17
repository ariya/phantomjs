/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "GeolocationClientQt.h"

#include "Geolocation.h"
#include "GeolocationController.h"
#include "GeolocationError.h"
#include "GeolocationPermissionClientQt.h"
#include "GeolocationPosition.h"
#include "Page.h"
#include "qwebframe.h"
#include "qwebframe_p.h"
#include "qwebpage.h"
#include "qwebpage_p.h"

using namespace QtMobility;

namespace WebCore {

static const char failedToStartServiceErrorMessage[] = "Failed to start Geolocation service";

GeolocationClientQt::GeolocationClientQt(const QWebPage* page)
    : m_page(page)
    , m_lastPosition(0)
    , m_location(0)
{
}

GeolocationClientQt::~GeolocationClientQt()
{
    delete m_location;
}

void GeolocationClientQt::geolocationDestroyed()
{
    delete this;
}

void GeolocationClientQt::positionUpdated(const QGeoPositionInfo &geoPosition)
{
    if (!geoPosition.isValid())
        return;

    QGeoCoordinate coord = geoPosition.coordinate();
    double latitude = coord.latitude();
    double longitude = coord.longitude();
    bool providesAltitude = (geoPosition.coordinate().type() == QGeoCoordinate::Coordinate3D);
    double altitude = coord.altitude();

    double accuracy = geoPosition.attribute(QGeoPositionInfo::HorizontalAccuracy);

    bool providesAltitudeAccuracy = geoPosition.hasAttribute(QGeoPositionInfo::VerticalAccuracy);
    double altitudeAccuracy = geoPosition.attribute(QGeoPositionInfo::VerticalAccuracy);

    bool providesHeading =  geoPosition.hasAttribute(QGeoPositionInfo::Direction);
    double heading = geoPosition.attribute(QGeoPositionInfo::Direction);

    bool providesSpeed = geoPosition.hasAttribute(QGeoPositionInfo::GroundSpeed);
    double speed = geoPosition.attribute(QGeoPositionInfo::GroundSpeed);

    double timeStampInSeconds = geoPosition.timestamp().toMSecsSinceEpoch() / 1000;

    m_lastPosition = GeolocationPosition::create(timeStampInSeconds, latitude, longitude,
                                                 accuracy, providesAltitude, altitude,
                                                 providesAltitudeAccuracy, altitudeAccuracy,
                                                 providesHeading, heading, providesSpeed, speed);

    WebCore::Page* page = QWebPagePrivate::core(m_page);
    page->geolocationController()->positionChanged(m_lastPosition.get());
}

void GeolocationClientQt::startUpdating()
{
    if (!m_location && (m_location = QGeoPositionInfoSource::createDefaultSource(this)))
        connect(m_location, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(positionUpdated(QGeoPositionInfo)));

    if (!m_location) {
        WebCore::Page* page = QWebPagePrivate::core(m_page);
        RefPtr<WebCore::GeolocationError> error = GeolocationError::create(GeolocationError::PositionUnavailable, failedToStartServiceErrorMessage);
        page->geolocationController()->errorOccurred(error.get());
        return;
    }

    m_location->startUpdates();
}

void GeolocationClientQt::stopUpdating()
{
    if (m_location)
        m_location->stopUpdates();
}


void GeolocationClientQt::setEnableHighAccuracy(bool)
{
    // qtmobility 1.0 supports only GPS as of now so high accuracy is enabled by default
}

void GeolocationClientQt::requestPermission(Geolocation* geolocation)
{
    ASSERT(geolocation);
    QWebFrame* webFrame = QWebFramePrivate::kit(geolocation->frame());
    GeolocationPermissionClientQt::geolocationPermissionClient()->requestGeolocationPermissionForFrame(webFrame, geolocation);
}

void GeolocationClientQt::cancelPermissionRequest(Geolocation* geolocation)
{
    ASSERT(geolocation);
    QWebFrame* webFrame = QWebFramePrivate::kit(geolocation->frame());
    GeolocationPermissionClientQt::geolocationPermissionClient()->cancelGeolocationPermissionRequestForFrame(webFrame, geolocation);
}

} // namespace WebCore

#include "moc_GeolocationClientQt.cpp"
