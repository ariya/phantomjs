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

#ifndef NotificationClientBlackBerry_h
#define NotificationClientBlackBerry_h

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#include "NotificationClient.h"

namespace BlackBerry {
namespace WebKit {
class WebPagePrivate;
}
}

namespace WebCore {

class NotificationClientBlackBerry : public NotificationClient {
public:
    NotificationClientBlackBerry(BlackBerry::WebKit::WebPagePrivate*);

    virtual bool show(Notification*);
    virtual void cancel(Notification*);
    virtual void clearNotifications(ScriptExecutionContext*);
    virtual void notificationObjectDestroyed(Notification*);
    virtual void notificationControllerDestroyed();
#if ENABLE(LEGACY_NOTIFICATIONS)
    virtual void requestPermission(ScriptExecutionContext*, PassRefPtr<VoidCallback>);
#endif
#if ENABLE(NOTIFICATIONS)
    virtual void requestPermission(ScriptExecutionContext*, PassRefPtr<NotificationPermissionCallback>);
#endif
    virtual void cancelRequestsForPermission(ScriptExecutionContext*);
    virtual Permission checkPermission(ScriptExecutionContext*);

private:
    BlackBerry::WebKit::WebPagePrivate* m_webPagePrivate;
};

}

#endif // ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#endif // NotificationClientBlackBerry_h
