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

#if ENABLE(FULLSCREEN_API)

#include "InjectedBundlePageFullScreenClient.h"

#include "InjectedBundleNodeHandle.h"
#include "WKAPICast.h"
#include "WKBundleAPICast.h"
#include "WKSharedAPICast.h"
#include "WebCoreArgumentCoders.h"
#include "WebFullScreenManagerProxyMessages.h"
#include "WebPage.h"
#include <WebCore/Element.h>

using namespace WebCore;

namespace WebKit {

bool InjectedBundlePageFullScreenClient::supportsFullScreen(WebPage *page, bool withKeyboard)
{
    if (m_client.supportsFullScreen) 
        return m_client.supportsFullScreen(toAPI(page), withKeyboard);

    bool supports = true;
    page->sendSync(Messages::WebFullScreenManagerProxy::SupportsFullScreen(withKeyboard), supports);
    return supports;
}

void InjectedBundlePageFullScreenClient::enterFullScreenForElement(WebPage *page, WebCore::Element *element)
{
    if (m_client.enterFullScreenForElement) {
        RefPtr<InjectedBundleNodeHandle> nodeHandle = InjectedBundleNodeHandle::getOrCreate(element);
        m_client.enterFullScreenForElement(toAPI(page), toAPI(nodeHandle.get()));
    } else
        page->send(Messages::WebFullScreenManagerProxy::EnterFullScreen());
}

void InjectedBundlePageFullScreenClient::exitFullScreenForElement(WebPage *page, WebCore::Element *element)
{
    if (m_client.exitFullScreenForElement) {
        RefPtr<InjectedBundleNodeHandle> nodeHandle = InjectedBundleNodeHandle::getOrCreate(element);
        m_client.exitFullScreenForElement(toAPI(page), toAPI(nodeHandle.get()));
    } else
        page->send(Messages::WebFullScreenManagerProxy::ExitFullScreen());
}


void InjectedBundlePageFullScreenClient::beganEnterFullScreen(WebPage *page, IntRect& initialFrame, IntRect& finalFrame)
{
    if (m_client.beganEnterFullScreen)
        m_client.beganEnterFullScreen(toAPI(page), toAPI(initialFrame), toAPI(finalFrame));
    else
        page->send(Messages::WebFullScreenManagerProxy::BeganEnterFullScreen(initialFrame, finalFrame));
}


void InjectedBundlePageFullScreenClient::beganExitFullScreen(WebPage *page, IntRect& initialFrame, IntRect& finalFrame)
{
    if (m_client.beganExitFullScreen)
        m_client.beganExitFullScreen(toAPI(page), toAPI(initialFrame), toAPI(finalFrame));
    else
        page->send(Messages::WebFullScreenManagerProxy::BeganExitFullScreen(initialFrame, finalFrame));
}

void InjectedBundlePageFullScreenClient::closeFullScreen(WebPage *page)
{
    if (m_client.closeFullScreen)
        m_client.closeFullScreen(toAPI(page));
    else
        page->send(Messages::WebFullScreenManagerProxy::Close());
}

} // namespace WebKit

#endif // ENABLE(FULLSCREEN_API)
