/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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
#include "NotificationManager.h"

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#include "UUID.h"
#include "UserGestureIndicator.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformString.h>

#include <vector>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

NotificationManager::NotificationManager(WebPagePrivate* webPagePrivate)
    : m_webPagePrivate(webPagePrivate)
{
}

NotificationManager::~NotificationManager()
{
}

bool NotificationManager::show(Notification* notification)
{
    String notificationID = createCanonicalUUIDString();
    m_notificationMap.set(notification, notificationID);
    m_notificationIDMap.set(notificationID, notification);

    NotificationContextMap::iterator it = m_notificationContextMap.add(notification->scriptExecutionContext(), Vector<String>()).iterator;
    it->value.append(notificationID);

    m_webPagePrivate->client()->showNotification(notificationID, notification->title(), notification->body(), notification->iconURL().string(), notification->tag(), notification->scriptExecutionContext()->securityOrigin()->toString());
    return true;
}

void NotificationManager::cancel(Notification* notification)
{
    String notificationID = m_notificationMap.get(notification);
    if (!notificationID)
        return;

    m_webPagePrivate->client()->cancelNotification(notificationID);
}

void NotificationManager::clearNotifications(ScriptExecutionContext* context)
{
    NotificationContextMap::iterator it = m_notificationContextMap.find(context);
    if (it == m_notificationContextMap.end())
        return;

    Vector<String>& notificationIDs = it->value;
    std::vector<BlackBerry::Platform::String> ids;
    size_t count = notificationIDs.size();
    for (size_t i = 0; i < count; ++i) {
        ids.push_back(notificationIDs[i]);
        RefPtr<Notification> notification = m_notificationIDMap.take(notificationIDs[i]);
        if (!notification)
            continue;

        notification->finalize();
        m_notificationMap.remove(notification);
    }

    m_webPagePrivate->client()->clearNotifications(ids);
    m_notificationContextMap.remove(it);
}

void NotificationManager::notificationObjectDestroyed(Notification* notification)
{
    String notificationID = m_notificationMap.take(notification);
    if (!notificationID)
        return;

    m_notificationIDMap.remove(notificationID);
    removeNotificationFromContextMap(notificationID, notification);
    m_webPagePrivate->client()->notificationDestroyed(notificationID);
}

#if ENABLE(LEGACY_NOTIFICATIONS)
void NotificationManager::requestPermission(ScriptExecutionContext* context, PassRefPtr<VoidCallback> callback)
{
    SecurityOrigin* origin = context->securityOrigin();
    String requestID = createCanonicalUUIDString();
    m_originToIDMap.set(origin, requestID);
    m_idToOriginMap.set(requestID, origin);
    m_idToVoidCallbackMap.set(requestID, callback);
    m_webPagePrivate->client()->requestNotificationPermission(requestID, origin->toString());
}
#endif

#if ENABLE(NOTIFICATIONS)
void NotificationManager::requestPermission(ScriptExecutionContext* context, PassRefPtr<NotificationPermissionCallback> callback)
{
    SecurityOrigin* origin = context->securityOrigin();
    String requestID = createCanonicalUUIDString();
    m_originToIDMap.set(origin, requestID);
    m_idToOriginMap.set(requestID, origin);
    m_idToCallbackMap.set(requestID, callback);
    m_webPagePrivate->client()->requestNotificationPermission(requestID, origin->toString());
}
#endif

void NotificationManager::cancelRequestsForPermission(ScriptExecutionContext* context)
{
    SecurityOrigin* origin = context->securityOrigin();
    String requestID = m_originToIDMap.take(origin);
    if (!requestID)
        return;

    m_idToOriginMap.remove(requestID);
#if ENABLE(LEGACY_NOTIFICATIONS)
    m_idToVoidCallbackMap.remove(requestID);
#endif
#if ENABLE(NOTIFICATIONS)
    m_idToCallbackMap.remove(requestID);
#endif
}

NotificationClient::Permission NotificationManager::checkPermission(ScriptExecutionContext* context)
{
    return static_cast<NotificationClient::Permission>(m_webPagePrivate->client()->checkNotificationPermission(context->securityOrigin()->toString()));
}

void NotificationManager::updatePermission(const String& requestID, bool allowed)
{
    RefPtr<WebCore::SecurityOrigin> origin = m_idToOriginMap.take(requestID);
    m_originToIDMap.remove(origin);

#if ENABLE(LEGACY_NOTIFICATIONS)
    RefPtr<VoidCallback> voidCallback = m_idToVoidCallbackMap.take(requestID);
    if (voidCallback) {
        voidCallback->handleEvent();
        return;
    }
#endif

#if ENABLE(NOTIFICATIONS)
    RefPtr<NotificationPermissionCallback> callback = m_idToCallbackMap.take(requestID);
    if (!callback)
        return;

    callback->handleEvent(Notification::permissionString(allowed ? NotificationClient::PermissionAllowed : NotificationClient::PermissionDenied));
#endif
}

void NotificationManager::notificationClicked(const String& notificationID)
{
    RefPtr<Notification> notification = m_notificationIDMap.get(notificationID);
    if (!notification)
        return;

    // Indicate that this event is being dispatched in reaction to a user's interaction with a platform notification.
    UserGestureIndicator indicator(DefinitelyProcessingUserGesture);
    notification->dispatchClickEvent();
}

void NotificationManager::notificationClosed(const String& notificationID)
{
    RefPtr<Notification> notification = m_notificationIDMap.take(notificationID);
    if (!notification)
        return;

    m_notificationMap.remove(notification);
    removeNotificationFromContextMap(notificationID, notification.get());
    notification->dispatchCloseEvent();
}

void NotificationManager::notificationError(const String& notificationID)
{
    RefPtr<Notification> notification = m_notificationIDMap.take(notificationID);
    if (!notification)
        return;

    notification->dispatchErrorEvent();
}

void NotificationManager::notificationShown(const String& notificationID)
{
    RefPtr<Notification> notification = m_notificationIDMap.get(notificationID);
    if (!notification)
        return;

    notification->dispatchShowEvent();
}

void NotificationManager::removeNotificationFromContextMap(const String& notificationID, Notification* notification)
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

}
}

#endif // ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
