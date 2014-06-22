/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef GeolocationProviderGeoclue_h
#define GeolocationProviderGeoclue_h

#if ENABLE(GEOLOCATION)

#include "GeolocationProviderGeoclueClient.h"
#include <geoclue/geoclue-master.h>
#include <geoclue/geoclue-position.h>
#include <wtf/gobject/GRefPtr.h>

namespace WebCore {

class GeolocationProviderGeoclue {
public:
    GeolocationProviderGeoclue(GeolocationProviderGeoclueClient*);
    ~GeolocationProviderGeoclue();

    void startUpdating();
    void stopUpdating();
    void setEnableHighAccuracy(bool);

    // To be used from signal callbacks.
    void initializeGeoclueClient(GeoclueMasterClient*);
    void initializeGeocluePosition(GeocluePosition*);
    void updateClientRequirements();
    void positionChanged(GeocluePosition*, GeocluePositionFields, int, double, double, double, GeoclueAccuracy*);
    void errorOccurred(const char*);

private:
    GeolocationProviderGeoclueClient* m_client;

    GRefPtr<GeoclueMasterClient> m_geoclueClient;
    GRefPtr<GeocluePosition> m_geocluePosition;

    double m_latitude;
    double m_longitude;
    double m_altitude;
    double m_accuracy;
    double m_altitudeAccuracy;
    int m_timestamp;

    bool m_enableHighAccuracy;
    bool m_isUpdating;
};

} // namespace WebCore

#endif // ENABLE(GEOLOCATION)

#endif // GeolocationProviderGeoclue_h
