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

#ifndef NotificationManager_h
#define NotificationManager_h

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#include "Notification.h"
#include "NotificationClient.h"
#include "NotificationPermissionCallback.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include "VoidCallback.h"
#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace BlackBerry {
namespace WebKit {
class WebPagePrivate;

class NotificationManager {
public:
    NotificationManager(WebPagePrivate*);
    ~NotificationManager();

    bool show(WebCore::Notification*);
    void cancel(WebCore::Notification*);
    void clearNotifications(WebCore::ScriptExecutionContext*);
    void notificationObjectDestroyed(WebCore::Notification*);
#if ENABLE(LEGACY_NOTIFICATIONS)
    void requestPermission(WebCore::ScriptExecutionContext*, PassRefPtr<WebCore::VoidCallback>);
#endif
#if ENABLE(NOTIFICATIONS)
    void requestPermission(WebCore::ScriptExecutionContext*, PassRefPtr<WebCore::NotificationPermissionCallback>);
#endif
    void cancelRequestsForPermission(WebCore::ScriptExecutionContext*);
    WebCore::NotificationClient::Permission checkPermission(WebCore::ScriptExecutionContext*);

    void updatePermission(const String& requestID, bool allowed);

    void notificationClicked(const String& notificationID);
    void notificationClosed(const String& notificationID);
    void notificationError(const String& notificationID);
    void notificationShown(const String& notificationID);

private:
    void removeNotificationFromContextMap(const String& notificationID, WebCore::Notification*);

    WebPagePrivate* m_webPagePrivate;
    typedef HashMap<RefPtr<WebCore::Notification>, String> NotificationMap;
    NotificationMap m_notificationMap;

    typedef HashMap<String, RefPtr<WebCore::Notification> > NotificationIdMap;
    NotificationIdMap m_notificationIDMap;

    typedef HashMap<RefPtr<WebCore::ScriptExecutionContext>, Vector<String> > NotificationContextMap;
    NotificationContextMap m_notificationContextMap;

#if ENABLE(NOTIFICATIONS)
    typedef HashMap<String, RefPtr<WebCore::NotificationPermissionCallback> > PermissionCallbackMap;
    PermissionCallbackMap m_idToCallbackMap;
#endif
#if ENABLE(LEGACY_NOTIFICATIONS)
    typedef HashMap<String, RefPtr<WebCore::VoidCallback> > VoidCallbackMap;
    VoidCallbackMap m_idToVoidCallbackMap;
#endif
    typedef HashMap<RefPtr<WebCore::SecurityOrigin>, String> OriginMap;
    OriginMap m_originToIDMap;

    typedef HashMap<String, RefPtr<WebCore::SecurityOrigin> > OriginIdMap;
    OriginIdMap m_idToOriginMap;
};

}
}

#endif // ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#endif // NotificationManager_h
