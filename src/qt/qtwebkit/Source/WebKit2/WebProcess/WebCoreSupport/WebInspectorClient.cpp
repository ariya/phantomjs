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
#include "WebInspectorClient.h"

#if ENABLE(INSPECTOR)

#include "WebInspector.h"
#include "WebPage.h"
#include <WebCore/InspectorController.h>
#include <WebCore/Page.h>

using namespace WebCore;

namespace WebKit {

void WebInspectorClient::inspectorDestroyed()
{
    closeInspectorFrontend();
    delete this;
}

WebCore::InspectorFrontendChannel* WebInspectorClient::openInspectorFrontend(InspectorController*)
{
    WebPage* inspectorPage = m_page->inspector()->createInspectorPage();
    ASSERT_UNUSED(inspectorPage, inspectorPage);
    return this;
}

void WebInspectorClient::closeInspectorFrontend()
{
    if (m_page->inspector())
        m_page->inspector()->didClose();
}

void WebInspectorClient::bringFrontendToFront()
{
    m_page->inspector()->bringToFront();
}

void WebInspectorClient::didResizeMainFrame(Frame*)
{
    if (m_page->inspector())
        m_page->inspector()->updateDockingAvailability();
}

void WebInspectorClient::highlight()
{
    if (!m_highlightOverlay) {
        RefPtr<PageOverlay> highlightOverlay = PageOverlay::create(this);
        m_highlightOverlay = highlightOverlay.get();
        m_page->installPageOverlay(highlightOverlay.release(), true);
        m_highlightOverlay->setNeedsDisplay();
    } else {
        m_highlightOverlay->stopFadeOutAnimation();
        m_highlightOverlay->setNeedsDisplay();
    }
}

void WebInspectorClient::hideHighlight()
{
    if (m_highlightOverlay)
        m_page->uninstallPageOverlay(m_highlightOverlay, true);
}

bool WebInspectorClient::sendMessageToFrontend(const String& message)
{
    WebInspector* inspector = m_page->inspector();
    if (!inspector)
        return false;

#if ENABLE(INSPECTOR_SERVER)
    if (inspector->hasRemoteFrontendConnected()) {
        inspector->sendMessageToRemoteFrontend(message);
        return true;
    }
#endif

    WebPage* inspectorPage = inspector->inspectorPage();
    if (inspectorPage)
        return doDispatchMessageOnFrontendPage(inspectorPage->corePage(), message);

    return false;
}

bool WebInspectorClient::supportsFrameInstrumentation()
{
#if USE(COORDINATED_GRAPHICS)
    return true;
#endif
    return false;
}

void WebInspectorClient::pageOverlayDestroyed(PageOverlay*)
{
}

void WebInspectorClient::willMoveToWebPage(PageOverlay*, WebPage* webPage)
{
    if (webPage)
        return;

    // The page overlay is moving away from the web page, reset it.
    ASSERT(m_highlightOverlay);
    m_highlightOverlay = 0;
}

void WebInspectorClient::didMoveToWebPage(PageOverlay*, WebPage*)
{
}

void WebInspectorClient::drawRect(PageOverlay*, WebCore::GraphicsContext& context, const WebCore::IntRect& /*dirtyRect*/)
{
    m_page->corePage()->inspectorController()->drawHighlight(context);
}

bool WebInspectorClient::mouseEvent(PageOverlay*, const WebMouseEvent&)
{
    return false;
}

} // namespace WebKit

#endif // ENABLE(INSPECTOR)
