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

#ifndef ViewClientEfl_h
#define ViewClientEfl_h

#include <WebKit2/WKBase.h>
#include <WebKit2/WKGeometry.h>
#include <wtf/PassOwnPtr.h>

class EwkView;

namespace WebKit {

class ViewClientEfl {
public:
    static PassOwnPtr<ViewClientEfl> create(EwkView* view)
    {
        return adoptPtr(new ViewClientEfl(view));
    }

    ~ViewClientEfl();

private:
    explicit ViewClientEfl(EwkView*);

    static EwkView* toEwkView(const void* clientInfo);
    static void viewNeedsDisplay(WKViewRef, WKRect area, const void* clientInfo);
    static void didChangeContentsSize(WKViewRef, WKSize, const void* clientInfo);
    static void webProcessCrashed(WKViewRef, WKURLRef, const void* clientInfo);
    static void webProcessDidRelaunch(WKViewRef, const void* clientInfo);
    static void didChangeContentsPosition(WKViewRef, WKPoint, const void* clientInfo);
    static void didRenderFrame(WKViewRef, WKSize, WKRect, const void* clientInfo);
    static void didCompletePageTransition(WKViewRef, const void* clientInfo);
    static void didChangeViewportAttributes(WKViewRef, WKViewportAttributesRef, const void* clientInfo);
    static void didChangeTooltip(WKViewRef, WKStringRef, const void* clientInfo);
    static void didFindZoomableArea(WKViewRef, WKPoint, WKRect, const void* clientInfo);
#if ENABLE(TOUCH_EVENTS)
    static void doneWithTouchEvent(WKViewRef, WKTouchEventRef, bool, const void* clientInfo);
#endif

    EwkView* m_view;
};

} // namespace WebKit

#endif // ViewClientEfl_h
