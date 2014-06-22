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

#ifndef InjectedBundlePageResourceLoadClient_h
#define InjectedBundlePageResourceLoadClient_h

#include "APIClient.h"
#include "SameDocumentNavigationType.h"
#include "WKBundlePage.h"
#include <JavaScriptCore/JSBase.h>
#include <wtf/Forward.h>

namespace WebCore {
class ResourceError;
class ResourceRequest;
class ResourceResponse;
}

namespace WebKit {

class APIObject;
class WebPage;
class WebFrame;

class InjectedBundlePageResourceLoadClient : public APIClient<WKBundlePageResourceLoadClient, kWKBundlePageResourceLoadClientCurrentVersion> {
public:
    void didInitiateLoadForResource(WebPage*, WebFrame*, uint64_t identifier, const WebCore::ResourceRequest&, bool pageIsProvisionallyLoading);
    void willSendRequestForFrame(WebPage*, WebFrame*, uint64_t identifier, WebCore::ResourceRequest&, const WebCore::ResourceResponse&);
    void didReceiveResponseForResource(WebPage*, WebFrame*, uint64_t identifier, const WebCore::ResourceResponse&);
    void didReceiveContentLengthForResource(WebPage*, WebFrame*, uint64_t identifier, uint64_t contentLength);
    void didFinishLoadForResource(WebPage*, WebFrame*, uint64_t identifier);
    void didFailLoadForResource(WebPage*, WebFrame*, uint64_t identifier, const WebCore::ResourceError&);
    bool shouldCacheResponse(WebPage*, WebFrame*, uint64_t identifier);
    bool shouldUseCredentialStorage(WebPage*, WebFrame*, uint64_t identifier);
};

} // namespace WebKit

#endif // InjectedBundlePageResourceLoadClient_h
