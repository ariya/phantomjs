/*
 * Copyright (C) 2012 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebFullScreenManagerProxy.h"
#include "WebFullScreenManagerProxyMessages.h"

#if ENABLE(FULLSCREEN_API)

#include "EwkView.h"
#include <WebCore/NotImplemented.h>

using namespace WebCore;

namespace WebKit {

void WebFullScreenManagerProxy::invalidate()
{
    m_page->process()->removeMessageReceiver(Messages::WebFullScreenManagerProxy::messageReceiverName(), m_page->pageID());
    m_webView = 0;
}

void WebFullScreenManagerProxy::close()
{
    notImplemented();
}

bool WebFullScreenManagerProxy::isFullScreen()
{
    return m_hasRequestedFullScreen;
}

void WebFullScreenManagerProxy::enterFullScreen()
{
    if (!m_webView || m_hasRequestedFullScreen)
        return;

    m_hasRequestedFullScreen = true;

    willEnterFullScreen();
    toEwkView(m_webView)->enterFullScreen();
    didEnterFullScreen();
}

void WebFullScreenManagerProxy::exitFullScreen()
{
    if (!m_webView || !m_hasRequestedFullScreen)
        return;

    m_hasRequestedFullScreen = false;

    willExitFullScreen();
    toEwkView(m_webView)->exitFullScreen();
    didExitFullScreen();
}

void WebFullScreenManagerProxy::beganEnterFullScreen(const IntRect& /*initialFrame*/, const IntRect& /*finalFrame*/)
{
    notImplemented();
}

void WebFullScreenManagerProxy::beganExitFullScreen(const IntRect& /*initialFrame*/, const IntRect& /*finalFrame*/)
{
    notImplemented();
}

} // namespace WebKit

#endif // ENABLE(FULLSCREEN_API)
