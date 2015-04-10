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

#import "WebNotificationClient.h"

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#import "WebDelegateImplementationCaching.h"
#import "WebNotificationInternal.h"
#import "WebPreferencesPrivate.h"
#import "WebSecurityOriginInternal.h"
#import "WebUIDelegatePrivate.h"
#import "WebViewInternal.h"
#import <WebCore/BlockExceptions.h>
#import <WebCore/Page.h>
#import <WebCore/ScriptExecutionContext.h>
#endif

#if ENABLE(NOTIFICATIONS)
#import <WebCore/NotificationPermissionCallback.h>
#endif
#if ENABLE(LEGACY_NOTIFICATIONS)
#import <WebCore/VoidCallback.h>
#endif

using namespace WebCore;

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
@interface WebNotificationPolicyListener : NSObject <WebAllowDenyPolicyListener>
{
#if ENABLE(NOTIFICATIONS)
    RefPtr<NotificationPermissionCallback> _callback;
#endif
#if ENABLE(LEGACY_NOTIFICATIONS)
    RefPtr<VoidCallback> _voidCallback;
    bool _isLegacyRequest;
#endif
}
#if ENABLE(NOTIFICATIONS)
- (id)initWithCallback:(PassRefPtr<NotificationPermissionCallback>)callback;
#endif
#if ENABLE(LEGACY_NOTIFICATIONS)
- (id)initWithVoidCallback:(PassRefPtr<VoidCallback>)callback;
#endif

@end
#endif

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
static uint64_t generateNotificationID()
{
    static uint64_t uniqueNotificationID = 1;
    return uniqueNotificationID++;
}
#endif

WebNotificationClient::WebNotificationClient(WebView *webView)
    : m_webView(webView)
{
}

bool WebNotificationClient::show(Notification* notification)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    if (![m_webView _notificationProvider])
        return false;

    uint64_t notificationID = generateNotificationID();
    RetainPtr<WebNotification> webNotification = adoptNS([[WebNotification alloc] initWithCoreNotification:notification notificationID:notificationID]);
    m_notificationMap.set(notification, webNotification);

    NotificationContextMap::iterator it = m_notificationContextMap.add(notification->scriptExecutionContext(), Vector<RetainPtr<WebNotification> >()).iterator;
    it->value.append(webNotification);

    [[m_webView _notificationProvider] showNotification:webNotification.get() fromWebView:m_webView];
    return true;
#else
    UNUSED_PARAM(notification);
    return false;
#endif
}

void WebNotificationClient::cancel(Notification* notification)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    WebNotification *webNotification = m_notificationMap.get(notification).get();
    if (!webNotification)
        return;

    [[m_webView _notificationProvider] cancelNotification:webNotification];
#else
    UNUSED_PARAM(notification);
#endif
}

void WebNotificationClient::clearNotifications(ScriptExecutionContext* context)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    NotificationContextMap::iterator it = m_notificationContextMap.find(context);
    if (it == m_notificationContextMap.end())
        return;
    
    Vector<RetainPtr<WebNotification> >& webNotifications = it->value;
    NSMutableArray *nsIDs = [NSMutableArray array];
    size_t count = webNotifications.size();
    for (size_t i = 0; i < count; ++i) {
        WebNotification *webNotification = webNotifications[i].get();
        [nsIDs addObject:[NSNumber numberWithUnsignedLongLong:[webNotification notificationID]]];
        core(webNotification)->finalize();
        m_notificationMap.remove(core(webNotification));
    }

    [[m_webView _notificationProvider] clearNotifications:nsIDs];
    m_notificationContextMap.remove(it);
#else
    UNUSED_PARAM(context);
#endif
}

void WebNotificationClient::notificationObjectDestroyed(Notification* notification)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    RetainPtr<WebNotification> webNotification = m_notificationMap.take(notification);
    if (!webNotification)
        return;

    NotificationContextMap::iterator it = m_notificationContextMap.find(notification->scriptExecutionContext());
    ASSERT(it != m_notificationContextMap.end());
    size_t index = it->value.find(webNotification);
    ASSERT(index != notFound);
    it->value.remove(index);
    if (it->value.isEmpty())
        m_notificationContextMap.remove(it);

    [[m_webView _notificationProvider] notificationDestroyed:webNotification.get()];
