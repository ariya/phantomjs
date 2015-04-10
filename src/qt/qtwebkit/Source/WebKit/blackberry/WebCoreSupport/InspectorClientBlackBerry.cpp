/*
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
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
#include "InspectorClientBlackBerry.h"

#include "BackingStore.h"
#include "Frame.h"
#include "GraphicsContext.h"
#include "InspectorController.h"
#include "NotImplemented.h"
#include "Page.h"
#include "RenderObject.h"
#include "WebPageClient.h"
#include "WebPage_p.h"

namespace WebCore {

InspectorClientBlackBerry::InspectorClientBlackBerry(BlackBerry::WebKit::WebPagePrivate* webPagePrivate)
    : m_webPagePrivate(webPagePrivate)
{
    m_inspectorSettingsMap = adoptPtr(new SettingsMap);
}

void InspectorClientBlackBerry::inspectorDestroyed()
{
    delete this;
}

void InspectorClientBlackBerry::highlight()
{
    m_webPagePrivate->setInspectorOverlayClient(this);
}

void InspectorClientBlackBerry::hideHighlight()
{
    m_webPagePrivate->setInspectorOverlayClient(0);
}

InspectorFrontendChannel* InspectorClientBlackBerry::openInspectorFrontend(InspectorController*)
{
    notImplemented();
    return 0;
}

void InspectorClientBlackBerry::closeInspectorFrontend()
{
    notImplemented();
}

void InspectorClientBlackBerry::bringFrontendToFront()
{
    notImplemented();
}

bool InspectorClientBlackBerry::sendMessageToFrontend(const String& message)
{
    CString utf8Message = message.utf8();
    m_webPagePrivate->m_client->handleWebInspectorMessageToFrontend(0, utf8Message.data(), utf8Message.length());
    return true;
}

void InspectorClientBlackBerry::clearBrowserCache()
{
    m_webPagePrivate->m_client->clearCache();
}

void InspectorClientBlackBerry::clearBrowserCookies()
{
    m_webPagePrivate->m_client->clearCookies();
}

bool InspectorClientBlackBerry::canOverrideDeviceMetrics()
{
    return true;
}

void InspectorClientBlackBerry::overrideDeviceMetrics(int width, int height, float fontScaleFactor, bool)
{
    // Note: when width and height = 0, and fontScaleFactor = 1, this is the signal for restoring to default size.
    m_webPagePrivate->applySizeOverride(width, height);
    m_webPagePrivate->setTextZoomFactor(fontScaleFactor);
}

bool InspectorClientBlackBerry::supportsFrameInstrumentation()
{
    return true;
}

void InspectorClientBlackBerry::updateInspectorStateCookie(const String&)
{
    // If this is implemented, we should override and return true in InspectorStateClient::supportsInspectorStateUpdates().
    notImplemented();
};

void InspectorClientBlackBerry::paintInspectorOverlay(GraphicsContext& gc)
{
    InspectorController* inspectorController = m_webPagePrivate->m_page->inspectorController();
    if (inspectorController)
        inspectorController->drawHighlight(gc);
}

} // namespace WebCore
