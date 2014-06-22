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
#include "Test.h"

#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace TestWebKitAPI {

class WebKit2WillLoadTest : public ::testing::Test {
public:
    WebKit2WillLoadTest()
        : didReceiveMessage(false)
    {
    }

    WKRetainPtr<WKContextRef> context;
    OwnPtr<PlatformWebView> webView;

    WKRetainPtr<WKStringRef> messageName;
    WKRetainPtr<WKTypeRef> messageBody;
    bool didReceiveMessage;

    static void didReceiveMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef messageBody, const void* clientInfo)
    {
        ((WebKit2WillLoadTest*)clientInfo)->messageName = messageName;
        ((WebKit2WillLoadTest*)clientInfo)->messageBody = messageBody;
        ((WebKit2WillLoadTest*)clientInfo)->didReceiveMessage = true;
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

    virtual void SetUp()
    {
        context = adoptWK(Util::createContextForInjectedBundleTest("WillLoadTest"));
        setInjectedBundleClient(context.get(), this);

        webView = adoptPtr(new PlatformWebView(context.get()));

        didReceiveMessage = false;
    }

    void testWillLoadURLRequestReturnValues(WKURLRef expectedURL, WKStringRef expectedUserDataString)
    {
        didReceiveMessage = false;
        Util::run(&didReceiveMessage);

        EXPECT_WK_STREQ("WillLoadURLRequestReturn", messageName.get());

        EXPECT_EQ(WKDictionaryGetTypeID(), WKGetTypeID(messageBody.get()));
        WKDictionaryRef dictionary = static_cast<WKDictionaryRef>(messageBody.get());

        if (expectedUserDataString) {
            WKStringRef userDataReturnValue = static_cast<WKStringRef>(WKDictionaryGetItemForKey(dictionary, Util::toWK("UserDataReturn").get()));
            EXPECT_WK_STREQ(expectedUserDataString, userDataReturnValue);
        } else
            EXPECT_NULL(WKDictionaryGetItemForKey(dictionary, Util::toWK("UserDataReturn").get()));

        WKURLRequestRef urlRequestReturnValue = static_cast<WKURLRequestRef>(WKDictionaryGetItemForKey(dictionary, Util::toWK("URLRequestReturn").get()));
        WKRetainPtr<WKURLRef> urlReturnValue = adoptWK(WKURLRequestCopyURL(urlRequestReturnValue));
        EXPECT_TRUE(WKURLIsEqual(expectedURL, urlReturnValue.get()));
    }

    void testWillLoadDataRequestReturnValues(WKURLRef expectedURL, WKStringRef expectedMIMEType, WKStringRef expectedEncodingName, WKURLRef expectedUnreachableURL, WKStringRef expectedUserDataString)
    {
        didReceiveMessage = false;
        Util::run(&didReceiveMessage);

        EXPECT_WK_STREQ("WillLoadDataRequestReturn", messageName.get());

        EXPECT_EQ(WKDictionaryGetTypeID(), WKGetTypeID(messageBody.get()));
        WKDictionaryRef dictionary = static_cast<WKDictionaryRef>(messageBody.get());

        if (expectedUserDataString) {
            WKStringRef userDataReturnValue = static_cast<WKStringRef>(WKDictionaryGetItemForKey(dictionary, Util::toWK("UserDataReturn").get()));
            EXPECT_WK_STREQ(expectedUserDataString, userDataReturnValue);
        } else
            EXPECT_NULL(WKDictionaryGetItemForKey(dictionary, Util::toWK("UserDataReturn").get()));

        WKURLRequestRef urlRequestReturnValue = static_cast<WKURLRequestRef>(WKDictionaryGetItemForKey(dictionary, Util::toWK("URLRequestReturn").get()));
        WKRetainPtr<WKURLRef> urlReturnValue = adoptWK(WKURLRequestCopyURL(urlRequestReturnValue));
        EXPECT_TRUE(WKURLIsEqual(expectedURL, urlReturnValue.get()));

        WKStringRef MIMEType = static_cast<WKStringRef>(WKDictionaryGetItemForKey(dictionary, Util::toWK("MIMETypeReturn").get()));
        EXPECT_WK_STREQ(expectedMIMEType, MIMEType);

        WKStringRef encodingName = static_cast<WKStringRef>(WKDictionaryGetItemForKey(dictionary, Util::toWK("EncodingNameReturn").get()));
        EXPECT_WK_STREQ(expectedEncodingName, encodingName);

        if (expectedUnreachableURL) {
            WKURLRef unreachableURL = static_cast<WKURLRef>(WKDictionaryGetItemForKey(dictionary, Util::toWK("UnreachableURLReturn").get()));
            EXPECT_TRUE(WKURLIsEqual(expectedUnreachableURL, unreachableURL));
        } else
            EXPECT_NULL(WKDictionaryGetItemForKey(dictionary, Util::toWK("UnreachableURLReturn").get()));
    }
};

// URL Request tests

TEST_F(WebKit2WillLoadTest, WKPageLoadURLWithUserData)
{
    WKRetainPtr<WKURLRef> url = adoptWK(WKURLCreateWithUTF8CString("about:blank"));
    WKRetainPtr<WKStringRef> userData = Util::toWK("WKPageLoadURLWithUserData UserData");
    WKPageLoadURLWithUserData(webView->page(), url.get(), userData.get());

    testWillLoadURLRequestReturnValues(url.get(), userData.get());
}

TEST_F(WebKit2WillLoadTest, WKPageLoadURL)
{
    WKRetainPtr<WKURLRef> url = adoptWK(WKURLCreateWithUTF8CString("about:blank"));
    WKPageLoadURL(webView->page(), url.get());

    testWillLoadURLRequestReturnValues(url.get(), 0);
}