#else
    UNUSED_PARAM(notification);
#endif
}

void WebNotificationClient::notificationControllerDestroyed()
{
    delete this;
}

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
void WebNotificationClient::requestPermission(ScriptExecutionContext* context, WebNotificationPolicyListener *listener)
{
    SEL selector = @selector(webView:decidePolicyForNotificationRequestFromOrigin:listener:);
    if (![[m_webView UIDelegate] respondsToSelector:selector])
        return;
    
    WebSecurityOrigin *webOrigin = [[WebSecurityOrigin alloc] _initWithWebCoreSecurityOrigin:context->securityOrigin()];
    
    CallUIDelegate(m_webView, selector, webOrigin, listener);
    
    [webOrigin release];
}
#endif

#if ENABLE(LEGACY_NOTIFICATIONS)
void WebNotificationClient::requestPermission(ScriptExecutionContext* context, PassRefPtr<VoidCallback> callback)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    WebNotificationPolicyListener *listener = [[WebNotificationPolicyListener alloc] initWithVoidCallback:callback];
    requestPermission(context, listener);
    [listener release];
    END_BLOCK_OBJC_EXCEPTIONS;
}
#endif

#if ENABLE(NOTIFICATIONS)
void WebNotificationClient::requestPermission(ScriptExecutionContext* context, PassRefPtr<NotificationPermissionCallback> callback)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    WebNotificationPolicyListener *listener = [[WebNotificationPolicyListener alloc] initWithCallback:callback];
    requestPermission(context, listener);
    [listener release];
    END_BLOCK_OBJC_EXCEPTIONS;
}
#endif

NotificationClient::Permission WebNotificationClient::checkPermission(ScriptExecutionContext* context)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    if (!context || !context->isDocument())
        return NotificationClient::PermissionDenied;
    if (![[m_webView preferences] notificationsEnabled])
        return NotificationClient::PermissionDenied;
    WebSecurityOrigin *webOrigin = [[WebSecurityOrigin alloc] _initWithWebCoreSecurityOrigin:context->securityOrigin()];
    WebNotificationPermission permission = [[m_webView _notificationProvider] policyForOrigin:webOrigin];
    [webOrigin release];
    switch (permission) {
        case WebNotificationPermissionAllowed:
            return NotificationClient::PermissionAllowed;
        case WebNotificationPermissionDenied:
            return NotificationClient::PermissionDenied;
        case WebNotificationPermissionNotAllowed:
            return NotificationClient::PermissionNotAllowed;
        default:
            return NotificationClient::PermissionNotAllowed;
    }
#else
    UNUSED_PARAM(context);
    return NotificationClient::PermissionDenied;
#endif
}

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
uint64_t WebNotificationClient::notificationIDForTesting(WebCore::Notification* notification)
{
    return [m_notificationMap.get(notification).get() notificationID];
}

@implementation WebNotificationPolicyListener

#if ENABLE(NOTIFICATIONS)
- (id)initWithCallback:(PassRefPtr<NotificationPermissionCallback>)callback
{
    if (!(self = [super init]))
        return nil;

    _callback = callback;
    return self;
}
#endif

#if ENABLE(LEGACY_NOTIFICATIONS)
- (id)initWithVoidCallback:(PassRefPtr<VoidCallback>)callback
{
    if (!(self = [super init]))
        return nil;

    _isLegacyRequest = true;
    _voidCallback = callback;
    return self;
}
#endif

- (void)allow
{
#if ENABLE(LEGACY_NOTIFICATIONS)
    if (_isLegacyRequest) {
        if (_voidCallback)
            _voidCallback->handleEvent();
        return;
    }
#endif
#if ENABLE(NOTIFICATIONS)
    if (_callback)
        _callback->handleEvent(Notification::permissionString(NotificationClient::PermissionAllowed));
#endif
}

- (void)deny
{
#if ENABLE(LEGACY_NOTIFICATIONS)
    if (_isLegacyRequest) {
        if (_voidCallback)
            _voidCallback->handleEvent();
        return;
    }
#endif
#if ENABLE(NOTIFICATIONS)
    if (_callback)
        _callback->handleEvent(Notification::permissionString(NotificationClient::PermissionDenied));
#endif
}

@end
#endif
