/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include <WebKit2/WKBundlePage.h>
#include <WebKit2/WKBundleFrame.h>
#include <WebKit2/WKRetainPtr.h>

namespace TestWebKitAPI {

static WKRetainPtr<WKBundleRef> testBundle;
static WKRetainPtr<WKBundleFrameRef> childFrame;

class ParentFrameTest : public InjectedBundleTest {
public:
    ParentFrameTest(const std::string& identifier);

private:
    virtual void didCreatePage(WKBundleRef, WKBundlePageRef);
    
};

static InjectedBundleTest::Register<ParentFrameTest> registrar("ParentFrameTest");

ParentFrameTest::ParentFrameTest(const std::string& identifier)
    : InjectedBundleTest(identifier)
{
}

static void didFinishLoadForFrame(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo)
{
    if (!WKBundleFrameIsMainFrame(frame)) {
        childFrame = frame;
        return;
    }
    
    bool isParentFrameCheckSuccessful = childFrame ? WKBundleFrameGetParentFrame(childFrame.get()) == frame : false;
    WKBundlePostMessage(testBundle.get(), Util::toWK("DidCheckParentFrame").get(), adoptWK(WKBooleanCreate(isParentFrameCheckSuccessful)).get());
}

void ParentFrameTest::didCreatePage(WKBundleRef bundle, WKBundlePageRef page)
{
    testBundle = bundle;
    
    WKBundlePageLoaderClient pageLoaderClient;
    memset(&pageLoaderClient, 0, sizeof(pageLoaderClient));
    
    pageLoaderClient.version = 1;
    pageLoaderClient.didFinishLoadForFrame = didFinishLoadForFrame;
    
    WKBundlePageSetPageLoaderClient(page, &pageLoaderClient);
}

} // namespace TestWebKitAPI
