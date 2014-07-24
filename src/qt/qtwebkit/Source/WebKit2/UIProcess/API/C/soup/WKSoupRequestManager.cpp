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

#include "config.h"
#include "WKSoupRequestManager.h"

#include "WKAPICast.h"
#include "WebSoupRequestManagerProxy.h"

using namespace WebKit;

WKTypeID WKSoupRequestManagerGetTypeID()
{
    return toAPI(WebSoupRequestManagerProxy::APIType);
}

void WKSoupRequestManagerSetClient(WKSoupRequestManagerRef soupRequestManagerRef, const WKSoupRequestManagerClient* wkClient)
{
    toImpl(soupRequestManagerRef)->initializeClient(wkClient);
}

void WKSoupRequestManagerRegisterURIScheme(WKSoupRequestManagerRef soupRequestManagerRef, WKStringRef schemeRef)
{
    toImpl(soupRequestManagerRef)->registerURIScheme(toWTFString(schemeRef));
}

void WKSoupRequestManagerDidHandleURIRequest(WKSoupRequestManagerRef soupRequestManagerRef, WKDataRef data, uint64_t contentLength, WKStringRef mimeTypeRef, uint64_t requestID)
{
    toImpl(soupRequestManagerRef)->didHandleURIRequest(toImpl(data), contentLength, toWTFString(mimeTypeRef), requestID);
}

void WKSoupRequestManagerDidReceiveURIRequestData(WKSoupRequestManagerRef soupRequestManagerRef, WKDataRef data, uint64_t requestID)
{
    toImpl(soupRequestManagerRef)->didReceiveURIRequestData(toImpl(data), requestID);
}
