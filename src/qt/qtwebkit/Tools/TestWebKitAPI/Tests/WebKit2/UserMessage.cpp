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
#include "Test.h"

#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace TestWebKitAPI {

class WebKit2UserMessageRoundTripTest : public ::testing::Test {
public:
    WebKit2UserMessageRoundTripTest()
        : didFinishLoad(false)
        , didReceiveMessage(false)
    {
    }

    WKRetainPtr<WKContextRef> context;
    OwnPtr<PlatformWebView> webView;

    WKRetainPtr<WKTypeRef> recievedBody;

    bool didFinishLoad;
    bool didReceiveMessage;

    static void didReceiveMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef messageBody, const void* clientInfo)
    {
        if (!WKStringIsEqualToUTF8CString(messageName, "RoundTripReturn"))
            return;

        ((WebKit2UserMessageRoundTripTest*)clientInfo)->recievedBody = messageBody;
        ((WebKit2UserMessageRoundTripTest*)clientInfo)->didReceiveMessage = true;
    }

    static void didFinishLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef, const void* clientInfo)
    {
        ((WebKit2UserMessageRoundTripTest*)clientInfo)->didFinishLoad = true;
    }

    static void setInjectedBundleClient(WKContextRef context, const void* clientInfo)
    {
        WKContextInjectedBundleClient injectedBundleClient;
        memset(&injectedBundleClient, 0, sizeof(injectedBundleClient));
        injectedBundleClient.version = kWKContextInjectedBundleClientCurrentVersion;
        injectedBundleClient.clientInfo = clientInfo;
        injectedBundleClient.didReceiveMessageFromInjectedBundle = didReceiveMessageFromInjectedBundle;

        WKContextSetInjectedBundleClient(context, &injectedBundleClient);
    }

    static void setPageLoaderClient(WKPageRef page, const void* clientInfo)
    {
        WKPageLoaderClient loaderClient;
        memset(&loaderClient, 0, sizeof(loaderClient));
        loaderClient.version = kWKPageLoaderClientCurrentVersion;
        loaderClient.clientInfo = clientInfo;
        loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;

        WKPageSetPageLoaderClient(page, &loaderClient);
    }

    virtual void SetUp()
    {
        context = adoptWK(Util::createContextForInjectedBundleTest("UserMessageTest"));
        setInjectedBundleClient(context.get(), this);

        webView = adoptPtr(new PlatformWebView(context.get()));
        setPageLoaderClient(webView->page(), this);

        didFinishLoad = false;
        didReceiveMessage = false;

        // Force the creation of the 
        WKPageLoadURL(webView->page(), adoptWK(Util::createURLForResource("simple", "html")).get());
        Util::run(&didFinishLoad);

    }

    // Used to test sending a WKType round trip to the WebProcess and back.
    // Result is stored into the recievedBody member variable.
    void roundTrip(WKTypeRef object)
    {
        WKTypeID storedTypeID = WKGetTypeID(object);
    
        recievedBody.clear();
        didReceiveMessage = false;
        WKContextPostMessageToInjectedBundle(context.get(), Util::toWK("RoundTrip").get(), object);
        Util::run(&didReceiveMessage);

        EXPECT_NOT_NULL(recievedBody);
        EXPECT_EQ(storedTypeID, WKGetTypeID(recievedBody.get()));
    }
};


TEST_F(WebKit2UserMessageRoundTripTest, WKURLRequestRef)
{
    WKRetainPtr<WKURLRef> url = adoptWK(WKURLCreateWithUTF8CString("http://webkit.org/"));
    WKRetainPtr<WKURLRequestRef> request = adoptWK(WKURLRequestCreateWithWKURL(url.get()));
    
    roundTrip(request.get());
    WKTypeRef roundTrippedTypeRef = recievedBody.get();

    WKRetainPtr<WKURLRequestRef> roundTrippedRequest = static_cast<WKURLRequestRef>(roundTrippedTypeRef);
    WKRetainPtr<WKURLRef> roundTrippedURL = adoptWK(WKURLRequestCopyURL(roundTrippedRequest.get()));
    EXPECT_TRUE(WKURLIsEqual(roundTrippedURL.get(), url.get()));
}

TEST_F(WebKit2UserMessageRoundTripTest, WKURL)
{
    WKRetainPtr<WKURLRef> url = adoptWK(WKURLCreateWithUTF8CString("http://webkit.org/"));
    
    roundTrip(url.get());
    WKTypeRef roundTrippedTypeRef = recievedBody.get();

    WKRetainPtr<WKURLRef> roundTrippedURL = static_cast<WKURLRef>(roundTrippedTypeRef);
    EXPECT_TRUE(WKURLIsEqual(roundTrippedURL.get(), url.get()));
}

TEST_F(WebKit2UserMessageRoundTripTest, WKString)
{
    WKRetainPtr<WKStringRef> string = adoptWK(WKStringCreateWithUTF8CString("An important string"));
    
    roundTrip(string.get());
    WKTypeRef roundTrippedTypeRef = recievedBody.get();

    WKRetainPtr<WKStringRef> roundTrippedString = static_cast<WKStringRef>(roundTrippedTypeRef);
    EXPECT_TRUE(WKStringIsEqual(roundTrippedString.get(), string.get()));
}

} // namespace TestWebKitAPI
