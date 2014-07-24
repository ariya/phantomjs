/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#import "WebStorageTrackerClient.h"

#import "WebSecurityOriginInternal.h"
#import "WebStorageManagerPrivate.h"
#import <WebCore/SecurityOrigin.h>
#import <wtf/MainThread.h>
#import <wtf/RetainPtr.h>
#import <wtf/text/WTFString.h>

using namespace WebCore;

WebStorageTrackerClient* WebStorageTrackerClient::sharedWebStorageTrackerClient()
{
    static WebStorageTrackerClient* sharedClient = new WebStorageTrackerClient();
    return sharedClient;
}

WebStorageTrackerClient::WebStorageTrackerClient()
{
}

WebStorageTrackerClient::~WebStorageTrackerClient()
{
}

void WebStorageTrackerClient::dispatchDidModifyOriginOnMainThread(void* context)
{
    ASSERT(isMainThread());
    // adoptRef is balanced by leakRef in dispatchDidModifyOrigin.
    RefPtr<SecurityOrigin> origin = adoptRef(static_cast<SecurityOrigin*>(context));
    WebStorageTrackerClient::sharedWebStorageTrackerClient()->dispatchDidModifyOrigin(origin.get());
}

void WebStorageTrackerClient::dispatchDidModifyOrigin(PassRefPtr<SecurityOrigin> origin)
{
    RetainPtr<WebSecurityOrigin> webSecurityOrigin = adoptNS([[WebSecurityOrigin alloc] _initWithWebCoreSecurityOrigin:origin.get()]);

    [[NSNotificationCenter defaultCenter] postNotificationName:WebStorageDidModifyOriginNotification 
                                                        object:webSecurityOrigin.get()];
}

void WebStorageTrackerClient::dispatchDidModifyOrigin(const String& originIdentifier)
{
    PassRefPtr<SecurityOrigin> origin = SecurityOrigin::createFromDatabaseIdentifier(originIdentifier);

    if (!isMainThread()) {
        // leakRef is balanced by adoptRef in dispatchDidModifyOriginOnMainThread.
        callOnMainThread(dispatchDidModifyOriginOnMainThread, origin.leakRef());
        return;
    }

    dispatchDidModifyOrigin(origin);
}

void WebStorageTrackerClient::didFinishLoadingOrigins()
{
}
