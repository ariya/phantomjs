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

#ifndef WebViewClient_h
#define WebViewClient_h

#if USE(COORDINATED_GRAPHICS)

#include "APIClient.h"
#include "WKView.h"
#include <wtf/text/WTFString.h>

namespace WebCore {
class IntPoint;
class IntRect;
class IntSize;
class ViewportAttributes;
}

namespace WebKit {

class WebView;

#if ENABLE(TOUCH_EVENTS)
class NativeWebTouchEvent;
#endif

class WebViewClient: public APIClient<WKViewClient, kWKViewClientCurrentVersion> {
public:
    void viewNeedsDisplay(WebView*, const WebCore::IntRect&);
    void didChangeContentsSize(WebView*, const WebCore::IntSize&);
    void webProcessCrashed(WebView*, const String& url);
    void webProcessDidRelaunch(WebView*);
    void didChangeContentsPosition(WebView*, const WebCore::IntPoint&);
    void didRenderFrame(WebView*, const WebCore::IntSize&, const WebCore::IntRect&);
    void didCompletePageTransition(WebView*);
    void didChangeViewportAttributes(WebView*, const WebCore::ViewportAttributes&);
    void didChangeTooltip(WebView*, const String& tooltip);
    void didFindZoomableArea(WebView*, const WebCore::IntPoint&, const WebCore::IntRect&);
#if ENABLE(TOUCH_EVENTS)
    void doneWithTouchEvent(WebView*, const NativeWebTouchEvent&, bool);
#endif
};

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS)

#endif // WebViewClient_h
