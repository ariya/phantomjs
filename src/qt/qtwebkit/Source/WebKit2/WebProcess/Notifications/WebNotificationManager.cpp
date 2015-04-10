/*
 * Copyright (C) 2011, 2012, 2013 Apple Inc. All rights reserved.
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
#include "WebNotificationManager.h"

#include "WebPage.h"
#include "WebProcess.h"
#include "WebProcessCreationParameters.h"

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#include "WebNotification.h"
#include "WebNotificationManagerMessages.h"
#include "WebPageProxyMessages.h"
#include <WebCore/Document.h>
#include <WebCore/Notification.h>
#include <WebCore/Page.h>
#include <WebCore/ScriptExecutionContext.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/Settings.h>
#include <WebCore/UserGestureIndicator.h>
#endif

using namespace WebCore;

namespace WebKit {

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
static uint64_t generateNotificationID()
{
    static uint64_t uniqueNotificationID = 1;
    return uniqueNotificationID++;
}
#endif

const char* WebNotificationManager::supplementName()
{
    return "WebNotificationManager";
}

WebNotificationManager::WebNotificationManager(WebProcess* process)
    : m_process(process)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    m_process->addMessageReceiver(Messages::WebNotificationManager::messageReceiverName(), this);
#endif
}

WebNotificationManager::~WebNotificationManager()
{
}

void WebNotificationManager::initialize(const WebProcessCreationParameters& parameters)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    m_permissionsMap = parameters.notificationPermissions;
#else
    UNUSED_PARAM(parameters);
#endif
}

void WebNotificationManager::didUpdateNotificationDecision(const String& originString, bool allowed)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    m_permissionsMap.set(originString, allowed);
#else
    UNUSED_PARAM(originString);
    UNUSED_PARAM(allowed);
#endif
}

void WebNotificationManager::didRemoveNotificationDecisions(const Vector<String>& originStrings)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    size_t count = originStrings.size();
    for (size_t i = 0; i < count; ++i)
        m_permissionsMap.remove(originStrings[i]);
#else
    UNUSED_PARAM(originStrings);
#endif
}

NotificationClient::Permission WebNotificationManager::policyForOrigin(WebCore::SecurityOrigin *origin) const
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    if (!origin)
        return NotificationClient::PermissionNotAllowed;

    ASSERT(!origin->isUnique());
    HashMap<String, bool>::const_iterator it = m_permissionsMap.find(origin->toRawString());
    if (it != m_permissionsMap.end())
        return it->value ? NotificationClient::PermissionAllowed : NotificationClient::PermissionDenied;
#else
    UNUSED_PARAM(origin);
#endif
    
    return NotificationClient::PermissionNotAllowed;
}

void WebNotificationManager::removeAllPermissionsForTesting()
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    m_permissionsMap.clear();
#endif
}

uint64_t WebNotificationManager::notificationIDForTesting(Notification* notification)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    if (!notification)
        return 0;
    return m_notificationMap.get(notification);
#else
    UNUSED_PARAM(notification);
    return 0;
#endif
}

bool WebNotificationManager::show(Notification* notification, WebPage* page)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    if (!notification || !page->corePage()->settings()->notificationsEnabled())
        return false;
    
    uint64_t notificationID = generateNotificationID();
    m_notificationMap.set(notification, notificationID);
    m_notificationIDMap.set(notificationID, notification);
    
    NotificationContextMap::iterator it = m_notificationContextMap.add(notification->scriptExecutionContext(), Vector<uint64_t>()).iterator;
    it->value.append(notificationID);

#if ENABLE(NOTIFICATIONS)
    m_process->parentProcessConnection()->send(Messages::WebPageProxy::ShowNotification(notification->title(), notification->body(), notification->iconURL().string(), notification->tag(), notification->lang(), notification->dir(), notification->scriptExecutionContext()->securityOrigin()->toString(), notificationID), page->pageID());
#else
    m_process->parentProcessConnection()->send(Messages::WebPageProxy::ShowNotification(notification->title(), notification->body(), notification->iconURL().string(), notification->replaceId(), notification->lang(), notification->dir(), notification->scriptExecutionContext()->securityOrigin()->toString(), notificationID), page->pageID());
#endif
    return true;
#else
    UNUSED_PARAM(notification);
    UNUSED_PARAM(page);
    return false;
#endif
}

void WebNotificationManager::cancel(Notification* notification, WebPage* page)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    if (!notification || !page->corePage()->settings()->notificationsEnabled())
        return;
    
    uint64_t notificationID = m_notificationMap.get(notification);
    if (!notificationID)
        return;
    
    m_process->parentProcessConnection()->send(Messages::WebPageProxy::CancelNotification(notificationID), page->pageID());
#else
    UNUSED_PARAM(notification);
    UNUSED_PARAM(page);
#endif
}

void WebNotificationManager::clearNotifications(WebCore::ScriptExecutionContext* context, WebPage* page)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    NotificationContextMap::iterator it = m_notificationContextMap.find(context);
    if (it == m_notificationContextMap.end())
        return;

    Vector<uint64_t>& notificationIDs = it->value;
    m_process->parentProcessConnection()->send(Messages::WebPageProxy::ClearNotifications(notificationIDs), page->pageID());
    size_t count = notificationIDs.size();
    for (size_t i = 0; i < count; ++i) {
        RefPtr<Notification> notification = m_notificationIDMap.take(notificationIDs[i]);
        if (!notification)
            continue;
        notification->finalize();
        m_notificationMap.remove(notification);
    }
    
    m_notificationContextMap.remove(it);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(page);
#endif
}

void WebNotificationManager::didDestroyNotification(Notification* notification, WebPage* page)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    uint64_t notificationID = m_notificationMap.take(notification);
    if (!notificationID)
        return;

    m_notificationIDMap.remove(notificationID);
    removeNotificationFromContextMap(notificationID, notification);
    m_process->parentProcessConnection()->send(Messages::WebPageProxy::DidDestroyNotification(notificationID), page->pageID());
#else
    UNUSED_PARAM(notification);
    UNUSED_PARAM(page);
#endif
}

void WebNotificationManager::didShowNotification(uint64_t notificationID)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    if (!isNotificationIDValid(notificationID))
        return;
    
    RefPtr<Notification> notification = m_notificationIDMap.get(notificationID);
    if (!notification)
        return;

    notification->dispatchShowEvent();
#else
    UNUSED_PARAM(notificationID);
#endif
}

void WebNotificationManager::didClickNotification(uint64_t notificationID)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    if (!isNotificationIDValid(notificationID))
        return;

    RefPtr<Notification> notification = m_notificationIDMap.get(notificationID);
    if (!notification)
        return;

    // Indicate that this event is being dispatched in reaction to a user's interaction with a platform notification.
    UserGestureIndicator indicator(DefinitelyProcessingUserGesture);
    notification->dispatchClickEvent();
#else
    UNUSED_PARAM(notificationID);
#endif
}

void WebNotificationManager::didCloseNotifications(const Vector<uint64_t>& notificationIDs)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    size_t count = notificationIDs.size();
    for (size_t i = 0; i < count; ++i) {
        uint64_t notificationID = notificationIDs[i];
        if (!isNotificationIDValid(notificationID))
            continue;

        RefPtr<Notification> notification = m_notificationIDMap.take(notificationID);
        if (!notification)
            continue;

        m_notificationMap.remove(notification);
        removeNotificationFromContextMap(notificationID, notification.get());

        notification->dispatchCloseEvent();
    }
#else
    UNUSED_PARAM(notificationIDs);
#endif
}

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
void WebNotificationManager::removeNotificationFromContextMap(uint64_t notificationID, Notification* notification)
{
    // This is a helper function for managing the hash maps.
    NotificationContextMap::iterator it = m_notificationContextMap.find(notification->scriptExecutionContext());
    ASSERT(it != m_notificationContextMap.end());
    size_t index = it->value.find(notificationID);
    ASSERT(index != notFound);
    it->value.remove(index);
    if (it->value.isEmpty())
        m_notificationContextMap.remove(it);
}
#endif

} // namespace WebKit
