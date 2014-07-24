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

#ifndef GeolocationClientBlackBerry_h
#define GeolocationClientBlackBerry_h

#include <BlackBerryPlatformGeoTracker.h>
#include <BlackBerryPlatformGeoTrackerListener.h>
#include <GeolocationClient.h>
#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace BlackBerry {
namespace WebKit {
class WebPagePrivate;
}
}

namespace WebCore {

class GeolocationClientBlackBerry : public GeolocationClient, public BlackBerry::Platform::GeoTrackerListener {
public:
    GeolocationClientBlackBerry(BlackBerry::WebKit::WebPagePrivate*);

    virtual void geolocationDestroyed();
    virtual void startUpdating();
    virtual void stopUpdating();
    virtual GeolocationPosition* lastPosition();
    virtual void setEnableHighAccuracy(bool);
    virtual void requestPermission(Geolocation*);
    virtual void cancelPermissionRequest(Geolocation*);

    virtual bool requiresHighAccuracy() { return m_accuracy; }
    virtual void onLocationUpdate(double timestamp, double latitude, double longitude, double accuracy, double altitude, bool altitudeValid, double altitudeAccuracy,
        bool altitudeAccuracyValid, double speed, bool speedValid, double heading, bool headingValid);
    virtual void onLocationError(const char* error);
    virtual void onPermission(const BlackBerry::Platform::String& origin, bool isAllowed);

private:
    BlackBerry::WebKit::WebPagePrivate* m_webPagePrivate;
    RefPtr<GeolocationPosition> m_lastPosition;
    HashMap<String, Vector<RefPtr<Geolocation> > > m_geolocationRequestMap;
    bool m_accuracy;
    bool m_isActive;
};
}

#endif // GeolocationClientBlackBerry_h
