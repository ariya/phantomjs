/*
 *  Copyright (C) 2011 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "NotificationPresenterClientEfl.h"

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#include "NotImplemented.h"

namespace WebCore {

NotificationPresenterClientEfl::NotificationPresenterClientEfl()
{
}

NotificationPresenterClientEfl::~NotificationPresenterClientEfl()
{
}

bool NotificationPresenterClientEfl::show(Notification* notification)
{
    notImplemented();
    return false;
}

void NotificationPresenterClientEfl::cancel(Notification* notification)
{
    notImplemented();
}

void NotificationPresenterClientEfl::notificationObjectDestroyed(Notification* notification)
{
    notImplemented();
}

void NotificationPresenterClientEfl::notificationControllerDestroyed()
{
    notImplemented();
}

void NotificationPresenterClientEfl::requestPermission(ScriptExecutionContext* context, PassRefPtr<VoidCallback> callback)
{
    notImplemented();
}

NotificationClient::Permission NotificationPresenterClientEfl::checkPermission(ScriptExecutionContext* context)
{
    notImplemented();
    return PermissionDenied;
}

void NotificationPresenterClientEfl::cancelRequestsForPermission(ScriptExecutionContext* context)
{
    notImplemented();
}

}
#endif
