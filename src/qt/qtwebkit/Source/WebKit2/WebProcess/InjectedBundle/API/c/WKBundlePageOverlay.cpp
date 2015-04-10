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
#include "WKBundlePageOverlay.h"

#include "PageOverlay.h"
#include "WKAPICast.h"
#include "WKBundleAPICast.h"
#include <WebCore/GraphicsContext.h>
#include <wtf/PassOwnPtr.h>

using namespace WebCore;
using namespace WebKit;

class PageOverlayClientImpl : public PageOverlay::Client {
public:
    static PassOwnPtr<PageOverlayClientImpl> create(WKBundlePageOverlayClient* client)
    {
        return adoptPtr(new PageOverlayClientImpl(client));
    }

private:
    explicit PageOverlayClientImpl(WKBundlePageOverlayClient* client)
        : m_client()
    {
        if (client)
            m_client = *client;
    }

    // PageOverlay::Client.
    virtual void pageOverlayDestroyed(PageOverlay*)
    {
        delete this;
    }

    virtual void willMoveToWebPage(PageOverlay* pageOverlay, WebPage* page)
    {
        if (!m_client.willMoveToPage)
            return;

        m_client.willMoveToPage(toAPI(pageOverlay), toAPI(page), m_client.clientInfo);
    }
    
    virtual void didMoveToWebPage(PageOverlay* pageOverlay, WebPage* page)
    {
        if (!m_client.didMoveToPage)
            return;

        m_client.didMoveToPage(toAPI(pageOverlay), toAPI(page), m_client.clientInfo);
    }

    virtual void drawRect(PageOverlay* pageOverlay, GraphicsContext& graphicsContext, const IntRect& dirtyRect)
    {
        if (!m_client.drawRect)
            return;

        m_client.drawRect(toAPI(pageOverlay), graphicsContext.platformContext(), toAPI(dirtyRect), m_client.clientInfo);
    }
    
    virtual bool mouseEvent(PageOverlay* pageOverlay, const WebMouseEvent& event)
    {
        switch (event.type()) {
        case WebEvent::MouseDown: {
            if (!m_client.mouseDown)
                return false;

            return m_client.mouseDown(toAPI(pageOverlay), toAPI(event.position()), toAPI(event.button()), m_client.clientInfo);
        }
        case WebEvent::MouseUp: {
            if (!m_client.mouseUp)
                return false;

            return m_client.mouseUp(toAPI(pageOverlay), toAPI(event.position()), toAPI(event.button()), m_client.clientInfo);
        }
        case WebEvent::MouseMove: {
            if (event.button() == WebMouseEvent::NoButton) {
                if (!m_client.mouseMoved)
                    return false;

                return m_client.mouseMoved(toAPI(pageOverlay), toAPI(event.position()), m_client.clientInfo);
            }

            // This is a MouseMove event with a mouse button pressed. Call mouseDragged.
            if (!m_client.mouseDragged)
                return false;

            return m_client.mouseDragged(toAPI(pageOverlay), toAPI(event.position()), toAPI(event.button()), m_client.clientInfo);
        }

        default:
            return false;
        }
    }
    
    WKBundlePageOverlayClient m_client;
};

WKTypeID WKBundlePageOverlayGetTypeID()
{
    return toAPI(PageOverlay::APIType);
}

WKBundlePageOverlayRef WKBundlePageOverlayCreate(WKBundlePageOverlayClient* wkClient)
{
    if (wkClient && wkClient->version)
        return 0;

    OwnPtr<PageOverlayClientImpl> clientImpl = PageOverlayClientImpl::create(wkClient);

    return toAPI(PageOverlay::create(clientImpl.leakPtr()).leakRef());
}

void WKBundlePageOverlaySetNeedsDisplay(WKBundlePageOverlayRef bundlePageOverlayRef, WKRect rect)
{
    toImpl(bundlePageOverlayRef)->setNeedsDisplay(enclosingIntRect(toFloatRect(rect)));
}

float WKBundlePageOverlayFractionFadedIn(WKBundlePageOverlayRef bundlePageOverlayRef)
{
    return toImpl(bundlePageOverlayRef)->fractionFadedIn();
}
