/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef InjectedBundlePagePolicyClient_h
#define InjectedBundlePagePolicyClient_h

#include "APIClient.h"
#include "APIObject.h"
#include "WKBundlePage.h"
#include <wtf/Forward.h>

namespace WebCore {
    class ResourceError;
    class ResourceRequest;
    class ResourceResponse;
}

namespace WebKit {

class InjectedBundleNavigationAction;
class WebFrame;
class WebPage;

class InjectedBundlePagePolicyClient : public APIClient<WKBundlePagePolicyClient, kWKBundlePagePolicyClientCurrentVersion> {
public:
    WKBundlePagePolicyAction decidePolicyForNavigationAction(WebPage*, WebFrame*, InjectedBundleNavigationAction*, const WebCore::ResourceRequest&, RefPtr<APIObject>& userData);
    WKBundlePagePolicyAction decidePolicyForNewWindowAction(WebPage*, WebFrame*, InjectedBundleNavigationAction*, const WebCore::ResourceRequest&, const String& frameName, RefPtr<APIObject>& userData);
    WKBundlePagePolicyAction decidePolicyForResponse(WebPage*, WebFrame*, const WebCore::ResourceResponse&, const WebCore::ResourceRequest&, RefPtr<APIObject>& userData);
    void unableToImplementPolicy(WebPage*, WebFrame*, const WebCore::ResourceError&, RefPtr<APIObject>& userData);
};

} // namespace WebKit

#endif // InjectedBundlePagePolicyClient_h
