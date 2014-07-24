/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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
#include "NotificationClientBlackBerry.h"

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#include "Notification.h"
#include "NotificationPermissionCallback.h"
#include "ScriptExecutionContext.h"
#include "VoidCallback.h"
#include "WebPage_p.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

NotificationClientBlackBerry::NotificationClientBlackBerry(BlackBerry::WebKit::WebPagePrivate* webPagePrivate)
    : m_webPagePrivate(webPagePrivate)
{
}

bool NotificationClientBlackBerry::show(Notification* notification)
{
    m_webPagePrivate->notificationManager().show(notification);
    return true;
}

void NotificationClientBlackBerry::cancel(Notification* notification)
{
    m_webPagePrivate->notificationManager().cancel(notification);
}


void NotificationClientBlackBerry::clearNotifications(ScriptExecutionContext* context)
{
    m_webPagePrivate->notificationManager().clearNotifications(context);
}

void NotificationClientBlackBerry::notificationObjectDestroyed(Notification* notification)
{
    m_webPagePrivate->notificationManager().notificationObjectDestroyed(notification);
}

void NotificationClientBlackBerry::notificationControllerDestroyed()
{
    delete this;
}

void NotificationClientBlackBerry::requestPermission(ScriptExecutionContext* context, PassRefPtr<VoidCallback> callback)
{
    m_webPagePrivate->notificationManager().requestPermission(context, callback);
}

void NotificationClientBlackBerry::requestPermission(ScriptExecutionContext* context, PassRefPtr<NotificationPermissionCallback> callback)
{
    m_webPagePrivate->notificationManager().requestPermission(context, callback);
}

void NotificationClientBlackBerry::cancelRequestsForPermission(ScriptExecutionContext* context)
{
    m_webPagePrivate->notificationManager().cancelRequestsForPermission(context);
}

NotificationClient::Permission NotificationClientBlackBerry::checkPermission(ScriptExecutionContext* context)
{
    return m_webPagePrivate->notificationManager().checkPermission(context);
}

}

#endif // ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
