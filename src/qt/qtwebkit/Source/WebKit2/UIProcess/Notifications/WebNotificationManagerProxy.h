/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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

#ifndef WebNotificationManagerProxy_h
#define WebNotificationManagerProxy_h

#include "APIObject.h"
#include "MessageReceiver.h"
#include "WebContextSupplement.h"
#include "WebNotificationProvider.h"
#include <WebCore/NotificationClient.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/StringHash.h>

namespace WebKit {

class ImmutableArray;
class WebContext;
class WebPageProxy;
class WebSecurityOrigin;

class WebNotificationManagerProxy : public TypedAPIObject<APIObject::TypeNotificationManager>, public WebContextSupplement {
public:

    static const char* supplementName();

    static PassRefPtr<WebNotificationManagerProxy> create(WebContext*);

    void initializeProvider(const WKNotificationProvider*);
    void populateCopyOfNotificationPermissions(HashMap<String, bool>&);

    void show(WebPageProxy*, const String& title, const String& body, const String& iconURL, const String& tag, const String& lang, const String& dir, const String& originString, uint64_t pageNotificationID);
    void cancel(WebPageProxy*, uint64_t pageNotificationID);
    void clearNotifications(WebPageProxy*);
    void clearNotifications(WebPageProxy*, const Vector<uint64_t>& pageNotificationIDs);
    void didDestroyNotification(WebPageProxy*, uint64_t pageNotificationID);

    void providerDidShowNotification(uint64_t notificationID);
    void providerDidClickNotification(uint64_t notificationID);
    void providerDidCloseNotifications(ImmutableArray* notificationIDs);
    void providerDidUpdateNotificationPolicy(const WebSecurityOrigin*, bool allowed);
    void providerDidRemoveNotificationPolicies(ImmutableArray* origins);

    using APIObject::ref;
    using APIObject::deref;

private:
    explicit WebNotificationManagerProxy(WebContext*);

    typedef bool (*NotificationFilterFunction)(uint64_t pageID, uint64_t pageNotificationID, uint64_t desiredPageID, const Vector<uint64_t>& desiredPageNotificationIDs);
    void clearNotifications(WebPageProxy*, const Vector<uint64_t>& pageNotificationIDs, NotificationFilterFunction);

    // WebContextSupplement
    virtual void contextDestroyed() OVERRIDE;
    virtual void refWebContextSupplement() OVERRIDE;
    virtual void derefWebContextSupplement() OVERRIDE;

    WebNotificationProvider m_provider;
    // Pair comprised of web page ID and the web process's notification ID
    HashMap<uint64_t, pair<uint64_t, uint64_t> > m_globalNotificationMap;
    // Key pair comprised of web page ID and the web process's notification ID; value pair comprised of global notification ID, and notification object
    HashMap<pair<uint64_t, uint64_t>, pair<uint64_t, RefPtr<WebNotification> > > m_notifications;
};

} // namespace WebKit

#endif // WebNotificationManagerProxy_h
