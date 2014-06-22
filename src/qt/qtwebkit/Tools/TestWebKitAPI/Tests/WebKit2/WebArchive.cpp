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

#include "config.h"
#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include <CoreFoundation/CoreFoundation.h>
#include <WebKit2/WKURLCF.h>
#include <WebKit2/WKContextPrivate.h>
#include <wtf/RetainPtr.h>

namespace TestWebKitAPI {

static bool didFinishLoad;
static bool didReceiveMessage;
    
static void didReceiveMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef body, const void*)
{
    didReceiveMessage = true;

    EXPECT_WK_STREQ("DidGetWebArchive", messageName);
    EXPECT_TRUE(body);
    EXPECT_EQ(WKDataGetTypeID(), WKGetTypeID(body));
    WKDataRef receivedData = static_cast<WKDataRef>(body);
        
    // Do basic sanity checks on the returned webarchive. We have more thorough checks in LayoutTests.
    size_t size = WKDataGetSize(receivedData);
    const unsigned char* bytes = WKDataGetBytes(receivedData);
    RetainPtr<CFDataRef> data = adoptCF(CFDataCreate(0, bytes, size));
    RetainPtr<CFPropertyListRef> propertyList = adoptCF(CFPropertyListCreateWithData(0, data.get(), kCFPropertyListImmutable, 0, 0));
    EXPECT_TRUE(propertyList);
    
    // It should be a dictionary.
    EXPECT_EQ(CFDictionaryGetTypeID(), CFGetTypeID(propertyList.get()));
    CFDictionaryRef dictionary = (CFDictionaryRef)propertyList.get();
    
    // It should have a main resource.
    CFTypeRef mainResource = CFDictionaryGetValue(dictionary, CFSTR("WebMainResource"));
    EXPECT_TRUE(mainResource);
    EXPECT_EQ(CFDictionaryGetTypeID(), CFGetTypeID(mainResource));
    CFDictionaryRef mainResourceDictionary = (CFDictionaryRef)mainResource;
    
    // Main resource should have a non-empty url and mime type.
    CFTypeRef url = CFDictionaryGetValue(mainResourceDictionary, CFSTR("WebResourceURL"));
    EXPECT_TRUE(url);
    EXPECT_EQ(CFStringGetTypeID(), CFGetTypeID(url));
    EXPECT_NE(CFStringGetLength((CFStringRef)url), 0);
    
    CFTypeRef mimeType = CFDictionaryGetValue(mainResourceDictionary, CFSTR("WebResourceMIMEType"));
    EXPECT_TRUE(mimeType);
    EXPECT_EQ(CFStringGetTypeID(), CFGetTypeID(mimeType));
    EXPECT_NE(CFStringGetLength((CFStringRef)mimeType), 0);
    
    // Main resource dictionary should have a "WebResourceData" key.
    CFTypeRef resourceData = CFDictionaryGetValue(mainResourceDictionary, CFSTR("WebResourceData"));
    EXPECT_TRUE(resourceData);
    EXPECT_EQ(CFDataGetTypeID(), CFGetTypeID(resourceData));
    
    RetainPtr<CFStringRef> stringData = adoptCF(CFStringCreateFromExternalRepresentation(0, (CFDataRef)resourceData, kCFStringEncodingUTF8));
    EXPECT_TRUE(stringData);
    
    // It should contain the string "Simple HTML file." in it.
    bool foundString = CFStringFind(stringData.get(), CFSTR("Simple HTML file."), 0).location != kCFNotFound;
    EXPECT_TRUE(foundString);
}

static void setInjectedBundleClient(WKContextRef context)
{
    WKContextInjectedBundleClient injectedBundleClient;
    memset(&injectedBundleClient, 0, sizeof(injectedBundleClient));
    injectedBundleClient.didReceiveMessageFromInjectedBundle = didReceiveMessageFromInjectedBundle;

    WKContextSetInjectedBundleClient(context, &injectedBundleClient);
}

static void didFinishLoadForFrame(WKPageRef page, WKFrameRef, WKTypeRef, const void*)
{
    didFinishLoad = true;
}

TEST(WebKit2, WebArchive)
{
    WKRetainPtr<WKContextRef> context = adoptWK(Util::createContextForInjectedBundleTest("WebArchiveTest"));
    setInjectedBundleClient(context.get());

    PlatformWebView webView(context.get());

    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));
    
    loaderClient.version = kWKPageLoaderClientCurrentVersion;
    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;
    WKPageSetPageLoaderClient(webView.page(), &loaderClient);

    WKPageLoadURL(webView.page(), adoptWK(Util::createURLForResource("simple", "html")).get());

    // Wait till the load finishes before getting the web archive.
    Util::run(&didFinishLoad);
    WKContextPostMessageToInjectedBundle(context.get(), Util::toWK("GetWebArchive").get(), webView.page());

    // Wait till we have received the web archive from the injected bundle.
    Util::run(&didReceiveMessage);
}

} // namespace TestWebKitAPI
