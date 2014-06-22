/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "WebNotificationProvider.h"

#include <WebKit2/WKMutableArray.h>
#include <WebKit2/WKNotification.h>
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKSecurityOrigin.h>
#include <wtf/Assertions.h>

namespace WTR {

static void showWebNotification(WKPageRef page, WKNotificationRef notification, const void* clientInfo)
{
    static_cast<WebNotificationProvider*>(const_cast<void*>(clientInfo))->showWebNotification(page, notification);
}

static void closeWebNotification(WKNotificationRef notification, const void* clientInfo)
{
    static_cast<WebNotificationProvider*>(const_cast<void*>(clientInfo))->closeWebNotification(notification);
}

static void addNotificationManager(WKNotificationManagerRef manager, const void* clientInfo)
{
    static_cast<WebNotificationProvider*>(const_cast<void*>(clientInfo))->addNotificationManager(manager);
}

static void removeNotificationManager(WKNotificationManagerRef manager, const void* clientInfo)
{
    static_cast<WebNotificationProvider*>(const_cast<void*>(clientInfo))->removeNotificationManager(manager);
}

static WKDictionaryRef notificationPermissions(const void* clientInfo)
{
    return static_cast<WebNotificationProvider*>(const_cast<void*>(clientInfo))->notificationPermissions();
}

WebNotificationProvider::WebNotificationProvider()
{
}

WebNotificationProvider::~WebNotificationProvider()
{
    WKNotificationManagerSetProvider(m_notificationManager.get(), 0);
}

WKNotificationProvider WebNotificationProvider::provider()
{
    WKNotificationProvider notificationProvider = {
        kWKNotificationProviderCurrentVersion,
        this,
        WTR::showWebNotification,
        WTR::closeWebNotification,
        0, // didDestroyNotification
        WTR::addNotificationManager,
        WTR::removeNotificationManager,
        WTR::notificationPermissions,
        0, // clearNotifications
    };
    return notificationProvider;
}

void WebNotificationProvider::showWebNotification(WKPageRef, WKNotificationRef notification)
{
    if (!m_notificationManager)
        return;

    uint64_t id = WKNotificationGetID(notification);
    ASSERT(!m_shownNotifications.contains(id));
    m_shownNotifications.add(id);

    WKNotificationManagerProviderDidShowNotification(m_notificationManager.get(), WKNotificationGetID(notification));
}

void WebNotificationProvider::closeWebNotification(WKNotificationRef notification)
{
    if (!m_notificationManager)
        return;

    uint64_t id = WKNotificationGetID(notification);
    WKRetainPtr<WKUInt64Ref> wkID = WKUInt64Create(id);
    WKRetainPtr<WKMutableArrayRef> array(AdoptWK, WKMutableArrayCreate());
    WKArrayAppendItem(array.get(), wkID.get());
    m_shownNotifications.remove(id);
    WKNotificationManagerProviderDidCloseNotifications(m_notificationManager.get(), array.get());
}

void WebNotificationProvider::addNotificationManager(WKNotificationManagerRef manager)
{
    // We assume there is only one for testing.
    ASSERT(!m_notificationManager);
    m_notificationManager = manager;
}

void WebNotificationProvider::removeNotificationManager(WKNotificationManagerRef manager)
{
    // We assume there is only one for testing.
    ASSERT(m_notificationManager);
    ASSERT(m_notificationManager.get() == manager);
    m_notificationManager = 0;
}

WKDictionaryRef WebNotificationProvider::notificationPermissions()
{
    // Initial permissions are always empty.
    return WKMutableDictionaryCreate();
}

void WebNotificationProvider::simulateWebNotificationClick(uint64_t notificationID)
{
    if (!m_notificationManager)
        return;

    ASSERT(m_shownNotifications.contains(notificationID));
    WKNotificationManagerProviderDidClickNotification(m_notificationManager.get(), notificationID);
}

void WebNotificationProvider::reset()
{
    if (!m_notificationManager) {
        m_shownNotifications.clear();
        return;
    }

    WKRetainPtr<WKMutableArrayRef> array(AdoptWK, WKMutableArrayCreate());
    HashSet<uint64_t>::const_iterator itEnd = m_shownNotifications.end();
    for (HashSet<uint64_t>::const_iterator it = m_shownNotifications.begin(); it != itEnd; ++it) {
        WKRetainPtr<WKUInt64Ref> wkID = WKUInt64Create(*it);
        WKArrayAppendItem(array.get(), wkID.get());
    }

    m_shownNotifications.clear();
    WKNotificationManagerProviderDidCloseNotifications(m_notificationManager.get(), array.get());
}

} // namespace WTR
