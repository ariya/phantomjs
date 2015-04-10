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

#ifndef WKBundlePageOverlay_h
#define WKBundlePageOverlay_h

#include <WebKit2/WKBase.h>
#include <WebKit2/WKEvent.h>
#include <WebKit2/WKGeometry.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Page overlay client.
typedef void (*WKBundlePageOverlayWillMoveToPageCallback)(WKBundlePageOverlayRef pageOverlay, WKBundlePageRef page, const void* clientInfo);
typedef void (*WKBundlePageOverlayDidMoveToPageCallback)(WKBundlePageOverlayRef pageOverlay, WKBundlePageRef page, const void* clientInfo);
typedef void (*WKBundlePageOverlayDrawRectCallback)(WKBundlePageOverlayRef pageOverlay, void* graphicsContext, WKRect dirtyRect, const void* clientInfo);
typedef bool (*WKBundlePageOverlayMouseDownCallback)(WKBundlePageOverlayRef pageOverlay, WKPoint position, WKEventMouseButton mouseButton, const void* clientInfo);
typedef bool (*WKBundlePageOverlayMouseUpCallback)(WKBundlePageOverlayRef pageOverlay, WKPoint position, WKEventMouseButton mouseButton, const void* clientInfo);
typedef bool (*WKBundlePageOverlayMouseMovedCallback)(WKBundlePageOverlayRef pageOverlay, WKPoint position, const void* clientInfo);
typedef bool (*WKBundlePageOverlayMouseDraggedCallback)(WKBundlePageOverlayRef pageOverlay, WKPoint position, WKEventMouseButton mouseButton, const void* clientInfo);

struct WKBundlePageOverlayClient {
    int                                                                 version;
    const void *                                                        clientInfo;
    WKBundlePageOverlayWillMoveToPageCallback                           willMoveToPage;
    WKBundlePageOverlayDidMoveToPageCallback                            didMoveToPage;
    WKBundlePageOverlayDrawRectCallback                                 drawRect;
    WKBundlePageOverlayMouseDownCallback                                mouseDown;
    WKBundlePageOverlayMouseUpCallback                                  mouseUp;
    WKBundlePageOverlayMouseMovedCallback                               mouseMoved;
    WKBundlePageOverlayMouseDraggedCallback                             mouseDragged;
};
typedef struct WKBundlePageOverlayClient WKBundlePageOverlayClient;

enum { kWKBundlePageOverlayClientCurrentVersion = 0 };
    
WK_EXPORT WKTypeID WKBundlePageOverlayGetTypeID();

WK_EXPORT WKBundlePageOverlayRef WKBundlePageOverlayCreate(WKBundlePageOverlayClient* client);
WK_EXPORT void WKBundlePageOverlaySetNeedsDisplay(WKBundlePageOverlayRef bundlePageOverlay, WKRect rect);
WK_EXPORT float WKBundlePageOverlayFractionFadedIn(WKBundlePageOverlayRef bundlePageOverlay);

#ifdef __cplusplus
}
#endif

#endif // WKBundlePageOverlay_h
