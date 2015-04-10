/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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
#include "NotificationPermissionRequestManager.h"

#include "WebCoreArgumentCoders.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/Notification.h>
#include <WebCore/Page.h>
#include <WebCore/ScriptExecutionContext.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/Settings.h>

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#include "WebNotificationManager.h"
#endif

using namespace WebCore;

namespace WebKit {

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
static uint64_t generateRequestID()
{
    static uint64_t uniqueRequestID = 1;
    return uniqueRequestID++;
}
#endif

PassRefPtr<NotificationPermissionRequestManager> NotificationPermissionRequestManager::create(WebPage* page)
{
    return adoptRef(new NotificationPermissionRequestManager(page));
}

NotificationPermissionRequestManager::NotificationPermissionRequestManager(WebPage* page)
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    : m_page(page)
#endif
{
    (void)page;
}

#if ENABLE(NOTIFICATIONS)
void NotificationPermissionRequestManager::startRequest(SecurityOrigin* origin, PassRefPtr<NotificationPermissionCallback> callback)
{
    NotificationClient::Permission permission = permissionLevel(origin);
    if (permission != NotificationClient::PermissionNotAllowed) {
        if (callback)
            callback->handleEvent(Notification::permissionString(permission));
        return;
    }

    uint64_t requestID = generateRequestID();
    m_originToIDMap.set(origin, requestID);
    m_idToOriginMap.set(requestID, origin);
    m_idToCallbackMap.set(requestID, callback);
    m_page->send(Messages::WebPageProxy::RequestNotificationPermission(requestID, origin->toString()));
}
#endif

#if ENABLE(LEGACY_NOTIFICATIONS)
void NotificationPermissionRequestManager::startRequest(SecurityOrigin* origin, PassRefPtr<VoidCallback> callback)
{
    NotificationClient::Permission permission = permissionLevel(origin);
    if (permission != NotificationClient::PermissionNotAllowed) {
        if (callback)
            callback->handleEvent();
        return;
    }
    
    uint64_t requestID = generateRequestID();
    m_originToIDMap.set(origin, requestID);
    m_idToOriginMap.set(requestID, origin);
    m_idToVoidCallbackMap.set(requestID, callback);
    m_page->send(Messages::WebPageProxy::RequestNotificationPermission(requestID, origin->toString()));
}
#endif

void NotificationPermissionRequestManager::cancelRequest(SecurityOrigin* origin)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    uint64_t id = m_originToIDMap.take(origin);
    if (!id)
        return;
    
    m_idToOriginMap.remove(id);
#if ENABLE(NOTIFICATIONS)
    m_idToCallbackMap.remove(id);
#endif
#if ENABLE(LEGACY_NOTIFICATIONS)
    m_idToVoidCallbackMap.remove(id);
#endif
#else
    UNUSED_PARAM(origin);
#endif
}

NotificationClient::Permission NotificationPermissionRequestManager::permissionLevel(SecurityOrigin* securityOrigin)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    if (!m_page->corePage()->settings()->notificationsEnabled())
        return NotificationClient::PermissionDenied;
    
    return WebProcess::shared().supplement<WebNotificationManager>()->policyForOrigin(securityOrigin);
#else
    UNUSED_PARAM(securityOrigin);
    return NotificationClient::PermissionDenied;
#endif
}

void NotificationPermissionRequestManager::setPermissionLevelForTesting(const String& originString, bool allowed)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    WebProcess::shared().supplement<WebNotificationManager>()->didUpdateNotificationDecision(originString, allowed);
#else
    UNUSED_PARAM(originString);
    UNUSED_PARAM(allowed);
#endif
}

void NotificationPermissionRequestManager::removeAllPermissionsForTesting()
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    WebProcess::shared().supplement<WebNotificationManager>()->removeAllPermissionsForTesting();
#endif
}

void NotificationPermissionRequestManager::didReceiveNotificationPermissionDecision(uint64_t requestID, bool allowed)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    if (!isRequestIDValid(requestID))
        return;

    RefPtr<WebCore::SecurityOrigin> origin = m_idToOriginMap.take(requestID);
    m_originToIDMap.remove(origin);

    WebProcess::shared().supplement<WebNotificationManager>()->didUpdateNotificationDecision(origin->toString(), allowed);

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

#else
    UNUSED_PARAM(requestID);
    UNUSED_PARAM(allowed);
#endif    
}

} // namespace WebKit
