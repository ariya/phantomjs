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

#import "config.h"
#import "WebFullScreenManagerProxy.h"

#if ENABLE(FULLSCREEN_API)

#import "LayerTreeContext.h"
#import "WKFullScreenWindowController.h"
#import "WKViewInternal.h"
#import "WebFullScreenManagerProxyMessages.h"
#import "WebPageProxy.h"
#import "WebProcessProxy.h"
#import <WebCore/IntRect.h>

using namespace WebCore;

namespace WebKit {

void WebFullScreenManagerProxy::invalidate()
{
    m_page->process()->removeMessageReceiver(Messages::WebFullScreenManagerProxy::messageReceiverName(), m_page->pageID());

    if (!m_webView)
        return;
    
    [m_webView closeFullScreenWindowController];
    m_webView = 0;
}

void WebFullScreenManagerProxy::close()
{
    if (!m_webView)
        return;
    [[m_webView fullScreenWindowController] close];
}

bool WebFullScreenManagerProxy::isFullScreen()
{
    if (!m_webView)
        return false;
    if (![m_webView hasFullScreenWindowController])
        return false;

    return [[m_webView fullScreenWindowController] isFullScreen];
}

void WebFullScreenManagerProxy::enterFullScreen()
{
    if (!m_webView)
        return;
    [[m_webView fullScreenWindowController] enterFullScreen:nil];
}

void WebFullScreenManagerProxy::exitFullScreen()
{
    if (!m_webView)
        return;
    [[m_webView fullScreenWindowController] exitFullScreen];
}
    
void WebFullScreenManagerProxy::beganEnterFullScreen(const IntRect& initialFrame, const IntRect& finalFrame)
{
    if (m_webView)
        [[m_webView fullScreenWindowController] beganEnterFullScreenWithInitialFrame:initialFrame finalFrame:finalFrame];
}

void WebFullScreenManagerProxy::beganExitFullScreen(const IntRect& initialFrame, const IntRect& finalFrame)
{
    if (m_webView)
        [[m_webView fullScreenWindowController] beganExitFullScreenWithInitialFrame:initialFrame finalFrame:finalFrame];
}

} // namespace WebKit

#endif
