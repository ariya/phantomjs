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
#include <WebKit2/WKDownload.h>

namespace TestWebKitAPI {

static bool didDecideDestination;

static void decidePolicyForNavigationAction(WKPageRef, WKFrameRef, WKFrameNavigationType, WKEventModifiers, WKEventMouseButton, WKURLRequestRef, WKFramePolicyListenerRef listener, WKTypeRef, const void*)
{
    WKFramePolicyListenerDownload(listener);
}

static WKStringRef decideDestinationWithSuggestedFilename(WKContextRef, WKDownloadRef download, WKStringRef, bool*, const void*)
{
    didDecideDestination = true;
    WKDownloadCancel(download);
    return Util::toWK("does not matter").leakRef();
}

static void setContextDownloadClient(WKContextRef context)
{
    WKContextDownloadClient client;
    memset(&client, 0, sizeof(client));
    client.decideDestinationWithSuggestedFilename = decideDestinationWithSuggestedFilename;

    WKContextSetDownloadClient(context, &client);
}

static void setPagePolicyClient(WKPageRef page)
{
    WKPagePolicyClient policyClient;
    memset(&policyClient, 0, sizeof(policyClient));
    policyClient.decidePolicyForNavigationAction = decidePolicyForNavigationAction;

    WKPageSetPagePolicyClient(page, &policyClient);
}

TEST(WebKit2, DownloadDecideDestinationCrash)
{
    WKRetainPtr<WKContextRef> context = adoptWK(WKContextCreate());
    setContextDownloadClient(context.get());

    PlatformWebView webView(context.get());
    setPagePolicyClient(webView.page());

    // The length of this filename was specially chosen to trigger the crash conditions in
    // <http://webkit.org/b/61142>. Specifically, it causes ArgumentDecoder::m_bufferPos and m_bufferEnd
    // to be equal after the DecideDestinationWithSuggestedFilename message has been handled.
    WKPageLoadURL(webView.page(), adoptWK(Util::createURLForResource("18-characters", "html")).get());

    Util::run(&didDecideDestination);
}

} // namespace TestWebKitAPI
