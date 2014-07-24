/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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
#include "GeolocationClientBlackBerry.h"

#include "Chrome.h"
#include "Frame.h"
#include "Geolocation.h"
#include "GeolocationController.h"
#include "GeolocationError.h"
#include "GeolocationPosition.h"
#include "Page.h"
#include "SecurityOrigin.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformString.h>

using namespace WebCore;

static String getOrigin(Frame* frame) 
{
    String origin; 
    SecurityOrigin* securityOrigin = frame->document()->securityOrigin();

    // Special case for file.
    if (securityOrigin->protocol() == "file")
        origin = securityOrigin->fileSystemPath();
    else
        origin = securityOrigin->toString();

    return origin;
}

GeolocationClientBlackBerry::GeolocationClientBlackBerry(BlackBerry::WebKit::WebPagePrivate* webPagePrivate)
    : m_webPagePrivate(webPagePrivate)
    , m_accuracy(false)
    , m_isActive(false)
{
}

void GeolocationClientBlackBerry::geolocationDestroyed()
{
    BlackBerry::Platform::GeolocationHandler::instance()->unregisterFromPermissionTracking(this);

    delete this;
}

void GeolocationClientBlackBerry::startUpdating()
{
    if (!m_isActive)
        BlackBerry::Platform::GeolocationHandler::instance()->addListener(this);
    m_isActive = true;
}

void GeolocationClientBlackBerry::stopUpdating()
{
    if (m_isActive)
        BlackBerry::Platform::GeolocationHandler::instance()->removeListener(this);
    m_isActive = false;
}

GeolocationPosition* GeolocationClientBlackBerry::lastPosition()
{
    return m_lastPosition.get();
}

void GeolocationClientBlackBerry::requestPermission(Geolocation* location)
{
    Frame* frame = location->frame();
    if (!frame)
        return;

    if (!m_webPagePrivate->m_webSettings->isGeolocationEnabled()) {
        location->setIsAllowed(false);
        return;
    }

    const String origin = getOrigin(frame);

    // Special case for documents with the isUnique flag on. (ie. sandboxed iframes)
    if (origin == "null")
        location->setIsAllowed(false);

    // Check global location setting, if it is off then we request an infobar that invokes a location settings card.
    // If it's on, then we request an infobar that allows the site to have permission to use geolocation.
    if (!BlackBerry::Platform::GeolocationHandler::instance()->isGlobalServiceActive()) {
        // We only want to ask them once per session. If we get here again, automatically fail the request.
        if (!BlackBerry::Platform::GeolocationHandler::instance()->didAskUserForGlobalPermission()) {
            m_webPagePrivate->m_client->requestGlobalLocalServicePermission(this, origin);
            BlackBerry::Platform::GeolocationHandler::instance()->setAskedUserForGlobalPermission();
        } else
            location->setIsAllowed(false);
        return;
    }

    // Register the listener with the GeolocationHandler to receive permissions.
    if (m_geolocationRequestMap.isEmpty())
        BlackBerry::Platform::GeolocationHandler::instance()->registerPermissionTracking(this);

    // Add this geolocation permission request to our request map.
    Vector<RefPtr<Geolocation> > geoRequestsForOrigin;
    HashMap<String, Vector<RefPtr<Geolocation> > >::AddResult result = m_geolocationRequestMap.add(origin, geoRequestsForOrigin);
    result.iterator->value.append(location);

    // Avoid sending another request if the vector already has another geolocation pending for this origin in this page.
    if (result.isNewEntry)
        m_webPagePrivate->m_client->requestGeolocationPermission(this, origin);
}

void GeolocationClientBlackBerry::cancelPermissionRequest(Geolocation* location)
{
    Frame* frame = location->frame();
    if (!frame)
        return;

    const String origin = getOrigin(frame);

    // Remove the geolocation from the pending permission map.
    HashMap<String, Vector<RefPtr<Geolocation> > >::iterator it = m_geolocationRequestMap.find(origin);
    if (it == m_geolocationRequestMap.end())
        return;

    Vector<RefPtr<Geolocation> >* result = &(it->value);

    size_t geolocationCount = result->size();
    for (size_t i = 0; i < geolocationCount; ++i) {
        if ((*result)[i].get() == location) {
            result->remove(i);
            // Remove this vector from the pending permission map is it doesn't contain anymore geo objects
            if (result->isEmpty())
                m_geolocationRequestMap.remove(origin);
            break;
        }
    }

    if (m_geolocationRequestMap.isEmpty())
        BlackBerry::Platform::GeolocationHandler::instance()->unregisterFromPermissionTracking(this);

    m_webPagePrivate->m_client->cancelGeolocationPermission();
}

void GeolocationClientBlackBerry::onLocationUpdate(double timestamp, double latitude, double longitude, double accuracy, double altitude, bool altitudeValid,
    double altitudeAccuracy, bool altitudeAccuracyValid, double speed, bool speedValid, double heading, bool headingValid)
{
    m_lastPosition = GeolocationPosition::create(timestamp, latitude, longitude, accuracy, altitudeValid, altitude, altitudeAccuracyValid,
        altitudeAccuracy, headingValid, heading, speedValid, speed);
    GeolocationController::from(m_webPagePrivate->m_page)->positionChanged(m_lastPosition.get());
}

void GeolocationClientBlackBerry::onLocationError(const char* errorStr)
{
    RefPtr<GeolocationError> error = GeolocationError::create(GeolocationError::PositionUnavailable, String::fromUTF8(errorStr));
    GeolocationController::from(m_webPagePrivate->m_page)->errorOccurred(error.get());
}

void GeolocationClientBlackBerry::onPermission(const BlackBerry::Platform::String& origin, bool isAllowed)
{
    Vector<RefPtr<Geolocation> > pendingPermissionGeolocation = m_geolocationRequestMap.get(origin);

    if (pendingPermissionGeolocation.isEmpty())
        return;

    size_t numberOfGeolocations = pendingPermissionGeolocation.size();
    for (size_t i = 0; i < numberOfGeolocations; ++i)
        pendingPermissionGeolocation[i]->setIsAllowed(isAllowed);
    m_geolocationRequestMap.remove(origin);

    if (m_geolocationRequestMap.isEmpty())
        BlackBerry::Platform::GeolocationHandler::instance()->unregisterFromPermissionTracking(this);
}

void GeolocationClientBlackBerry::setEnableHighAccuracy(bool newAccuracy)
{
    if (m_accuracy != newAccuracy) {
        m_accuracy = newAccuracy;
        BlackBerry::Platform::GeolocationHandler::instance()->switchAccuracy(this);
    }
}

