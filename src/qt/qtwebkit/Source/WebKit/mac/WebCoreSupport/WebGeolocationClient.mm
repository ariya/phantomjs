/*
 * Copyright (C) 2009, 2012 Apple Inc. All rights reserved.
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

#import "WebGeolocationClient.h"

#if ENABLE(GEOLOCATION)

#import "WebDelegateImplementationCaching.h"
#import "WebFrameInternal.h"
#import "WebGeolocationPositionInternal.h"
#import "WebSecurityOriginInternal.h"
#import "WebUIDelegatePrivate.h"
#import "WebViewInternal.h"
#import <WebCore/BlockExceptions.h>
#import <WebCore/Document.h>
#import <WebCore/Frame.h>
#import <WebCore/Geolocation.h>

using namespace WebCore;

@interface WebGeolocationPolicyListener : NSObject <WebAllowDenyPolicyListener>
{
    RefPtr<Geolocation> _geolocation;
}
- (id)initWithGeolocation:(Geolocation*)geolocation;
@end

WebGeolocationClient::WebGeolocationClient(WebView *webView)
    : m_webView(webView)
{
}

void WebGeolocationClient::geolocationDestroyed()
{
    delete this;
}

void WebGeolocationClient::startUpdating()
{
    [[m_webView _geolocationProvider] registerWebView:m_webView];
}

void WebGeolocationClient::stopUpdating()
{
    [[m_webView _geolocationProvider] unregisterWebView:m_webView];
}

void WebGeolocationClient::requestPermission(Geolocation* geolocation)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    SEL selector = @selector(webView:decidePolicyForGeolocationRequestFromOrigin:frame:listener:);
    if (![[m_webView UIDelegate] respondsToSelector:selector]) {
        geolocation->setIsAllowed(false);
        return;
    }

    Frame *frame = geolocation->frame();
    WebSecurityOrigin *webOrigin = [[WebSecurityOrigin alloc] _initWithWebCoreSecurityOrigin:frame->document()->securityOrigin()];
    WebGeolocationPolicyListener* listener = [[WebGeolocationPolicyListener alloc] initWithGeolocation:geolocation];

    CallUIDelegate(m_webView, selector, webOrigin, kit(frame), listener);

    [webOrigin release];
    [listener release];

    END_BLOCK_OBJC_EXCEPTIONS;
}

GeolocationPosition* WebGeolocationClient::lastPosition()
{
    return core([[m_webView _geolocationProvider] lastPosition]);
}

@implementation WebGeolocationPolicyListener

- (id)initWithGeolocation:(Geolocation*)geolocation
{
    if (!(self = [super init]))
        return nil;
    _geolocation = geolocation;
    return self;
}

- (void)allow
{
    _geolocation->setIsAllowed(true);
}

- (void)deny
{
    _geolocation->setIsAllowed(false);
}

@end

#endif // ENABLE(GEOLOCATION)
