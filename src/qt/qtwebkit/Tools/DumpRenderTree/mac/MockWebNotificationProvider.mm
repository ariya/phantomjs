/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "MockWebNotificationProvider.h"

#import <WebKit/WebSecurityOriginPrivate.h>

@implementation MockWebNotificationProvider

+ (MockWebNotificationProvider *)shared
{
    static MockWebNotificationProvider *provider = [[MockWebNotificationProvider alloc] init];
    return provider;
}

- (id)init
{
    if (!(self = [super init]))
        return nil;
    _permissions = adoptNS([[NSMutableDictionary alloc] init]);
    return self;
}

- (void)registerWebView:(WebView *)webView
{
    ASSERT(!_registeredWebViews.contains(webView));
    _registeredWebViews.add(webView);
}

- (void)unregisterWebView:(WebView *)webView
{
    ASSERT(_registeredWebViews.contains(webView));
    _registeredWebViews.remove(webView);
}

- (void)showNotification:(WebNotification *)notification fromWebView:(WebView *)webView
{
    ASSERT(_registeredWebViews.contains(webView));

    uint64_t notificationID = [notification notificationID];
    _notifications.add(notificationID, notification);
    _notificationViewMap.add(notificationID, webView);

    [webView _notificationDidShow:notificationID];
}

- (void)cancelNotification:(WebNotification *)notification
{
    uint64_t notificationID = [notification notificationID];
    ASSERT(_notifications.contains(notificationID));

    [_notificationViewMap.get(notificationID) _notificationsDidClose:[NSArray arrayWithObject:[NSNumber numberWithUnsignedLongLong:notificationID]]];
}

- (void)notificationDestroyed:(WebNotification *)notification
{
    _notifications.remove([notification notificationID]);
    _notificationViewMap.remove([notification notificationID]);
}

- (void)clearNotifications:(NSArray *)notificationIDs
{
    for (NSNumber *notificationID in notificationIDs) {
        uint64_t id = [notificationID unsignedLongLongValue];
        RetainPtr<WebNotification> notification = _notifications.take(id);
        _notificationViewMap.remove(id);
    }
}

- (void)webView:(WebView *)webView didShowNotification:(uint64_t)notificationID
{
    [_notifications.get(notificationID).get() dispatchShowEvent];
}

- (void)webView:(WebView *)webView didClickNotification:(uint64_t)notificationID
{
    [_notifications.get(notificationID).get() dispatchClickEvent];
}

- (void)webView:(WebView *)webView didCloseNotifications:(NSArray *)notificationIDs
{
    for (NSNumber *notificationID in notificationIDs) {
        uint64_t id = [notificationID unsignedLongLongValue];
        NotificationIDMap::iterator it = _notifications.find(id);
        ASSERT(it != _notifications.end());
        [it->value.get() dispatchCloseEvent];
        _notifications.remove(it);
        _notificationViewMap.remove(id);
    }
}

- (void)simulateWebNotificationClick:(uint64_t)notificationID
{
    ASSERT(_notifications.contains(notificationID));
    [_notificationViewMap.get(notificationID) _notificationDidClick:notificationID];
}

- (WebNotificationPermission)policyForOrigin:(WebSecurityOrigin *)origin
{
    NSNumber *permission = [_permissions.get() objectForKey:[origin stringValue]];
    if (!permission)
        return WebNotificationPermissionNotAllowed;
    if ([permission boolValue])
        return WebNotificationPermissionAllowed;
    return WebNotificationPermissionDenied;
}

- (void)setWebNotificationOrigin:(NSString *)origin permission:(BOOL)allowed
{
    [_permissions.get() setObject:[NSNumber numberWithBool:allowed] forKey:origin];
}

- (void)removeAllWebNotificationPermissions
{
    [_permissions.get() removeAllObjects];
}

- (void)reset
{
    _notifications.clear();
    _notificationViewMap.clear();
    [self removeAllWebNotificationPermissions];
}

@end
