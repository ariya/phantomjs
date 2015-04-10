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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTAwBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WKPageEfl_h
#define WKPageEfl_h

#include <WebKit2/WKBase.h>
#include <WebKit2/WKGeometry.h>
#include <WebKit2/WKPopupItem.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*WKPageShowPopupMenuCallback)(WKPageRef page, WKPopupMenuListenerRef menuListenerRef, WKRect rect, WKPopupItemTextDirection textDirection, double pageScaleFactor, WKArrayRef itemsRef, int32_t selectedIndex, const void* clientInfo);
typedef void (*WKPageHidePopupMenuCallback)(WKPageRef page, const void* clientInfo);

struct WKPageUIPopupMenuClient {
    int                                            version;
    const void*                                    clientInfo;
    WKPageShowPopupMenuCallback                    showPopupMenu;
    WKPageHidePopupMenuCallback                    hidePopupMenu;
};
typedef struct WKPageUIPopupMenuClient WKPageUIPopupMenuClient;

enum { kWKPageUIPopupMenuClientCurrentVersion = 0 };

WK_EXPORT void WKPageSetUIPopupMenuClient(WKPageRef page, const WKPageUIPopupMenuClient* client);

#ifdef __cplusplus
}
#endif

#endif /* WKPageEfl_h */
