/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "WebKitDLL.h"
#include "WebGeolocationClient.h"

#include "WebFrame.h"
#include "WebGeolocationPolicyListener.h"
#include "WebGeolocationPosition.h"
#include "WebSecurityOrigin.h"
#include "WebView.h"
#include <WebCore/Document.h>
#include <WebCore/Frame.h>
#include <WebCore/Geolocation.h>
#include <WebCore/SecurityOrigin.h>

using namespace WebCore;

WebGeolocationClient::WebGeolocationClient(WebView* webView)
    : m_webView(webView)
{
}

void WebGeolocationClient::geolocationDestroyed()
{
    delete this;
}

void WebGeolocationClient::startUpdating()
{
    COMPtr<IWebGeolocationProvider> provider;
    if (FAILED(m_webView->geolocationProvider(&provider)))
        return;
    provider->registerWebView(m_webView.get());
}

void WebGeolocationClient::stopUpdating()
{
    COMPtr<IWebGeolocationProvider> provider;
    if (FAILED(m_webView->geolocationProvider(&provider)))
        return;
    provider->unregisterWebView(m_webView.get());
}

GeolocationPosition* WebGeolocationClient::lastPosition()
{
    COMPtr<IWebGeolocationProvider> provider;
    if (FAILED(m_webView->geolocationProvider(&provider)))
        return 0;
    COMPtr<IWebGeolocationPosition> position;
    if (FAILED(provider->lastPosition(&position)))
        return 0;
    return core(position.get());
}

void WebGeolocationClient::requestPermission(Geolocation* geolocation)
{
    COMPtr<IWebUIDelegate> uiDelegate;
    if (FAILED(m_webView->uiDelegate(&uiDelegate))) {
        geolocation->setIsAllowed(false);
        return;
    }

    COMPtr<IWebUIDelegatePrivate2> uiDelegatePrivate2(Query, uiDelegate);
    if (!uiDelegatePrivate2) {
        geolocation->setIsAllowed(false);
        return;
    }

    Frame* frame = geolocation->frame();
    COMPtr<WebSecurityOrigin> origin(AdoptCOM, WebSecurityOrigin::createInstance(frame->document()->securityOrigin()));
    COMPtr<WebGeolocationPolicyListener> listener = WebGeolocationPolicyListener::createInstance(geolocation);
    HRESULT hr = uiDelegatePrivate2->decidePolicyForGeolocationRequest(m_webView.get(), kit(frame), origin.get(), listener.get());
    if (hr != E_NOTIMPL)
        return;

    geolocation->setIsAllowed(false);
}
