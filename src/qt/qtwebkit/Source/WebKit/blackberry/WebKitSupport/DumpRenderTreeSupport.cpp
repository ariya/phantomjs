/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 * Copyright (C) 2012 Apple Inc. All Rights Reserved.
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
#include "DumpRenderTreeSupport.h"

#include "DeviceOrientationClientMock.h"
#include "DeviceOrientationController.h"
#include "DeviceOrientationData.h"
#include "Frame.h"
#include "GeolocationClientMock.h"
#include "GeolocationController.h"
#include "GeolocationError.h"
#include "GeolocationPosition.h"
#include "JSCSSStyleDeclaration.h"
#include "JSElement.h"
#include "Page.h"
#include "RuntimeEnabledFeatures.h"
#include "WebPage_p.h"
#include "bindings/js/GCController.h"
#include <JavaScriptCore/APICast.h>
#include <wtf/CurrentTime.h>

using namespace BlackBerry::WebKit;
using namespace WebCore;
using namespace JSC;

bool DumpRenderTreeSupport::s_linksIncludedInTabChain = true;

GeolocationClientMock* toGeolocationClientMock(GeolocationClient* client)
{
    ASSERT(isRunningDrt());
    return static_cast<GeolocationClientMock*>(client);
}

DumpRenderTreeSupport::DumpRenderTreeSupport()
{
}

DumpRenderTreeSupport::~DumpRenderTreeSupport()
{
}

Page* DumpRenderTreeSupport::corePage(WebPage* webPage)
{
    return WebPagePrivate::core(webPage);
}

int DumpRenderTreeSupport::javaScriptObjectsCount()
{
    return JSDOMWindowBase::commonVM()->heap.globalObjectCount();
}

void DumpRenderTreeSupport::garbageCollectorCollect()
{
    gcController().garbageCollectNow();
}

void DumpRenderTreeSupport::garbageCollectorCollectOnAlternateThread(bool waitUntilDone)
{
    gcController().garbageCollectOnAlternateThreadForDebugging(waitUntilDone);
}

void DumpRenderTreeSupport::setLinksIncludedInFocusChain(bool enabled)
{
    s_linksIncludedInTabChain = enabled;
}

bool DumpRenderTreeSupport::linksIncludedInFocusChain()
{
    return s_linksIncludedInTabChain;
}

int DumpRenderTreeSupport::numberOfPendingGeolocationPermissionRequests(WebPage* webPage)
{
    GeolocationClientMock* mockClient = toGeolocationClientMock(GeolocationController::from(corePage(webPage))->client());
    return mockClient->numberOfPendingPermissionRequests();
}

void DumpRenderTreeSupport::resetGeolocationMock(WebPage* webPage)
{
    GeolocationClientMock* mockClient = toGeolocationClientMock(GeolocationController::from(corePage(webPage))->client());
    mockClient->reset();
}

void DumpRenderTreeSupport::setMockGeolocationPositionUnavailableError(WebPage* webPage, const String message)
{
    GeolocationClientMock* mockClient = static_cast<GeolocationClientMock*>(GeolocationController::from(corePage(webPage))->client());
    mockClient->setPositionUnavailableError(message);
}

void DumpRenderTreeSupport::setMockGeolocationPermission(WebPage* webPage, bool allowed)
{
    GeolocationClientMock* mockClient = toGeolocationClientMock(GeolocationController::from(corePage(webPage))->client());
    mockClient->setPermission(allowed);
}

void DumpRenderTreeSupport::setMockGeolocationPosition(WebPage* webPage, double latitude, double longitude, double accuracy, bool providesAltitude, double altitude, bool providesAltitudeAccuracy, double altitudeAccuracy, bool providesHeading, double heading, bool providesSpeed, double speed)
{
    GeolocationClientMock* mockClient = toGeolocationClientMock(GeolocationController::from(corePage(webPage))->client());
    mockClient->setPosition(GeolocationPosition::create(currentTime(), latitude, longitude, accuracy, providesAltitude, altitude, providesAltitudeAccuracy, altitudeAccuracy, providesHeading, heading, providesSpeed, speed));
}

void DumpRenderTreeSupport::scalePageBy(WebPage* webPage, float scaleFactor, float x, float y)
{
    corePage(webPage)->setPageScaleFactor(scaleFactor, IntPoint(x, y));
}

#if ENABLE(STYLE_SCOPED)
void DumpRenderTreeSupport::setStyleScopedEnabled(bool enabled)
{
    RuntimeEnabledFeatures::setStyleScopedEnabled(enabled);
}
#endif

#if ENABLE(DEVICE_ORIENTATION)
DeviceOrientationClientMock* toDeviceOrientationClientMock(DeviceOrientationClient* client)
{
    return static_cast<DeviceOrientationClientMock*>(client);
}
#endif

void DumpRenderTreeSupport::setMockDeviceOrientation(BlackBerry::WebKit::WebPage* webPage, bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
{
#if ENABLE(DEVICE_ORIENTATION)
    Page* page = corePage(webPage);
    DeviceOrientationClientMock* mockClient = toDeviceOrientationClientMock(DeviceOrientationController::from(page)->deviceOrientationClient());
    mockClient->setOrientation(DeviceOrientationData::create(canProvideAlpha, alpha, canProvideBeta, beta, canProvideGamma, gamma));
#endif
}
