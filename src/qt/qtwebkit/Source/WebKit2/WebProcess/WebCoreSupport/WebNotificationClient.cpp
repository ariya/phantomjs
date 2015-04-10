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
#include "WebNotificationClient.h"

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)

#include "NotificationPermissionRequestManager.h"
#include "WebNotificationManager.h"
#include "WebPage.h"
#include "WebProcess.h"
#include <WebCore/ScriptExecutionContext.h>

using namespace WebCore;

namespace WebKit {

WebNotificationClient::WebNotificationClient(WebPage* page)
    : m_page(page)
{
}

WebNotificationClient::~WebNotificationClient()
{
}

bool WebNotificationClient::show(Notification* notification)
{
    return WebProcess::shared().supplement<WebNotificationManager>()->show(notification, m_page);
}

void WebNotificationClient::cancel(Notification* notification)
{
    WebProcess::shared().supplement<WebNotificationManager>()->cancel(notification, m_page);
}

void WebNotificationClient::clearNotifications(ScriptExecutionContext* context)
{
    WebProcess::shared().supplement<WebNotificationManager>()->clearNotifications(context, m_page);
}

void WebNotificationClient::notificationObjectDestroyed(Notification* notification)
{
    WebProcess::shared().supplement<WebNotificationManager>()->didDestroyNotification(notification, m_page);
}

void WebNotificationClient::notificationControllerDestroyed()
{
    delete this;
}

#if ENABLE(LEGACY_NOTIFICATIONS)
void WebNotificationClient::requestPermission(ScriptExecutionContext* context, PassRefPtr<VoidCallback> callback)
{
    m_page->notificationPermissionRequestManager()->startRequest(context->securityOrigin(), callback);
}
#endif

#if ENABLE(NOTIFICATIONS)
void WebNotificationClient::requestPermission(ScriptExecutionContext* context, PassRefPtr<NotificationPermissionCallback> callback)
{
    m_page->notificationPermissionRequestManager()->startRequest(context->securityOrigin(), callback);
}
#endif

void WebNotificationClient::cancelRequestsForPermission(ScriptExecutionContext* context)
{
    m_page->notificationPermissionRequestManager()->cancelRequest(context->securityOrigin());
}

NotificationClient::Permission WebNotificationClient::checkPermission(ScriptExecutionContext* context)
{
    if (!context || !context->isDocument())
        return NotificationClient::PermissionDenied;
    return m_page->notificationPermissionRequestManager()->permissionLevel(context->securityOrigin());
}

} // namespace WebKit

#endif // ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
