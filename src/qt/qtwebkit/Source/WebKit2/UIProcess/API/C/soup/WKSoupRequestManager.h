/*
 * Copyright (C) 2012 Igalia S.L.
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

#ifndef WKSoupRequestManager_h
#define WKSoupRequestManager_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*WKSoupRequestManagerDidReceiveURIRequestCallback)(WKSoupRequestManagerRef soupRequestManagerRef, WKURLRef urlRef, WKPageRef pageRef, uint64_t requestID, const void* clientInfo);
typedef void (*WKSoupRequestManagerDidFailToLoadURIRequestCallback)(WKSoupRequestManagerRef soupRequestManagerRef, uint64_t requestID, const void* clientInfo);

struct WKSoupRequestManagerClient {
    int                                                 version;
    const void*                                         clientInfo;
    WKSoupRequestManagerDidReceiveURIRequestCallback    didReceiveURIRequest;
    WKSoupRequestManagerDidFailToLoadURIRequestCallback didFailToLoadURIRequest;
};
typedef struct WKSoupRequestManagerClient WKSoupRequestManagerClient;

enum { kWKSoupRequestManagerClientCurrentVersion = 0 };

WK_EXPORT WKTypeID WKSoupRequestManagerGetTypeID();

WK_EXPORT void WKSoupRequestManagerSetClient(WKSoupRequestManagerRef, const WKSoupRequestManagerClient* client);
WK_EXPORT void WKSoupRequestManagerRegisterURIScheme(WKSoupRequestManagerRef, WKStringRef schemeRef);
WK_EXPORT void WKSoupRequestManagerDidHandleURIRequest(WKSoupRequestManagerRef, WKDataRef, uint64_t contentLength, WKStringRef mimeTypeRef, uint64_t requestID);
WK_EXPORT void WKSoupRequestManagerDidReceiveURIRequestData(WKSoupRequestManagerRef, WKDataRef, uint64_t requestID);

#ifdef __cplusplus
}
#endif

#endif /* WKSoupRequestManager_h */
