/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <WebCore/COMPtr.h>
#include <WebCore/Notification.h>
#include <WebCore/NotificationClient.h>

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)

interface IWebDesktopNotificationPresenter;

namespace WebCore {
class Document;
class KURL;
}

class WebDesktopNotificationsDelegate : public WebCore::NotificationClient {
public:
    WebDesktopNotificationsDelegate(WebView* view);

    /* WebCore::NotificationClient interface */
    virtual bool show(WebCore::Notification* object);
    virtual void cancel(WebCore::Notification* object);
    virtual void notificationObjectDestroyed(WebCore::Notification* object);
    virtual void notificationControllerDestroyed();
#if ENABLE(LEGACY_NOTIFICATIONS)
    virtual void requestPermission(WebCore::SecurityOrigin*, PassRefPtr<WebCore::VoidCallback>);
#endif
#if ENABLE(NOTIFICATIONS)
    virtual void requestPermission(WebCore::SecurityOrigin*, PassRefPtr<WebCore::NotificationPermissionCallback>);
#endif
    virtual void cancelRequestsForPermission(WebCore::ScriptExecutionContext*);
    virtual WebCore::NotificationClient::Permission checkPermission(const KURL&);

private:
    bool hasNotificationDelegate();
    COMPtr<IWebDesktopNotificationsDelegate> notificationDelegate();

    WebView* m_webView;
};

#endif
