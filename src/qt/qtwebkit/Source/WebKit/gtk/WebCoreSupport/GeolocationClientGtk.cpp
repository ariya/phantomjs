/*
 * Copyright (C) 2008 Holger Hans Peter Freyther <zecke@selfish.org>
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
 *
 */

#include "config.h"
#include "GeolocationClientGtk.h"

#if ENABLE(GEOLOCATION)

#include "Chrome.h"
#include "ChromeClient.h"
#include "Geolocation.h"
#include "GeolocationController.h"
#include "GeolocationError.h"
#include "GeolocationPosition.h"
#include "webkitgeolocationpolicydecisionprivate.h"
#include "webkitwebframeprivate.h"
#include "webkitwebviewprivate.h"
#include <glib/gi18n-lib.h>
#include <wtf/gobject/GRefPtr.h>

namespace WebKit {

GeolocationClient::GeolocationClient(WebKitWebView* webView)
    : m_webView(webView)
    , m_provider(this)
{
}

void GeolocationClient::geolocationDestroyed()
{
    delete this;
}

void GeolocationClient::startUpdating()
{
    m_provider.startUpdating();
}

void GeolocationClient::stopUpdating()
{
    m_provider.stopUpdating();
}

void GeolocationClient::setEnableHighAccuracy(bool enable)
{
    m_provider.setEnableHighAccuracy(enable);
}

WebCore::GeolocationPosition* GeolocationClient::lastPosition()
{
    return m_lastPosition.get();
}

void GeolocationClient::requestPermission(WebCore::Geolocation* geolocation)
{
    WebKitWebFrame* webFrame = kit(geolocation->frame());
    GRefPtr<WebKitGeolocationPolicyDecision> policyDecision(adoptGRef(webkit_geolocation_policy_decision_new(webFrame, geolocation)));

    gboolean isHandled = FALSE;
    g_signal_emit_by_name(m_webView, "geolocation-policy-decision-requested", webFrame, policyDecision.get(), &isHandled);
    if (!isHandled)
        webkit_geolocation_policy_deny(policyDecision.get());
}

void GeolocationClient::cancelPermissionRequest(WebCore::Geolocation* geolocation)
{
    g_signal_emit_by_name(m_webView, "geolocation-policy-decision-cancelled", kit(geolocation->frame()));
}

void GeolocationClient::notifyPositionChanged(int timestamp, double latitude, double longitude, double altitude, double accuracy, double altitudeAccuracy)
{
    m_lastPosition = WebCore::GeolocationPosition::create(static_cast<double>(timestamp), latitude, longitude, accuracy,
                                                          true, altitude, true, altitudeAccuracy, false, 0, false, 0);
    WebCore::GeolocationController::from(core(m_webView))->positionChanged(m_lastPosition.get());
}

void GeolocationClient::notifyErrorOccurred(const char* message)
{
    RefPtr<WebCore::GeolocationError> error = WebCore::GeolocationError::create(WebCore::GeolocationError::PositionUnavailable, message);
    WebCore::GeolocationController::from(core(m_webView))->errorOccurred(error.get());
}

}

#endif // ENABLE(GEOLOCATION)
