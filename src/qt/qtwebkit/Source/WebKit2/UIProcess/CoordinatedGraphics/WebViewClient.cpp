/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(COORDINATED_GRAPHICS)

#include "WebViewClient.h"

#include "NotImplemented.h"
#include "WKAPICast.h"
#include "WKBase.h"
#include "WKRetainPtr.h"
#include "WebViewportAttributes.h"

#if ENABLE(TOUCH_EVENTS)
#include "NativeWebTouchEvent.h"
#endif

using namespace WebCore;

namespace WebKit {

void WebViewClient::viewNeedsDisplay(WebView* view, const IntRect& area)
{
    if (!m_client.viewNeedsDisplay)
        return;

    m_client.viewNeedsDisplay(toAPI(view), toAPI(area), m_client.clientInfo);
}

void WebViewClient::didChangeContentsSize(WebView* view, const IntSize& size)
{
    if (!m_client.didChangeContentsSize)
        return;

    m_client.didChangeContentsSize(toAPI(view), toAPI(size), m_client.clientInfo);
}

void WebViewClient::webProcessCrashed(WebView* view, const String& url)
{
    if (!m_client.webProcessCrashed)
        return;

    m_client.webProcessCrashed(toAPI(view), adoptWK(toCopiedURLAPI(url)).get(), m_client.clientInfo);
}

void WebViewClient::webProcessDidRelaunch(WebView* view)
{
    if (!m_client.webProcessDidRelaunch)
        return;

    m_client.webProcessDidRelaunch(toAPI(view), m_client.clientInfo);
}

void WebViewClient::didChangeContentsPosition(WebView* view, const WebCore::IntPoint& point)
{
    if (!m_client.didChangeContentsPosition)
        return;

    m_client.didChangeContentsPosition(toAPI(view), toAPI(point), m_client.clientInfo);
}

void WebViewClient::didRenderFrame(WebView* view, const WebCore::IntSize& size, const WebCore::IntRect& coveredRect)
{
    if (!m_client.didRenderFrame)
        return;

    m_client.didRenderFrame(toAPI(view), toAPI(size), toAPI(coveredRect), m_client.clientInfo);
}

void WebViewClient::didCompletePageTransition(WebView* view)
{
    if (!m_client.didCompletePageTransition)
        return;

    m_client.didCompletePageTransition(toAPI(view), m_client.clientInfo);
}

void WebViewClient::didChangeViewportAttributes(WebView* view, const ViewportAttributes& attributes)
{
    if (!m_client.didChangeViewportAttributes)
        return;

    WKRetainPtr<WKViewportAttributesRef> wkAttributes = adoptWK(toAPI(WebViewportAttributes::create(attributes).leakRef()));
    m_client.didChangeViewportAttributes(toAPI(view), wkAttributes.get(), m_client.clientInfo);
}

void WebViewClient::didChangeTooltip(WebView* view, const String& tooltip)
{
    if (!m_client.didChangeTooltip)
        return;

    m_client.didChangeTooltip(toAPI(view), adoptWK(toCopiedAPI(tooltip)).get(), m_client.clientInfo);
}

void WebViewClient::didFindZoomableArea(WebView* view, const IntPoint& target, const IntRect& area)
{
    if (!m_client.didFindZoomableArea)
        return;

    m_client.didFindZoomableArea(toAPI(view), toAPI(target), toAPI(area), m_client.clientInfo);
}

#if ENABLE(TOUCH_EVENTS)
void WebViewClient::doneWithTouchEvent(WebView* view, const NativeWebTouchEvent& event, bool wasEventHandled)
{
#if PLATFORM(EFL)
    if (!m_client.doneWithTouchEvent)
        return;

    m_client.doneWithTouchEvent(toAPI(view), toAPI(const_cast<EwkTouchEvent*>(event.nativeEvent())), wasEventHandled, m_client.clientInfo);
#else
    notImplemented();
    UNUSED_PARAM(view);
    UNUSED_PARAM(event);
    UNUSED_PARAM(wasEventHandled);
#endif
}
#endif

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS)
