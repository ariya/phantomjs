/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef WKBundlePageBanner_h
#define WKBundlePageBanner_h

#include <WebKit2/WKBase.h>
#include <WebKit2/WKEvent.h>
#include <WebKit2/WKGeometry.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Page banner client.
typedef bool (*WKBundlePageBannerMouseDownCallback)(WKBundlePageBannerRef pageBanner, WKPoint position, WKEventMouseButton mouseButton, const void* clientInfo);
typedef bool (*WKBundlePageBannerMouseUpCallback)(WKBundlePageBannerRef pageBanner, WKPoint position, WKEventMouseButton mouseButton, const void* clientInfo);
typedef bool (*WKBundlePageBannerMouseMovedCallback)(WKBundlePageBannerRef pageBanner, WKPoint position, const void* clientInfo);
typedef bool (*WKBundlePageBannerMouseDraggedCallback)(WKBundlePageBannerRef pageBanner, WKPoint position, WKEventMouseButton mouseButton, const void* clientInfo);

struct WKBundlePageBannerClient {
    int                                                                 version;
    const void *                                                        clientInfo;
    WKBundlePageBannerMouseDownCallback                                 mouseDown;
    WKBundlePageBannerMouseUpCallback                                   mouseUp;
    WKBundlePageBannerMouseMovedCallback                                mouseMoved;
    WKBundlePageBannerMouseDraggedCallback                              mouseDragged;
};
typedef struct WKBundlePageBannerClient WKBundlePageBannerClient;

enum { kWKBundlePageBannerClientCurrentVersion = 0 };
    
WK_EXPORT WKTypeID WKBundlePageBannerGetTypeID();

#ifdef __cplusplus
}
#endif

#endif // WKBundlePageBanner_h
