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

#ifndef WebKitGeolocationProvider_h
#define WebKitGeolocationProvider_h

#if ENABLE(GEOLOCATION)

#include "WebKitPrivate.h"
#include <WebCore/GeolocationProviderGeoclue.h>
#include <WebCore/GeolocationProviderGeoclueClient.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebKit {

class WebKitGeolocationProvider : public RefCounted<WebKitGeolocationProvider>, public WebCore::GeolocationProviderGeoclueClient {
public:
    virtual ~WebKitGeolocationProvider();
    static PassRefPtr<WebKitGeolocationProvider> create(WebGeolocationManagerProxy*);

    void startUpdating();
    void stopUpdating();

private:
    WebKitGeolocationProvider(WebGeolocationManagerProxy*);

    // GeolocationProviderGeoclueClient interface.
    virtual void notifyPositionChanged(int, double, double, double, double, double);
    virtual void notifyErrorOccurred(const char*);

    RefPtr<WebGeolocationManagerProxy> m_geolocationManager;
    WebCore::GeolocationProviderGeoclue m_provider;
};

} // namespace WebKit

#endif // ENABLE(GEOLOCATION)

#endif // WebKitGeolocationProvider_h
