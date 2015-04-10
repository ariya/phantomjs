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
#include "GeolocationProviderGeoclue.h"

#if ENABLE(GEOLOCATION)

#include <wtf/gobject/GOwnPtr.h>

using namespace WebCore;

static void getPositionCallback(GeocluePosition* position, GeocluePositionFields fields, int timestamp, double latitude, double longitude, double altitude, GeoclueAccuracy* accuracy, GError* error, GeolocationProviderGeoclue* provider)
{
    if (error) {
        provider->errorOccurred(error->message);
        g_error_free(error);
        return;
    }
    provider->positionChanged(position, fields, timestamp, latitude, longitude, altitude, accuracy);
}

static void positionChangedCallback(GeocluePosition* position, GeocluePositionFields fields, int timestamp, double latitude, double longitude, double altitude, GeoclueAccuracy* accuracy, GeolocationProviderGeoclue* provider)
{
    provider->positionChanged(position, fields, timestamp, latitude, longitude, altitude, accuracy);
}

static void createGeocluePositionCallback(GeoclueMasterClient*, GeocluePosition* position, GError *error, GeolocationProviderGeoclue* provider)
{
    if (error) {
        provider->errorOccurred(error->message);
        g_error_free(error);
        return;
    }
    provider->initializeGeocluePosition(position);
}

static void geoclueClientSetRequirementsCallback(GeoclueMasterClient* client, GError* error, GeolocationProviderGeoclue* provider)
{
    if (error) {
        provider->errorOccurred(error->message);
        g_error_free(error);
    }
}

static void createGeoclueClientCallback(GeoclueMaster*, GeoclueMasterClient* client, char*, GError* error, GeolocationProviderGeoclue* provider)
{
    if (error) {
        provider->errorOccurred(error->message);
        g_error_free(error);
        return;
    }
    provider->initializeGeoclueClient(client);
}

GeolocationProviderGeoclue::GeolocationProviderGeoclue(GeolocationProviderGeoclueClient* client)
    : m_client(client)
    , m_geoclueClient(0)
    , m_geocluePosition(0)
    , m_latitude(0)
    , m_longitude(0)
    , m_altitude(0)
    , m_accuracy(0)
    , m_altitudeAccuracy(0)
    , m_timestamp(0)
    , m_enableHighAccuracy(false)
    , m_isUpdating(false)
{
    ASSERT(m_client);
}

GeolocationProviderGeoclue::~GeolocationProviderGeoclue()
{
}

void GeolocationProviderGeoclue::startUpdating()
{
    ASSERT(!m_geoclueClient);

    GRefPtr<GeoclueMaster> master = adoptGRef(geoclue_master_get_default());
    geoclue_master_create_client_async(master.get(), reinterpret_cast<GeoclueCreateClientCallback>(createGeoclueClientCallback), this);
    m_isUpdating = true;
}

void GeolocationProviderGeoclue::stopUpdating()
{
    m_geocluePosition.clear();
    m_geoclueClient.clear();
    m_isUpdating = false;
}

void GeolocationProviderGeoclue::setEnableHighAccuracy(bool enable)
{
    m_enableHighAccuracy = enable;

    // If we're already updating we should report the new requirements in order
    // to change to a more suitable provider if needed. If not, return.
    if (!m_isUpdating)
        return;

    updateClientRequirements();
}

void GeolocationProviderGeoclue::initializeGeoclueClient(GeoclueMasterClient* geoclueClient)
{
    ASSERT(GEOCLUE_IS_MASTER_CLIENT(geoclueClient));

    // Store the GeoclueMasterClient object as created by GeoClue.
    m_geoclueClient = adoptGRef(geoclueClient);

    // Set the requirement for the client and ask it to create a position associated to it.
    updateClientRequirements();
    geoclue_master_client_create_position_async(m_geoclueClient.get(), reinterpret_cast<CreatePositionCallback>(createGeocluePositionCallback), this);
}

void GeolocationProviderGeoclue::initializeGeocluePosition(GeocluePosition* geocluePosition)
{
    ASSERT(GEOCLUE_IS_POSITION(geocluePosition));

    // Stores the GeocluePosition object associated with the GeoclueMasterClient.
    m_geocluePosition = adoptGRef(geocluePosition);

    // Get the initial position and keep updated of further changes.
    geoclue_position_get_position_async(m_geocluePosition.get(), reinterpret_cast<GeocluePositionCallback>(getPositionCallback), this);
    g_signal_connect(m_geocluePosition.get(), "position-changed", G_CALLBACK(positionChangedCallback), this);
}

void GeolocationProviderGeoclue::updateClientRequirements()
{
    if (!m_geoclueClient)
        return;

    GeoclueAccuracyLevel accuracyLevel = m_enableHighAccuracy ? GEOCLUE_ACCURACY_LEVEL_DETAILED : GEOCLUE_ACCURACY_LEVEL_LOCALITY;
    geoclue_master_client_set_requirements_async(m_geoclueClient.get(), accuracyLevel, 0, false, GEOCLUE_RESOURCE_ALL, reinterpret_cast<GeoclueSetRequirementsCallback>(geoclueClientSetRequirementsCallback), this);
}

void GeolocationProviderGeoclue::positionChanged(GeocluePosition* position, GeocluePositionFields fields, int timestamp, double latitude, double longitude, double altitude, GeoclueAccuracy* accuracy)
{
    if (!(fields & GEOCLUE_POSITION_FIELDS_LATITUDE && fields & GEOCLUE_POSITION_FIELDS_LONGITUDE)) {
        errorOccurred("Position could not be determined.");
        return;
    }

    m_timestamp = timestamp;
    m_latitude = latitude;
    m_longitude = longitude;
    m_altitude = altitude;

    geoclue_accuracy_get_details(accuracy, 0, &m_accuracy, &m_altitudeAccuracy);

    m_client->notifyPositionChanged(m_timestamp, m_latitude, m_longitude, m_altitude, m_accuracy, m_altitudeAccuracy);
}

void GeolocationProviderGeoclue::errorOccurred(const char* message)
{
    m_client->notifyErrorOccurred(message);
}

#endif // ENABLE(GEOLOCATION)
