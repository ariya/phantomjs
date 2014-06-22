/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ewk_view_private.h"
#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include "Test.h"
#include "WKView.h"
#include <WebKit2/WKRetainPtr.h>

namespace TestWebKitAPI {

struct TestStatesData {
    TestStatesData(WKViewRef view, WKURLRef url)
        : view(view)
        , url(url)
        , didFinishLoad(false)
        , didCrash(false)
        , didRelaunch(false)
    {
    }

    WKViewRef view;
    WKURLRef url;
    bool didFinishLoad;
    bool didCrash;
    bool didRelaunch;
};

static void didFinishLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef, const void* clientInfo)
{
    TestStatesData* states = const_cast<TestStatesData*>(static_cast<const TestStatesData*>(clientInfo));
    states->didFinishLoad = true;
}

static void setPageLoaderClient(WKPageRef page, const void* clientInfo)
{
    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));

    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;
    loaderClient.clientInfo = clientInfo;

    WKPageSetPageLoaderClient(page, &loaderClient);
}

void webProcessCrashed(WKViewRef view, WKURLRef url, const void* clientInfo)
{
    TestStatesData* states = const_cast<TestStatesData*>(static_cast<const TestStatesData*>(clientInfo));

    EXPECT_EQ(states->view, view);
    EXPECT_TRUE(WKURLIsEqual(url, states->url));

    states->didCrash = true;
}

void webProcessDidRelaunch(WKViewRef view, const void* clientInfo)
{
    TestStatesData* states = const_cast<TestStatesData*>(static_cast<const TestStatesData*>(clientInfo));

    EXPECT_EQ(states->view, view);

    states->didRelaunch = true;
}

static void setViewClient(WKViewRef view, const void* clientInfo)
{
    WKViewClient viewClient;
    memset(&viewClient, 0, sizeof(WKViewClient));

    viewClient.version = kWKViewClientCurrentVersion;
    viewClient.clientInfo = clientInfo;
    viewClient.webProcessCrashed = webProcessCrashed;
    viewClient.webProcessDidRelaunch = webProcessDidRelaunch;

    WKViewSetViewClient(view, &viewClient);
}

TEST(WebKit2, WKViewClientWebProcessCallbacks)
{
    WKRetainPtr<WKContextRef> context = adoptWK(Util::createContextForInjectedBundleTest("WKViewClientWebProcessCallbacksTest"));
    WKRetainPtr<WKURLRef> url(AdoptWK, Util::createURLForResource("simple", "html"));

    PlatformWebView view(context.get());
    WKViewRef wkView = EWKViewGetWKView(view.platformView());

    TestStatesData states = TestStatesData(wkView, url.get());

    setPageLoaderClient(view.page(), &states);
    setViewClient(wkView, &states);

    WKPageLoadURL(view.page(), url.get());
    Util::run(&states.didFinishLoad);

    WKContextPostMessageToInjectedBundle(context.get(), Util::toWK("Crash").get(), 0);
    Util::run(&states.didCrash);

    WKPageReload(view.page());
    Util::run(&states.didRelaunch);
}

} // namespace TestWebKitAPI
