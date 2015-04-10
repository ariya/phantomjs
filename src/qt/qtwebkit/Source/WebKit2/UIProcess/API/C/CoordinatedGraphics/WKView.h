/*
 * Copyright (C) 2012 Samsung Electronics
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef WKView_h
#define WKView_h

#include <WebKit2/WKBase.h>
#include <WebKit2/WKGeometry.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*WKViewCallback)(WKViewRef view, const void* clientInfo);
typedef void (*WKViewViewNeedsDisplayCallback)(WKViewRef view, WKRect area, const void* clientInfo);
typedef void (*WKViewPageDidChangeContentsSizeCallback)(WKViewRef view, WKSize size, const void* clientInfo);
typedef void (*WKViewWebProcessCrashedCallback)(WKViewRef view, WKURLRef url, const void* clientInfo);
typedef void (*WKViewPageDidChangeContentsPositionCallback)(WKViewRef view, WKPoint position, const void* clientInfo);
typedef void (*WKViewPageDidRenderFrameCallback)(WKViewRef view, WKSize contentsSize, WKRect coveredRect, const void* clientInfo);
typedef void (*WKViewPageDidChangeViewportAttributesCallback)(WKViewRef view, WKViewportAttributesRef, const void* clientInfo);
typedef void (*WKViewPageDidChangeTooltipCallback)(WKViewRef view, WKStringRef newTooltip, const void* clientInfo);
typedef void (*WKViewDidFindZoomableAreaCallback)(WKViewRef view, WKPoint point, WKRect area, const void* clientInfo);
typedef void (*WKViewDoneWithTouchEventCallback)(WKViewRef view, WKTouchEventRef touchEvent, bool wasEventHandled, const void* clientInfo);

struct WKViewClient {
    int                                              version;
    const void*                                      clientInfo;

    // Version 0
    WKViewViewNeedsDisplayCallback                   viewNeedsDisplay;
    WKViewPageDidChangeContentsSizeCallback          didChangeContentsSize;
    WKViewWebProcessCrashedCallback                  webProcessCrashed;
    WKViewCallback                                   webProcessDidRelaunch;
    WKViewPageDidChangeContentsPositionCallback      didChangeContentsPosition;
    WKViewPageDidRenderFrameCallback                 didRenderFrame;
    WKViewCallback                                   didCompletePageTransition;
    WKViewPageDidChangeViewportAttributesCallback    didChangeViewportAttributes;
    WKViewPageDidChangeTooltipCallback               didChangeTooltip;
    WKViewDidFindZoomableAreaCallback                didFindZoomableArea;
    WKViewDoneWithTouchEventCallback                 doneWithTouchEvent;
};
typedef struct WKViewClient WKViewClient;

enum { kWKViewClientCurrentVersion = 0 };

WK_EXPORT WKViewRef WKViewCreate(WKContextRef context, WKPageGroupRef pageGroup);

WK_EXPORT void WKViewInitialize(WKViewRef);

WK_EXPORT WKSize WKViewGetSize(WKViewRef);
WK_EXPORT void WKViewSetSize(WKViewRef, WKSize size);

WK_EXPORT void WKViewSetViewClient(WKViewRef, const WKViewClient*);

WK_EXPORT bool WKViewIsFocused(WKViewRef);
WK_EXPORT void WKViewSetIsFocused(WKViewRef, bool);

WK_EXPORT bool WKViewIsVisible(WKViewRef);
WK_EXPORT void WKViewSetIsVisible(WKViewRef, bool);

WK_EXPORT float WKViewGetContentScaleFactor(WKViewRef);
WK_EXPORT void WKViewSetContentScaleFactor(WKViewRef, float);

WK_EXPORT WKPoint WKViewGetContentPosition(WKViewRef);
WK_EXPORT void WKViewSetContentPosition(WKViewRef, WKPoint);

WK_EXPORT void WKViewSetUserViewportTranslation(WKViewRef, double tx, double ty);
WK_EXPORT WKPoint WKViewUserViewportToContents(WKViewRef, WKPoint);
WK_EXPORT WKPoint WKViewUserViewportToScene(WKViewRef, WKPoint);
WK_EXPORT WKPoint WKViewContentsToUserViewport(WKViewRef, WKPoint);

WK_EXPORT void WKViewPaintToCurrentGLContext(WKViewRef);

WK_EXPORT WKPageRef WKViewGetPage(WKViewRef);

WK_EXPORT void WKViewSetDrawsBackground(WKViewRef, bool);
WK_EXPORT bool WKViewGetDrawsBackground(WKViewRef);

WK_EXPORT void WKViewSetDrawsTransparentBackground(WKViewRef, bool);
WK_EXPORT bool WKViewGetDrawsTransparentBackground(WKViewRef);

WK_EXPORT void WKViewSuspendActiveDOMObjectsAndAnimations(WKViewRef);
WK_EXPORT void WKViewResumeActiveDOMObjectsAndAnimations(WKViewRef);

WK_EXPORT void WKViewSetShowsAsSource(WKViewRef, bool);
WK_EXPORT bool WKViewGetShowsAsSource(WKViewRef);

WK_EXPORT bool WKViewExitFullScreen(WKViewRef);

WK_EXPORT void WKViewSetOpacity(WKViewRef view, double opacity);
WK_EXPORT double WKViewOpacity(WKViewRef view);

WK_EXPORT void WKViewFindZoomableAreaForRect(WKViewRef, WKRect);

#ifdef __cplusplus
}
#endif

#endif /* WKView_h */
