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

#include "config.h"
#include "InjectedBundleTest.h"

#include "PlatformUtilities.h"
#include <WebKit2/WKBundlePage.h>
#include <WebKit2/WKRetainPtr.h>

namespace TestWebKitAPI {

class WillLoadTest : public InjectedBundleTest {
public:
    WillLoadTest(const std::string& identifier)
        : InjectedBundleTest(identifier)
    {
    }

private:
    static void willLoadURLRequest(WKBundlePageRef page, WKURLRequestRef request, WKTypeRef userData, const void *clientInfo)
    {
        WKRetainPtr<WKMutableDictionaryRef> messageBody = adoptWK(WKMutableDictionaryCreate());

        WKDictionaryAddItem(messageBody.get(), Util::toWK("URLRequestReturn").get(), request);
        WKDictionaryAddItem(messageBody.get(), Util::toWK("UserDataReturn").get(), userData);

        WKBundlePostMessage(InjectedBundleController::shared().bundle(), Util::toWK("WillLoadURLRequestReturn").get(), messageBody.get());
    }

    static void willLoadDataRequest(WKBundlePageRef page, WKURLRequestRef request, WKDataRef data, WKStringRef MIMEType, WKStringRef encodingName, WKURLRef unreachableURL, WKTypeRef userData, const void *clientInfo)
    {
        WKRetainPtr<WKMutableDictionaryRef> messageBody = adoptWK(WKMutableDictionaryCreate());

        WKDictionaryAddItem(messageBody.get(), Util::toWK("URLRequestReturn").get(), request);
        WKDictionaryAddItem(messageBody.get(), Util::toWK("DataReturn").get(), data);
        WKDictionaryAddItem(messageBody.get(), Util::toWK("MIMETypeReturn").get(), MIMEType);
        WKDictionaryAddItem(messageBody.get(), Util::toWK("EncodingNameReturn").get(), encodingName);
        WKDictionaryAddItem(messageBody.get(), Util::toWK("UnreachableURLReturn").get(), unreachableURL);
        WKDictionaryAddItem(messageBody.get(), Util::toWK("UserDataReturn").get(), userData);

        WKBundlePostMessage(InjectedBundleController::shared().bundle(), Util::toWK("WillLoadDataRequestReturn").get(), messageBody.get());

    }

    virtual void didCreatePage(WKBundleRef, WKBundlePageRef bundlePage) OVERRIDE
    {
        WKBundlePageLoaderClient pageLoaderClient;
        memset(&pageLoaderClient, 0, sizeof(pageLoaderClient));
        
        pageLoaderClient.version = 6;
        pageLoaderClient.clientInfo = this;
        pageLoaderClient.willLoadURLRequest = willLoadURLRequest;
        pageLoaderClient.willLoadDataRequest = willLoadDataRequest;
        
        WKBundlePageSetPageLoaderClient(bundlePage, &pageLoaderClient);
    }
};

static InjectedBundleTest::Register<WillLoadTest> registrar("WillLoadTest");

} // namespace TestWebKitAPI
