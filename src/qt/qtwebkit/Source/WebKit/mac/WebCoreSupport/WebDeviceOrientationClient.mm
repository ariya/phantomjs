/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "WebDeviceOrientationClient.h"

#import "WebDeviceOrientationInternal.h"
#import "WebDeviceOrientationProvider.h"
#import "WebViewInternal.h"
#import <wtf/ObjcRuntimeExtras.h>

using namespace WebCore;

WebDeviceOrientationClient::WebDeviceOrientationClient(WebView* webView)
    : m_webView(webView)
    , m_controller(0)
{
}

void WebDeviceOrientationClient::setController(DeviceOrientationController* controller)
{
    // This is called by the Page constructor before our WebView has the provider set up.
    // MemoryCache the controller for later use.
    m_controller = controller;
}

void WebDeviceOrientationClient::startUpdating()
{
    [getProvider() startUpdating];
}

void WebDeviceOrientationClient::stopUpdating()
{
    [getProvider() stopUpdating];
}

DeviceOrientationData* WebDeviceOrientationClient::lastOrientation() const
{
    return core([getProvider() lastOrientation]);
}

void WebDeviceOrientationClient::deviceOrientationControllerDestroyed()
{
      delete this;
}

id<WebDeviceOrientationProvider> WebDeviceOrientationClient::getProvider() const
{
    if (!m_provider) {
        m_provider = [m_webView _deviceOrientationProvider];
        if ([m_provider respondsToSelector:@selector(setController:)])
            wtfObjcMsgSend<void>(m_provider, @selector(setController:), m_controller);
    }
    return m_provider;
}
