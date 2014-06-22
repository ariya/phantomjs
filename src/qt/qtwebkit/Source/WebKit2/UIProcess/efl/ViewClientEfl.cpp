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
#include "ViewClientEfl.h"

#include "EwkView.h"
#include "PageViewportController.h" 
#include "WebViewportAttributes.h"
#include <WebKit2/WKString.h>
#include <WebKit2/WKView.h>
#include <WebKit2/WKViewEfl.h>

using namespace EwkViewCallbacks;
using namespace WebCore;

namespace WebKit {

EwkView* ViewClientEfl::toEwkView(const void* clientInfo)
{
    return static_cast<ViewClientEfl*>(const_cast<void*>(clientInfo))->m_view;
}

void ViewClientEfl::viewNeedsDisplay(WKViewRef, WKRect, const void* clientInfo)
{
    toEwkView(clientInfo)->scheduleUpdateDisplay();
}

void ViewClientEfl::didChangeContentsSize(WKViewRef, WKSize size, const void* clientInfo)
{
    EwkView* ewkView = toEwkView(clientInfo);
    if (WKPageUseFixedLayout(ewkView->wkPage()))
#if USE(ACCELERATED_COMPOSITING)
        ewkView->pageViewportController()->didChangeContentsSize(toIntSize(size));
#else
        { }
#endif
    else
        ewkView->scheduleUpdateDisplay();

    ewkView->smartCallback<ContentsSizeChanged>().call(size);
}

void ViewClientEfl::webProcessCrashed(WKViewRef, WKURLRef url, const void* clientInfo)
{
    EwkView* ewkView = toEwkView(clientInfo);

    // Check if loading was ongoing, when web process crashed.
    double loadProgress = WKPageGetEstimatedProgress(ewkView->wkPage());
    if (loadProgress >= 0 && loadProgress < 1) {
        loadProgress = 1;
        ewkView->smartCallback<LoadProgress>().call(&loadProgress);
    }

    ewkView->smartCallback<TooltipTextUnset>().call();

    bool handled = false;
    ewkView->smartCallback<WebProcessCrashed>().call(&handled);

    if (!handled) {
        WKEinaSharedString urlString(url);
        WARN("WARNING: The web process experienced a crash on '%s'.\n", static_cast<const char*>(urlString));

        // Display an error page
        ewk_view_html_string_load(ewkView->evasObject(), "The web process has crashed.", 0, urlString);
    }
}

void ViewClientEfl::webProcessDidRelaunch(WKViewRef viewRef, const void* clientInfo)
{
    if (const char* themePath = toEwkView(clientInfo)->themePath())
        WKViewSetThemePath(viewRef, adoptWK(WKStringCreateWithUTF8CString(themePath)).get());
}

void ViewClientEfl::didChangeContentsPosition(WKViewRef, WKPoint position, const void* clientInfo)
{
    EwkView* ewkView = toEwkView(clientInfo);
    if (WKPageUseFixedLayout(ewkView->wkPage())) {
#if USE(ACCELERATED_COMPOSITING)
        ewkView->pageViewportController()->pageDidRequestScroll(toIntPoint(position));
#endif
        return;
    }

    ewkView->scheduleUpdateDisplay();
}

void ViewClientEfl::didRenderFrame(WKViewRef, WKSize contentsSize, WKRect coveredRect, const void* clientInfo)
{
    EwkView* ewkView = toEwkView(clientInfo);
    if (WKPageUseFixedLayout(ewkView->wkPage())) {
#if USE(ACCELERATED_COMPOSITING)
        ewkView->pageViewportController()->didRenderFrame(toIntSize(contentsSize), toIntRect(coveredRect));
#endif
        return;
    }

    ewkView->scheduleUpdateDisplay();
}

void ViewClientEfl::didCompletePageTransition(WKViewRef, const void* clientInfo)
{
    EwkView* ewkView = toEwkView(clientInfo);
    if (WKPageUseFixedLayout(ewkView->wkPage())) {
#if USE(ACCELERATED_COMPOSITING)
        ewkView->pageViewportController()->pageTransitionViewportReady();
#endif
        return;
    }

    ewkView->scheduleUpdateDisplay();
}

void ViewClientEfl::didChangeViewportAttributes(WKViewRef, WKViewportAttributesRef attributes, const void* clientInfo)
{
    EwkView* ewkView = toEwkView(clientInfo);
    if (WKPageUseFixedLayout(ewkView->wkPage())) {
#if USE(ACCELERATED_COMPOSITING)
        // FIXME: pageViewportController should accept WKViewportAttributesRef.
        ewkView->pageViewportController()->didChangeViewportAttributes(toImpl(attributes)->originalAttributes());
#endif
        return;
    }
    ewkView->scheduleUpdateDisplay();
}

void ViewClientEfl::didChangeTooltip(WKViewRef, WKStringRef tooltip, const void* clientInfo)
{
    if (WKStringIsEmpty(tooltip))
        toEwkView(clientInfo)->smartCallback<TooltipTextUnset>().call();
    else
        toEwkView(clientInfo)->smartCallback<TooltipTextSet>().call(WKEinaSharedString(tooltip));
}

void ViewClientEfl::didFindZoomableArea(WKViewRef, WKPoint point, WKRect area, const void* clientInfo)
{
    toEwkView(clientInfo)->didFindZoomableArea(point, area);
}

#if ENABLE(TOUCH_EVENTS)
void ViewClientEfl::doneWithTouchEvent(WKViewRef, WKTouchEventRef event, bool wasEventHandled, const void* clientInfo)
{
    toEwkView(clientInfo)->doneWithTouchEvent(event, wasEventHandled);
}
#endif

ViewClientEfl::ViewClientEfl(EwkView* view)
    : m_view(view)
{
    ASSERT(m_view);

    WKViewClient viewClient;
    memset(&viewClient, 0, sizeof(WKViewClient));
    viewClient.version = kWKViewClientCurrentVersion;
    viewClient.clientInfo = this;
    viewClient.didChangeContentsSize = didChangeContentsSize;
    viewClient.didFindZoomableArea = didFindZoomableArea;
    viewClient.viewNeedsDisplay = viewNeedsDisplay;
    viewClient.webProcessCrashed = webProcessCrashed;
    viewClient.webProcessDidRelaunch = webProcessDidRelaunch;
    viewClient.didChangeContentsPosition = didChangeContentsPosition;
    viewClient.didRenderFrame = didRenderFrame;
    viewClient.didCompletePageTransition = didCompletePageTransition;
    viewClient.didChangeViewportAttributes = didChangeViewportAttributes;
    viewClient.didChangeTooltip = didChangeTooltip;
#if ENABLE(TOUCH_EVENTS)
    viewClient.doneWithTouchEvent = doneWithTouchEvent;
#endif

    WKViewSetViewClient(m_view->wkView(), &viewClient);
}

ViewClientEfl::~ViewClientEfl()
{
    WKViewSetViewClient(m_view->wkView(), 0);
}

} // namespace WebKit
