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
#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include "Test.h"
#include <wtf/Vector.h>

namespace TestWebKitAPI {

static bool finished;

static const char* expectedMessages[] = {
"GlobalObjectIsAvailableForFrame called",
"GlobalObjectIsAvailableForFrame called",
"GlobalObjectIsAvailableForFrame called",
"GlobalObjectIsAvailableForFrame called",
"Subframe finished loading",
"Extension states:\nFirst page, main frame, standard world - Connected\nFirst page, main frame, non-standard world - Connected\nFirst page, subframe, standard world - Connected\nFirst page, subframe, non-standard world - Connected\nSecond page, main frame, standard world - Uncreated\nSecond page, main frame, non-standard world - Uncreated\nFirst page, main frame, standard world - Uncreated\nFirst page, main frame, non-standard world - Uncreated\nFirst page, subframe, standard world - Uncreated\nFirst page, subframe, non-standard world - Uncreated",
"Main frame finished loading",
"Extension states:\nFirst page, main frame, standard world - Connected\nFirst page, main frame, non-standard world - Connected\nFirst page, subframe, standard world - Connected\nFirst page, subframe, non-standard world - Connected\nSecond page, main frame, standard world - Uncreated\nSecond page, main frame, non-standard world - Uncreated\nFirst page, main frame, standard world - Uncreated\nFirst page, main frame, non-standard world - Uncreated\nFirst page, subframe, standard world - Uncreated\nFirst page, subframe, non-standard world - Uncreated",
"WillDestroyDOMWindowExtensionToGlobalObject called",
"WillDestroyDOMWindowExtensionToGlobalObject called",
"WillDestroyDOMWindowExtensionToGlobalObject called",
"WillDestroyDOMWindowExtensionToGlobalObject called",
"GlobalObjectIsAvailableForFrame called",
"GlobalObjectIsAvailableForFrame called",
"Main frame finished loading",
"Extension states:\nFirst page, main frame, standard world - Destroyed\nFirst page, main frame, non-standard world - Destroyed\nFirst page, subframe, standard world - Destroyed\nFirst page, subframe, non-standard world - Destroyed\nSecond page, main frame, standard world - Connected\nSecond page, main frame, non-standard world - Connected\nFirst page, main frame, standard world - Uncreated\nFirst page, main frame, non-standard world - Uncreated\nFirst page, subframe, standard world - Uncreated\nFirst page, subframe, non-standard world - Uncreated",
"WillDestroyDOMWindowExtensionToGlobalObject called",
"WillDestroyDOMWindowExtensionToGlobalObject called",
"GlobalObjectIsAvailableForFrame called",
"GlobalObjectIsAvailableForFrame called",
"GlobalObjectIsAvailableForFrame called",
"GlobalObjectIsAvailableForFrame called",
"Subframe finished loading",
"Extension states:\nFirst page, main frame, standard world - Destroyed\nFirst page, main frame, non-standard world - Destroyed\nFirst page, subframe, standard world - Destroyed\nFirst page, subframe, non-standard world - Destroyed\nSecond page, main frame, standard world - Destroyed\nSecond page, main frame, non-standard world - Destroyed\nFirst page, main frame, standard world - Connected\nFirst page, main frame, non-standard world - Connected\nFirst page, subframe, standard world - Connected\nFirst page, subframe, non-standard world - Connected",
"Main frame finished loading",
"Extension states:\nFirst page, main frame, standard world - Destroyed\nFirst page, main frame, non-standard world - Destroyed\nFirst page, subframe, standard world - Destroyed\nFirst page, subframe, non-standard world - Destroyed\nSecond page, main frame, standard world - Destroyed\nSecond page, main frame, non-standard world - Destroyed\nFirst page, main frame, standard world - Connected\nFirst page, main frame, non-standard world - Connected\nFirst page, subframe, standard world - Connected\nFirst page, subframe, non-standard world - Connected",
"Extension states:\nFirst page, main frame, standard world - Destroyed\nFirst page, main frame, non-standard world - Destroyed\nFirst page, subframe, standard world - Destroyed\nFirst page, subframe, non-standard world - Destroyed\nSecond page, main frame, standard world - Destroyed\nSecond page, main frame, non-standard world - Destroyed\nFirst page, main frame, standard world - Removed\nFirst page, main frame, non-standard world - Removed\nFirst page, subframe, standard world - Removed\nFirst page, subframe, non-standard world - Removed",
"TestComplete"
};

static Vector<WKRetainPtr<WKStringRef> > messages;

static void didReceiveMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef messageBody, const void*)
{
    ASSERT_NOT_NULL(messageBody);
    EXPECT_EQ(WKStringGetTypeID(), WKGetTypeID(messageBody));

    WKStringRef bodyString = (WKStringRef)messageBody;
    messages.append(bodyString);

    if (WKStringIsEqualToUTF8CString(messageName, "DidFinishLoadForMainFrame") || WKStringIsEqualToUTF8CString(messageName, "TestComplete"))
      finished = true;
}

TEST(WebKit2, DISABLED_DOMWindowExtensionNoCache)
{
    WKRetainPtr<WKPageGroupRef> pageGroup(AdoptWK, WKPageGroupCreateWithIdentifier(WKStringCreateWithUTF8CString("DOMWindowExtensionNoCachePageGroup")));

    WKRetainPtr<WKContextRef> context(AdoptWK, Util::createContextForInjectedBundleTest("DOMWindowExtensionNoCache", pageGroup.get()));

    WKContextInjectedBundleClient injectedBundleClient;
    memset(&injectedBundleClient, 0, sizeof(injectedBundleClient));
    injectedBundleClient.version = kWKContextInjectedBundleClientCurrentVersion;
    injectedBundleClient.clientInfo = 0;
    injectedBundleClient.didReceiveMessageFromInjectedBundle = didReceiveMessageFromInjectedBundle;
    WKContextSetInjectedBundleClient(context.get(), &injectedBundleClient);

    // Disable the page cache.
    WKContextSetCacheModel(context.get(), kWKCacheModelDocumentViewer);

    PlatformWebView webView(context.get(), pageGroup.get());

    // Make sure the extensions for each frame are installed in each world.
    WKRetainPtr<WKURLRef> url1(AdoptWK, Util::createURLForResource("simple-iframe", "html"));
    WKPageLoadURL(webView.page(), url1.get());

    Util::run(&finished);
    finished = false;

    // Make sure those first 4 extensions are destroyed, and 2 new ones are installed.
    WKRetainPtr<WKURLRef> url2(AdoptWK, Util::createURLForResource("simple", "html"));
    WKPageLoadURL(webView.page(), url2.get());

    Util::run(&finished);
    finished = false;

    // Make sure those 2 are destroyed, and the first 4 are recreated.
    WKPageGoBack(webView.page());

    Util::run(&finished);
    finished = false;

    WKPageClose(webView.page());

    Util::run(&finished);

    const size_t expectedSize = sizeof(expectedMessages) / sizeof(const char*);
    EXPECT_EQ(expectedSize, messages.size());

    if (messages.size() != expectedSize)
      return;

    for (size_t i = 0; i < messages.size(); ++i)
      EXPECT_WK_STREQ(expectedMessages[i], messages[i].get());
}

} // namespace TestWebKitAPI
