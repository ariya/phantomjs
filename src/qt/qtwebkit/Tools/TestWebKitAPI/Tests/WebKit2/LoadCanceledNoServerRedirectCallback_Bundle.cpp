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

#include "config.h"
#include "InjectedBundleTest.h"
#include "PlatformUtilities.h"
#include "Test.h"

#include <WebKit2/WKBundlePage.h>
#include <WebKit2/WKBundleFrame.h>
#include <WebKit2/WKRetainPtr.h>

#include <wtf/Assertions.h>

namespace TestWebKitAPI {
    
class LoadCanceledNoServerRedirectCallbackTest : public InjectedBundleTest {
public:
    LoadCanceledNoServerRedirectCallbackTest(const std::string& identifier)
        : InjectedBundleTest(identifier)
    {
    }
    
    static WKURLRequestRef willSendRequestForFrame(WKBundlePageRef, WKBundleFrameRef frame, uint64_t resourceIdentifier, WKURLRequestRef request, WKURLResponseRef redirectResponse, const void *clientInfo)
    {
        // Allow the loading of the main resource, but don't allow the loading of an iframe, return null from willSendRequest.
        if (WKBundleFrameIsMainFrame(frame)) {
            WKRetainPtr<WKURLRequestRef> newRequest = request;
            return newRequest.leakRef();
        }
        
        return 0;
    }

    virtual void didCreatePage(WKBundleRef bundle, WKBundlePageRef page)
    {
        WKBundlePageResourceLoadClient resourceLoadClient;
        memset(&resourceLoadClient, 0, sizeof(resourceLoadClient));
        
        resourceLoadClient.version = 0;
        resourceLoadClient.willSendRequestForFrame = willSendRequestForFrame;

        WKBundlePageSetResourceLoadClient(page, &resourceLoadClient);

    }
};

static InjectedBundleTest::Register<LoadCanceledNoServerRedirectCallbackTest> registrar("LoadCanceledNoServerRedirectCallbackTest");

} // namespace TestWebKitAPI
