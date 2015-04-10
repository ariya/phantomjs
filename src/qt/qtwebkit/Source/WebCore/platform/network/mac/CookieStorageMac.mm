/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "CookieStorage.h"

#import "ResourceHandle.h"

#import "WebCoreSystemInterface.h"
#import <wtf/RetainPtr.h>

using namespace WebCore;

@interface WebCookieStorageObjCAdapter : NSObject {
    CookieChangeCallbackPtr m_cookieChangeCallback;
}
-(void)notifyCookiesChangedOnMainThread;
-(void)cookiesChangedNotificationHandler:(NSNotification *)notification;
-(void)startListeningForCookieChangeNotificationsWithCallback:(CookieChangeCallbackPtr)callback;
-(void)stopListeningForCookieChangeNotifications;
@end

@implementation WebCookieStorageObjCAdapter

-(void)notifyCookiesChangedOnMainThread
{
    m_cookieChangeCallback();
}

-(void)cookiesChangedNotificationHandler:(NSNotification *)notification
{
    UNUSED_PARAM(notification);

    [self performSelectorOnMainThread:@selector(notifyCookiesChangedOnMainThread) withObject:nil waitUntilDone:FALSE];
}

-(void)startListeningForCookieChangeNotificationsWithCallback:(CookieChangeCallbackPtr)callback
{
    ASSERT(!m_cookieChangeCallback);
    m_cookieChangeCallback = callback;
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(cookiesChangedNotificationHandler:) name:NSHTTPCookieManagerCookiesChangedNotification object:[NSHTTPCookieStorage sharedHTTPCookieStorage]];
}

-(void)stopListeningForCookieChangeNotifications
{
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSHTTPCookieManagerCookiesChangedNotification object:nil];
    m_cookieChangeCallback = 0;
}

@end

namespace WebCore {

static WebCookieStorageObjCAdapter *cookieStorageAdapter;

void startObservingCookieChanges(CookieChangeCallbackPtr callback)
{
    if (!cookieStorageAdapter)
        cookieStorageAdapter = [[WebCookieStorageObjCAdapter alloc] init];
    [cookieStorageAdapter startListeningForCookieChangeNotificationsWithCallback:callback];
}

void stopObservingCookieChanges()
{
    // cookieStorageAdapter can be nil here, if the WebProcess crashed and was restarted between
    // when startObservingCookieChanges was called, and stopObservingCookieChanges is currently being called.
    if (!cookieStorageAdapter)
        return;
    [cookieStorageAdapter stopListeningForCookieChangeNotifications];
}

}
