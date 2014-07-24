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
#include <WebKit2/WKRetainPtr.h>

namespace TestWebKitAPI {

static bool done;

static void didReceiveMessageFromInjectedBundle(WKContextRef context, WKStringRef, WKTypeRef messageBody, const void* clientInfo)
{
    EXPECT_WK_STREQ("Set in the getInjectedBundleInitializationUserData callback", static_cast<WKStringRef>(messageBody));
    done = true;
}

static WKTypeRef getInjectedBundleInitializationUserData(WKContextRef context, const void* clientInfo)
{
    return Util::createInitializationDictionaryForInjectedBundleTest("InjectedBundleInitializationUserDataCallbackWinsTest",  Util::toWK("Set in the getInjectedBundleInitializationUserData callback").get());
}

TEST(WebKit2, InjectedBundleInitializationUserDataCallbackWins)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, Util::createContextWithInjectedBundle());
    WKRetainPtr<WKDictionaryRef> initializationDictionary(AdoptWK, Util::createInitializationDictionaryForInjectedBundleTest("InjectedBundleInitializationUserDataCallbackWinsTest",  Util::toWK("Set with WKContextSetInitializationUserDataForInjectedBundle").get()));
    WKContextSetInitializationUserDataForInjectedBundle(context.get(), initializationDictionary.get());

    WKContextInjectedBundleClient injectedBundleClient;
    memset(&injectedBundleClient, 0, sizeof(injectedBundleClient));
    injectedBundleClient.version = 1;
    injectedBundleClient.clientInfo = 0;
    injectedBundleClient.didReceiveMessageFromInjectedBundle = didReceiveMessageFromInjectedBundle;
    injectedBundleClient.getInjectedBundleInitializationUserData = getInjectedBundleInitializationUserData;
    WKContextSetInjectedBundleClient(context.get(), &injectedBundleClient);

    PlatformWebView webView(context.get());

    WKRetainPtr<WKURLRef> url(AdoptWK, Util::createURLForResource("simple", "html"));
    WKPageLoadURL(webView.page(), url.get());

    Util::run(&done);
}

} // namespace TestWebKitAPI