TEST_F(WebKit2WillLoadTest, WKPageLoadURLRequestWithUserData)
{
    WKRetainPtr<WKURLRef> url = adoptWK(WKURLCreateWithUTF8CString("about:blank"));
    WKRetainPtr<WKURLRequestRef> urlRequest = adoptWK(WKURLRequestCreateWithWKURL(url.get()));
    WKRetainPtr<WKStringRef> userData = Util::toWK("WKPageLoadURLRequestWithUserData UserData");
    WKPageLoadURLRequestWithUserData(webView->page(), urlRequest.get(), userData.get());

    testWillLoadURLRequestReturnValues(url.get(), userData.get());
}

TEST_F(WebKit2WillLoadTest, WKPageLoadURLRequest)
{
    WKRetainPtr<WKURLRef> url = adoptWK(WKURLCreateWithUTF8CString("about:blank"));
    WKRetainPtr<WKURLRequestRef> urlRequest = adoptWK(WKURLRequestCreateWithWKURL(url.get()));
    WKPageLoadURLRequest(webView->page(), urlRequest.get());

    testWillLoadURLRequestReturnValues(url.get(), 0);
}

// Data Request tests

TEST_F(WebKit2WillLoadTest, WKPageLoadHTMLStringWithUserData)
{
    WKRetainPtr<WKURLRef> baseURL = adoptWK(WKURLCreateWithUTF8CString("about:blank"));
    WKRetainPtr<WKStringRef> userData = Util::toWK("WKPageLoadHTMLStringWithUserData UserData");
    WKRetainPtr<WKStringRef> htmlString = Util::toWK("<body>Hello, World</body>");

    WKPageLoadHTMLStringWithUserData(webView->page(), htmlString.get(), baseURL.get(), userData.get());

    testWillLoadDataRequestReturnValues(baseURL.get(), Util::toWK("text/html").get(), Util::toWK("utf-16").get(), 0, userData.get());
}

TEST_F(WebKit2WillLoadTest, WKPageLoadHTMLString)
{
    WKRetainPtr<WKURLRef> baseURL = adoptWK(WKURLCreateWithUTF8CString("about:blank"));
    WKRetainPtr<WKStringRef> htmlString = Util::toWK("<body>Hello, World</body>");

    WKPageLoadHTMLString(webView->page(), htmlString.get(), baseURL.get());

    testWillLoadDataRequestReturnValues(baseURL.get(), Util::toWK("text/html").get(), Util::toWK("utf-16").get(), 0, 0);
}

TEST_F(WebKit2WillLoadTest, WKPageLoadAlternateHTMLStringWithUserData)
{
    WKRetainPtr<WKStringRef> htmlString = Util::toWK("<body>Hello, World</body>");

    WKRetainPtr<WKURLRef> baseURL = adoptWK(WKURLCreateWithUTF8CString("about:blank"));
    WKRetainPtr<WKURLRef> unreachableURL = adoptWK(WKURLCreateWithUTF8CString("about:other"));
    WKRetainPtr<WKStringRef> userData = Util::toWK("WKPageLoadAlternateHTMLStringWithUserData UserData");

    WKPageLoadAlternateHTMLStringWithUserData(webView->page(), htmlString.get(), baseURL.get(), unreachableURL.get(), userData.get());

    testWillLoadDataRequestReturnValues(baseURL.get(), Util::toWK("text/html").get(), Util::toWK("utf-16").get(), unreachableURL.get(), userData.get());
}

TEST_F(WebKit2WillLoadTest, WKPageLoadAlternateHTMLString)
{
    WKRetainPtr<WKStringRef> htmlString = Util::toWK("<body>Hello, World</body>");

    WKRetainPtr<WKURLRef> baseURL = adoptWK(WKURLCreateWithUTF8CString("about:blank"));
    WKRetainPtr<WKURLRef> unreachableURL = adoptWK(WKURLCreateWithUTF8CString("about:other"));

    WKPageLoadAlternateHTMLString(webView->page(), htmlString.get(), baseURL.get(), unreachableURL.get());

    testWillLoadDataRequestReturnValues(baseURL.get(), Util::toWK("text/html").get(), Util::toWK("utf-16").get(), unreachableURL.get(), 0);
}

TEST_F(WebKit2WillLoadTest, WKPageLoadPlainTextStringWithUserData)
{
    WKRetainPtr<WKStringRef> plaintTextString = Util::toWK("Hello, World");
    WKRetainPtr<WKStringRef> userData = Util::toWK("WKPageLoadPlainTextStringWithUserData UserData");

    WKPageLoadPlainTextStringWithUserData(webView->page(), plaintTextString.get(), userData.get());

    WKRetainPtr<WKURLRef> blankURL = adoptWK(WKURLCreateWithUTF8CString("about:blank"));
    testWillLoadDataRequestReturnValues(blankURL.get(), Util::toWK("text/plain").get(), Util::toWK("utf-16").get(), 0, userData.get());
}

TEST_F(WebKit2WillLoadTest, WKPageLoadPlainTextString)
{
    WKRetainPtr<WKStringRef> plaintTextString = Util::toWK("Hello, World");

    WKPageLoadPlainTextString(webView->page(), plaintTextString.get());

    WKRetainPtr<WKURLRef> blankURL = adoptWK(WKURLCreateWithUTF8CString("about:blank"));
    testWillLoadDataRequestReturnValues(blankURL.get(), Util::toWK("text/plain").get(), Util::toWK("utf-16").get(), 0, 0);
}

} // namespace TestWebKitAPI
