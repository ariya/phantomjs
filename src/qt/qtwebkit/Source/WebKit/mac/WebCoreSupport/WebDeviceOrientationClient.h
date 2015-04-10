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

#import "WebDeviceOrientationProvider.h"
#import <WebCore/DeviceOrientationClient.h>

namespace WebCore {
class DeviceOrientationController;
}

@class WebView;

// This class is the Mac implementation of DeviceOrientationClient. It is
// passed to the Page constructor by the WebView. It is a simple proxy to
// either the real or mock client which is passed to the WebView. It is
// required because the WebView must pass a client to the Page constructor,
// but the real or mock client can not be specified until after the Page has
// been constructed.
class WebDeviceOrientationClient : public WebCore::DeviceOrientationClient {
public:
    WebDeviceOrientationClient(WebView*);

    // DeviceOrientationClient methods
    virtual void setController(WebCore::DeviceOrientationController*) OVERRIDE;
    virtual void startUpdating() OVERRIDE;
    virtual void stopUpdating() OVERRIDE;
    virtual WebCore::DeviceOrientationData* lastOrientation() const OVERRIDE;
    virtual void deviceOrientationControllerDestroyed() OVERRIDE;

private:
    id<WebDeviceOrientationProvider> getProvider() const;

    WebView* m_webView;
    WebCore::DeviceOrientationController* m_controller;
    mutable id<WebDeviceOrientationProvider> m_provider;
};
