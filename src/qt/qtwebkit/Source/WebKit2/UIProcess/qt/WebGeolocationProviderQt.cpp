/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "WebGeolocationProviderQt.h"

#if ENABLE(GEOLOCATION) && HAVE(QTPOSITIONING)
#include <QtPositioning/QGeoPositionInfoSource>

namespace WebKit {

static inline const WebGeolocationProviderQt* toLocationProvider(const void* clientInfo)
{
    return static_cast<const WebGeolocationProviderQt*>(clientInfo);
}

static void locationStartUpdating(WKGeolocationManagerRef geolocationManager, const void* clientInfo)
{
    toLocationProvider(clientInfo)->startUpdating();
}

static void locationStopUpdating(WKGeolocationManagerRef geolocationManager, const void* clientInfo)
{
    toLocationProvider(clientInfo)->stopUpdating();
}

WebGeolocationProviderQt* WebGeolocationProviderQt::create(WKGeolocationManagerRef manager)
{
    return new WebGeolocationProviderQt(manager);
}

WKGeolocationProvider* WebGeolocationProviderQt::provider(const WebGeolocationProviderQt* location)
{
    static WKGeolocationProvider provider = {
        0, // This features the version.
        location, // This points to the object implementer.
        locationStartUpdating, // The callbacks are next.
        locationStopUpdating
    };

    return &provider;
}

WebGeolocationProviderQt::WebGeolocationProviderQt(WKGeolocationManagerRef manager)
    : m_manager(manager)
    , m_source(0)
{
}

WebGeolocationProviderQt::~WebGeolocationProviderQt()
{
}

void WebGeolocationProviderQt::updateTimeout()
{
    WKGeolocationManagerProviderDidFailToDeterminePosition(m_manager);
}

void WebGeolocationProviderQt::positionUpdated(const QGeoPositionInfo& geoPosition)
{
    if (!geoPosition.isValid())
        return;

    QGeoCoordinate coord = geoPosition.coordinate();
    double latitude = coord.latitude();
    double longitude = coord.longitude();
    double accuracy = geoPosition.attribute(QGeoPositionInfo::HorizontalAccuracy);
    double timeStampInSeconds = geoPosition.timestamp().toMSecsSinceEpoch() / 1000;

    m_lastPosition.adopt(WKGeolocationPositionCreate(timeStampInSeconds, latitude, longitude, accuracy));

    WKGeolocationManagerProviderDidChangePosition(m_manager, m_lastPosition.get());
}

void WebGeolocationProviderQt::startUpdating() const
{
    if (!m_source) {
        if (!(m_source = QGeoPositionInfoSource::createDefaultSource(const_cast<WebGeolocationProviderQt*>(this)))) {
            // Let the manager known that the provider is not available.
            WKGeolocationManagerSetProvider(m_manager, 0);
            // Notify failure at retrieving the position.
            WKGeolocationManagerProviderDidFailToDeterminePosition(m_manager);
            return;
        }

        connect(m_source, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(positionUpdated(QGeoPositionInfo)));
        connect(m_source, SIGNAL(updateTimeout()), this, SLOT(updateTimeout()));
    }

    m_source->startUpdates();
}

void WebGeolocationProviderQt::stopUpdating() const
{
    if (m_source)
        m_source->stopUpdates();
}

} // namespace WebKit

#include "moc_WebGeolocationProviderQt.cpp"

#endif // ENABLE(GEOLOCATION)
