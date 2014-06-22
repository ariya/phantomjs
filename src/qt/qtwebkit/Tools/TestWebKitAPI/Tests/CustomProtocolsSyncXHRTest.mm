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

#import "config.h"
#import "JavaScriptTest.h"
#import "Test.h"

#import "PlatformUtilities.h"
#import "TestBrowsingContextLoadDelegate.h"
#import "TestProtocol.h"
#import <WebKit2/WKBrowsingContextGroupPrivate.h>
#import <WebKit2/WKPreferencesPrivate.h>
#import <WebKit2/WKRetainPtr.h>
#import <WebKit2/WKString.h>
#import <WebKit2/WKViewPrivate.h>
#import <wtf/RetainPtr.h>

static bool testFinished = false;

namespace TestWebKitAPI {

TEST(WebKit2CustomProtocolsTest, SyncXHR)
{
    [NSURLProtocol registerClass:[TestProtocol class]];
    [WKBrowsingContextController registerSchemeForCustomProtocol:[TestProtocol scheme]];

    RetainPtr<WKProcessGroup> processGroup = adoptNS([[WKProcessGroup alloc] init]);
    RetainPtr<WKBrowsingContextGroup> browsingContextGroup = adoptNS([[WKBrowsingContextGroup alloc] initWithIdentifier:@"TestIdentifier"]);

    // Allow file URLs to load non-file resources
    WKRetainPtr<WKPreferencesRef> preferences(AdoptWK, WKPreferencesCreate());
    WKPreferencesSetUniversalAccessFromFileURLsAllowed(preferences.get(), true);
    WKPageGroupSetPreferences(browsingContextGroup.get()._pageGroupRef, preferences.get());

    RetainPtr<WKView> wkView = adoptNS([[WKView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) processGroup:processGroup.get() browsingContextGroup:browsingContextGroup.get()]);
     RetainPtr<TestBrowsingContextLoadDelegate> delegate = adoptNS([[TestBrowsingContextLoadDelegate alloc] initWithBlockToRunOnLoad:^(WKBrowsingContextController *sender) {
         EXPECT_JS_EQ(wkView.get().pageRef, "window._testResult", "PASS");
         testFinished = true;
    }]);
    wkView.get().browsingContextController.loadDelegate = delegate.get();

    WKPageLoadURL(wkView.get().pageRef, Util::createURLForResource("custom-protocol-sync-xhr", "html"));

    TestWebKitAPI::Util::run(&testFinished);
}

} // namespace TestWebKitAPI
